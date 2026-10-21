/* lab.c - the console lab, off unless the browser starts with the argument
 * lab=1, which Indigo passes when its own console lab (LabServer) is on. TCP
 * 4300 then answers the same requests as Indigo's lab, so one test run drives
 * Indigo, opens the Web face and goes on driving the browser:
 *   GET  /xfb    the framebuffer on screen, raw YUYV (X-Size: width x height)
 *   GET  /log    the lab's notes: pages opened, toasts, the network's state
 *   GET  /pad    how long since a real controller's button went down, as JSON
 *   POST /pad?press=A,Right&hold=150    buttons on a virtual pad, held for ms
 *   POST /reset  restarts the console into its boot program (Indigo)
 *   POST /open?url=<url>   the browser goes there, as if typed in the address bar
 *   POST /point?x=<px>&y=<px>   the pointer goes there
 * A POST needs an X-Lab header, so no web page can press or restart. ui_lab.c
 * and ui_lab.h are Indigo's request parser, copied unchanged.
 * With the lab on, nobody has to press Reset after a test: a crash screen
 * restarts the console after 10 s, and so does deadline=<s> seconds (default
 * 900, 0 = never) with neither a lab request nor a button press. sd=0 keeps
 * the browser off its SD card, for a run that must not write it (the browser
 * writes gcbrowser/seed.bin as it connects). */
#include <gccore.h>
#include <malloc.h>
#include <network.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ogc/lwp_watchdog.h>
#include "browser.h"
#include "gfx.h"
#include "input.h"
#include "lab.h"
#include "netio.h"
#include "ui_lab.h"

#define LAB_SEND_PIECE 16384u

extern void __reload(void);
extern void __exception_setreload(int seconds);

static bool on;
static lwp_t thread = LWP_THREAD_NULL;
static syswd_t deadline;
static u64 start;
static u32 limit_ms;
/* Milliseconds since start, 32 bits so the timer interrupt never reads half a write. */
static volatile u32 seen_ms;   /* the last lab request or button press */
static volatile u32 real_ms;   /* the last button press */
static volatile u32 virtual_bits, virtual_until_ms;
static char ring[16384];
static u32 ringlen;
static char open_url[1024];
static volatile bool open_pending, reset_pending, point_pending;
static volatile int point_x, point_y;

static u32 now_ms(void)
{
	return (u32)ticks_to_millisecs(diff_ticks(start, gettime()));
}

static const char *arg(int argc, char **argv, const char *key, const char *def)
{
	size_t k = strlen(key);
	for (int i = 1; i < argc; i++)
		if (!strncmp(argv[i], key, k) && argv[i][k] == '=')
			return argv[i] + k + 1;
	return def;
}

/* From the timer interrupt, so it restarts a browser that hangs too. */
static void deadline_check(syswd_t alarm, void *unused)
{
	if (now_ms() - seen_ms > limit_ms)
		__reload();
}

bool lab_init(int argc, char **argv)
{
	on = strcmp(arg(argc, argv, "lab", "0"), "1") == 0;
	if (!on) return true;
	start = gettime();
	__exception_setreload(10);
	limit_ms = (u32)atoi(arg(argc, argv, "deadline", "900")) * 1000u;
	struct timespec every = {10, 0};
	if (limit_ms && SYS_CreateAlarm(&deadline) == 0)
		SYS_SetPeriodicAlarm(deadline, &every, &every, deadline_check, NULL);
	return strcmp(arg(argc, argv, "sd", "1"), "0") != 0;
}

void lab_note(const char *fmt, ...)
{
	if (!on) return;
	char line[300];
	va_list ap;
	int n = snprintf(line, sizeof line, "[%7u ms] ", (unsigned)now_ms());
	va_start(ap, fmt);
	n += vsnprintf(line + n, sizeof line - n, fmt, ap);
	va_end(ap);
	if (n >= (int)sizeof line - 1) n = sizeof line - 2;
	line[n++] = '\n';

	u32 level = IRQ_Disable();
	if (ringlen + n > sizeof ring)	/* Deliberately simple: a full ring starts over */
		ringlen = 0;
	memcpy(ring + ringlen, line, n);
	ringlen += n;
	IRQ_Restore(level);
}

u32 lab_pad(u32 real_down)
{
	if (!on) return 0;
	u32 now = now_ms();
	if (real_down) seen_ms = real_ms = now;
	return now < virtual_until_ms ? virtual_bits : 0;
}

void lab_frame(void)
{
	if (!on) return;
	if (point_pending) {
		point_pending = false;
		input_set_cursor(point_x, point_y);
	}
	if (open_pending) {
		open_pending = false;
		browser_navigate(open_url);
	}
	if (reset_pending)
		SYS_ResetSystem(SYS_HOTRESET, 0, FALSE);
}

static bool lab_send(s32 client, const void *data, size_t length)
{
	const u8 *at = data;
	while (length > 0) {
		s32 sent = net_send(client, at, length > LAB_SEND_PIECE ? LAB_SEND_PIECE : length, 0);
		if (sent <= 0) return false;
		at += sent;
		length -= sent;
	}
	return true;
}

static void lab_reply(s32 client, const char *status, const char *type, const void *body, size_t length)
{
	char head[160];
	int n = snprintf(head, sizeof head, "HTTP/1.0 %s\r\nContent-Type: %s\r\nContent-Length: %u\r\n\r\n",
		status, type, (unsigned)length);
	if (n > 0 && lab_send(client, head, n)) lab_send(client, body, length);
}

static void lab_text(s32 client, const char *status, const char *text)
{
	lab_reply(client, status, "text/plain", text, strlen(text));
}

/* The frame on screen, copied just after a retrace, as Indigo's lab does. */
static void lab_xfb(s32 client)
{
	GXRModeObj *mode = gfx_mode();
	u32 size = VIDEO_GetFrameBufferSize(mode);
	u8 *frame = memalign(32, size);
	char head[192];

	if (!frame) {
		lab_text(client, "503 Service Unavailable", "no memory for a frame\n");
		return;
	}
	VIDEO_WaitVSync();
	void *shown = VIDEO_GetCurrentFramebuffer();
	memcpy(frame, MEM_PHYSICAL_TO_K1(MEM_VIRTUAL_TO_PHYSICAL(shown)), size);
	int n = snprintf(head, sizeof head, "HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\n"
		"Content-Length: %u\r\nX-Size: %ux%u\r\nX-Format: yuyv422\r\n\r\n",
		(unsigned)size, (unsigned)(size / 2 / mode->xfbHeight), (unsigned)mode->xfbHeight);
	if (n > 0 && lab_send(client, head, n)) lab_send(client, frame, size);
	free(frame);
}

static void lab_log(s32 client)
{
	char *text = malloc(sizeof ring + 256);
	if (!text) {
		lab_text(client, "503 Service Unavailable", "no memory for the log\n");
		return;
	}
	int n = snprintf(text, 256, "network: %s (ip %s)\n", netio_status(), netio_ip());
	u32 level = IRQ_Disable();
	memcpy(text + n, ring, ringlen);
	n += ringlen;
	IRQ_Restore(level);
	lab_reply(client, "200 OK", "text/plain; charset=utf-8", text, n);
	free(text);
}

static u32 lab_pad_bits(unsigned buttons)
{
	static const struct { unsigned lab; u32 pad; } map[] = {
		{UI_LAB_A, PAD_BUTTON_A}, {UI_LAB_B, PAD_BUTTON_B}, {UI_LAB_X, PAD_BUTTON_X},
		{UI_LAB_Y, PAD_BUTTON_Y}, {UI_LAB_Z, PAD_TRIGGER_Z}, {UI_LAB_L, PAD_TRIGGER_L},
		{UI_LAB_R, PAD_TRIGGER_R}, {UI_LAB_START, PAD_BUTTON_START}, {UI_LAB_UP, PAD_BUTTON_UP},
		{UI_LAB_DOWN, PAD_BUTTON_DOWN}, {UI_LAB_LEFT, PAD_BUTTON_LEFT}, {UI_LAB_RIGHT, PAD_BUTTON_RIGHT},
	};
	u32 bits = 0;
	for (unsigned i = 0; i < sizeof map / sizeof map[0]; i++)
		if (buttons & map[i].lab) bits |= map[i].pad;
	return bits;
}

/* url=<percent-encoded url> into open_url. */
static bool lab_take_url(const char *query, size_t length)
{
	if (!query || length < 5 || strncmp(query, "url=", 4)) return false;
	size_t n = 0;
	for (size_t i = 4; i < length && n < sizeof open_url - 1; i++) {
		char c = query[i];
		if (c == ' ' || c == '&' || c == '\r' || c == '\n') break;
		if (c == '%' && i + 2 < length) {
			char hex[3] = {query[i + 1], query[i + 2], 0};
			c = (char)strtol(hex, NULL, 16);
			i += 2;
		} else if (c == '+')
			c = ' ';
		open_url[n++] = c;
	}
	open_url[n] = 0;
	return n > 0;
}

/* The request's head: up to its blank line, a full buffer, or 5 s of quiet. */
static s32 lab_read_head(s32 client, char *request, size_t capacity)
{
	size_t got = 0;
	while (got < capacity) {
		fd_set readable;
		struct timeval wait = {5, 0};
		FD_ZERO(&readable);
		FD_SET(client, &readable);
		if (net_select(client + 1, &readable, NULL, NULL, &wait) <= 0) break;
		s32 n = net_recv(client, request + got, capacity - got, 0);
		if (n <= 0) break;
		got += n;
		for (size_t i = 1; i < got; i++)
			if (request[i] == '\n' && (request[i - 1] == '\n' ||
			    (i >= 2 && request[i - 1] == '\r' && request[i - 2] == '\n')))
				return got;
	}
	return got;
}

/* request is cut after its headers (lab_serve), so an X-Lab line in a body,
 * which a web page's form can send, does not count. */
static bool lab_has_header(const char *request)
{
	return strstr(request, "\nX-Lab:") || strstr(request, "\nx-lab:");
}

static void lab_serve(s32 client)
{
	char request[2048], status[48];
	s32 got = lab_read_head(client, request, sizeof request - 1);
	if (got <= 0) return;
	request[got] = 0;
	char *blank = strstr(request, "\n\r\n");
	if (!blank) blank = strstr(request, "\n\n");
	if (blank) blank[1] = 0;
	got = strlen(request);

	if (!strncmp(request, "POST /open?", 11)) {	/* the browser's own route; Indigo's parser has no /open */
		const char *q = request + 11, *end = strchr(q, ' ');
		if (!lab_has_header(request))
			lab_text(client, "403 Forbidden", "a POST needs an X-Lab header\n");
		else if (!end || !lab_take_url(q, end - q))
			lab_text(client, "400 Bad Request", "POST /open?url=<percent-encoded url>\n");
		else {
			lab_note("lab: open %s", open_url);
			open_pending = true;
			lab_text(client, "200 OK", "opening\n");
		}
		return;
	}

	if (!strncmp(request, "POST /point?", 12)) {
		int x, y;
		if (!lab_has_header(request))
			lab_text(client, "403 Forbidden", "a POST needs an X-Lab header\n");
		else if (sscanf(request + 12, "x=%d&y=%d", &x, &y) != 2)
			lab_text(client, "400 Bad Request", "POST /point?x=<px>&y=<px>\n");
		else {
			point_x = x;
			point_y = y;
			point_pending = true;
			lab_text(client, "200 OK", "pointed\n");
		}
		return;
	}

	uiLabRequest_t parsed = UILab_ParseRequest(request, got);
	uiLabPress_t press;
	switch (parsed.route) {
		case UI_LAB_XFB:
			lab_xfb(client);
			break;
		case UI_LAB_LOG:
			lab_log(client);
			break;
		case UI_LAB_PAD_STATUS:
			snprintf(status, sizeof status, "{\"real_idle_ms\":%u}\n", (unsigned)(now_ms() - real_ms));
			lab_reply(client, "200 OK", "application/json", status, strlen(status));
			break;
		case UI_LAB_PAD_PRESS:
			if (!UILab_ParsePress(parsed.query, parsed.queryLength, &press)) {
				lab_text(client, "400 Bad Request",
					"press=<A,B,X,Y,Z,L,R,Start,Up,Down,Left,Right>[&hold=<1-5000 ms>]\n");
				break;
			}
			virtual_bits = lab_pad_bits(press.buttons);
			virtual_until_ms = now_ms() + press.holdMs;
			lab_text(client, "200 OK", "pressed\n");
			break;
		case UI_LAB_RESET:
			lab_note("lab: restart requested");
			lab_text(client, "200 OK", "restarting\n");
			net_close(client);
			usleep(200000);	/* the reply leaves before the restart */
			reset_pending = true;	/* lab_frame restarts at the next frame */
			sleep(3);
			__reload();	/* the main loop is stuck: restart from here */
			break;
		case UI_LAB_FORBIDDEN:
			lab_text(client, "403 Forbidden", "a POST needs an X-Lab header\n");
			break;
		case UI_LAB_BAD_REQUEST:
			lab_text(client, "400 Bad Request", "not an HTTP request\n");
			break;
		default:
			lab_text(client, "404 Not Found",
				"GET /xfb, GET /log, GET /pad, POST /pad?press=..., POST /reset, POST /open?url=..., POST /point?x=..&y=..\n");
			break;
	}
}

static void *lab_thread(void *unused)
{
	while (netio_state() != NET_UP)	/* the fetch worker brings the network up at start, and retries */
		usleep(100000);
	struct sockaddr_in me = {.sin_family = AF_INET, .sin_port = htons(UI_LAB_PORT), .sin_addr.s_addr = INADDR_ANY};
	s32 server = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (server < 0 || net_bind(server, (struct sockaddr *)&me, sizeof me) < 0 || net_listen(server, 2) < 0) {
		lab_note("lab: cannot listen on port %d", UI_LAB_PORT);
		return NULL;
	}
	lab_note("lab: listening on %s:%d", netio_ip(), UI_LAB_PORT);
	for (;;) {
		struct sockaddr_in from;
		u32 length = sizeof from;
		s32 client = net_accept(server, (struct sockaddr *)&from, &length);
		if (client < 0) {
			usleep(100000);
			continue;
		}
		seen_ms = now_ms();
		lab_serve(client);
		net_close(client);
	}
	return NULL;
}

void lab_start(void)
{
	if (on && thread == LWP_THREAD_NULL)
		LWP_CreateThread(&thread, lab_thread, NULL, NULL, 32 * 1024, 60);
}
