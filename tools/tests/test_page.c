#include "page.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char paint_trace[];
extern int paint_count;

static void test_painter(void) {
    Page p;
    page_init(&p);
    p.boxes = calloc(2, sizeof(Box));
    p.nboxes = 2;
    p.arena = strdup("one two");
    for (int i = 0; i < 2; i++) {
        p.boxes[i] = (Box){.x = i * 50.0f, .y = 20, .w = 30, .h = 20, .size = 16,
                          .type = BOX_TEXT, .font = FONT_BODY, .flags = BF_UNDERLINE,
                          .field = -1, .link = i, .text = i * 4, .len = 3};
    }
    paint_count = 0;
    page_draw(&p, 0, 0, 640, 480, -1, -1);
    assert(!strcmp(paint_trace, "TTRR"));  /* adjacent links use one text pass */
    p.boxes[1].x = 20;
    paint_count = 0;
    page_draw(&p, 0, 0, 640, 480, -1, -1);
    assert(!strcmp(paint_trace, "TRTR"));  /* overlap retains the full original order */
    p.boxes[1].x = 50;
    p.boxes[1].font = FONT_MONO;
    paint_count = 0;
    page_draw(&p, 0, 0, 640, 480, -1, -1);
    assert(!strcmp(paint_trace, "TRTR"));
    page_free(&p);
}

static void test_refresh(void) {
    Page p;
    page_init(&p);
    const char *valid = "<meta http-equiv='Refresh' content=\"2; url='/next?a=1'\">";
    page_set_source(&p, strdup(valid), strlen(valid), false, "https://example.org/start");
    assert(p.refresh_seconds == 2 && !strcmp(p.refresh_url, "https://example.org/next?a=1"));
    const char *invalid[] = {"<meta http-equiv=refresh content='0;url=javascript:alert(1)'>",
        "<meta http-equiv=refresh content='999999999999999;url=/next'>",
        "<meta http-equiv=refresh content='0;url='>", "<meta http-equiv=refresh content='0;u'>"};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); i++) {
        page_set_source(&p, strdup(invalid[i]), strlen(invalid[i]), false, "https://example.org/start");
        assert(!p.refresh_url[0]);
    }
    page_free(&p);
}

static void test_forms(void) {
    Page p;
    page_init(&p);
    const char *html = "<form action='/find'><select name='sort'><option value='a'>Alpha</option>"
        "<option disabled value='bad'>Disabled</option><optgroup label='group'>"
        "<option selected value='b'>Beta</option></optgroup><optgroup disabled>"
        "<option value='bad2'>Disabled group</option></optgroup></select>"
        "<input disabled name='ignore' value='bad'><input name='query' value='hi there'></form>";
    page_set_source(&p, strdup(html), strlen(html), false, "https://example.org/start");
    assert(p.nfields == 3);
    SelectOption options[4];
    int current;
    assert(page_select_options(&p, 0, options, 4, &current) == 2 && current == 1);
    assert(!strcmp(options[0].label, "Alpha") && !strcmp(options[1].value, "b"));
    assert(page_select_choose(&p, 0, &options[0]));
    assert(!strcmp(p.fields[0].value, "a") && p.relayout);
    char url[URL_MAX], *post;
    assert(page_build_submit(&p, 0, -1, url, sizeof(url), &post));
    assert(!post && !strcmp(url, "https://example.org/find?sort=a&query=hi+there"));
    assert(page_select_options(&p, 0, options, 1, &current) == 1);
    assert(!page_select_choose(&p, -1, &options[0]));
    page_layout(&p, 400, 1);
    const int *visible;
    assert(page_visible(&p, 0, 480, &visible) > 0 && visible);
    page_free(&p);
}

int main(void) {
    test_painter();
    test_forms();
    test_refresh();
    puts("PASS: actual page painter preserves overlaps; dropdown groups/disabled choices and submitted values work");
}
