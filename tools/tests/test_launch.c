#include "launch.h"
#include "storage.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char *const button_names[BTN_COUNT] = {"A","B","X","Y","Z","L","R","START"};
static char output[8192];
char *storage_read(const char *path, size_t *length) {
    (void)path;
    const char *ini = "disp.theme=2\ndisp.aspect=0\ndisp.margin=4\ndisp.anim=1\ndisp.sounds=1\npad.rumble=1\nweb.home=https://example.org/custom\n";
    *length=strlen(ini); return strdup(ini);
}
bool storage_write_atomic(const char *path,const char *temp,const void *data,size_t length) {
    (void)path;(void)temp;assert(length<sizeof(output));memcpy(output,data,length);output[length]=0;return true;
}
static void parse(char **args,int count) { launch_parse(count,args); }
int main(void) {
    settings_load(); Settings before=g_set;
    char *args[]={"browser","indigo.context=1","indigo.color=azure","indigo.wide=1","indigo.video=480p",
        "indigo.margin=6","indigo.motion=1","indigo.sounds=0","indigo.rumble=0","indigo.timezone=840","indigo.device=gcldr"};
    parse(args,sizeof(args)/sizeof(args[0]));assert(g_launch.active && g_launch.color==1);
    assert(!strcmp(g_launch.device,"gcldr"));launch_capture(&g_set);launch_apply(&g_set);
    assert(g_set.widescreen==1 && g_set.video_mode==2 && g_set.margin==6 && !g_set.animations);
    assert(!g_set.sounds && !g_set.rumble && g_set.tz_minutes==840 && !g_set.dst);
    assert(launch_console_utc(100000)==100000-840*60);
    assert(!strcmp(g_set.home,"https://example.org/custom"));g_set.search_engine=2;
    assert(settings_save());assert(strstr(output,"disp.theme=2\n") && strstr(output,"disp.anim=1\n"));
    assert(strstr(output,"web.search=2\n") && strstr(output,"web.home=https://example.org/custom\n"));
    Settings stored=g_set;launch_preserve(&stored);assert(stored.widescreen==before.widescreen);
    assert(stored.tz_minutes==before.tz_minutes && stored.dst==before.dst && stored.rumble==before.rumble);
    char *invalid[]={"browser","indigo.context=2","indigo.wide=1"};parse(invalid,3);assert(!g_launch.active);
    g_set=before;launch_apply(&g_set);assert(!memcmp(&g_set,&before,sizeof(before)));
    char *bad[]={"browser","indigo.context=1","indigo.wide=99999999999999999999999999","indigo.margin=-1",
        "indigo.timezone=-721","indigo.device=../../other","indigo.color=1","indigo.video=480p-extra","indigo.rumble=1x"};
    parse(bad,9);assert(g_launch.active && g_launch.wide==INT_MIN && g_launch.margin==INT_MIN);
    assert(launch_console_utc(100000)==100000);
    assert(g_launch.timezone==INT_MIN && !g_launch.device[0] && g_launch.color==-1 && g_launch.video==INT_MIN);
    assert(g_launch.rumble==INT_MIN);launch_apply(&g_set);assert(!memcmp(&g_set,&before,sizeof(before)));
    puts("launch: version, bounds, named colors, card origin, saved overrides and persistence pass");
}
