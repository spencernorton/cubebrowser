#include "start_pref.h"
#include "storage.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char *settings_search_prefix(void) { return "https://example.org/?q="; }
static char disk[256];static size_t disk_length;static bool writable=true;
char *storage_read_limit(const char *path,size_t maximum,size_t *length) {
    (void)path;if(disk_length>maximum)return NULL;
    char *data=malloc(disk_length+1);assert(data);memcpy(data,disk,disk_length);data[disk_length]=0;*length=disk_length;return data;
}
bool storage_write_atomic(const char *path,const char *temp,const void *data,size_t length) {
    assert(strstr(path,"/start.pref") && strstr(temp,"/start.tmp"));
    if(!writable)return false;assert(length<sizeof(disk));memcpy(disk,data,length);disk[length]=0;disk_length=length;return true;
}
static const char *url="https://norvitech.com/indigo/start/";
static const char *cookie="indigo_zip=80202; Path=/indigo/start/; Secure; HttpOnly; SameSite=Lax; Max-Age=31536000";
static char output[64];
static const uint64_t now=1793320000;
static void header(const char *u,bool verified,uint64_t time) {start_pref_request(u,verified,time,output,sizeof(output));}
int main(void) {
    start_pref_load();header(url,true,now);assert(!output[0]);
    assert(start_pref_receive(url,true,cookie,now));assert(start_pref_take_notice()==START_PREF_SAVED);
    assert(start_pref_take_notice()==START_PREF_NONE);header(url,true,now);assert(!strcmp(output,"Cookie: indigo_zip=80202\r\n"));
    assert(strstr(disk,"v1\n80202\n"));start_pref_load();header(url,true,now+1);assert(strstr(output,"80202"));
    const char *outside[]={"http://norvitech.com/indigo/start/","https://norvitech.com.evil/indigo/start/",
        "https://sub.norvitech.com/indigo/start/","https://norvitech.com/other","https://norvitech.com/indigo/start-evil/",
        "https://norvitech.com:8443/indigo/start/", "https://norvitech.com/indigo/start/../other",
        "https://norvitech.com/indigo/start/%2e%2e/other"};
    for(unsigned i=0;i<sizeof(outside)/sizeof(outside[0]);i++) {
        header(outside[i],true,now);assert(!output[0]);assert(!start_pref_receive(outside[i],true,cookie,now));
    }
    header(url,false,now);assert(!output[0]);assert(!start_pref_receive(url,false,cookie,now));
    header(url,true,now+31536000);assert(!output[0]);header(url,true,now-1);assert(!output[0]);
    const char *bad[]={"indigo_zip=80202; Secure","indigo_zip=8020x; Path=/indigo/start/; Secure; HttpOnly; SameSite=Lax; Max-Age=100",
        "indigo_zip=80202; Path=/; Secure; HttpOnly; SameSite=Lax; Max-Age=100",
        "indigo_zip=80202; Path=/indigo/start/; Secure; HttpOnly; SameSite=None; Max-Age=100",
        "indigo_zip=80202; Path=/indigo/start/; Secure; HttpOnly; SameSite=Lax; Max-Age=99999999999999999999999999",
        "indigo_zip=80202; Path=/indigo/start/; Secure; HttpOnly; SameSite=Lax; Max-Age=100; Domain=norvitech.com",
        "indigo_zip=80202; Path=/indigo/start/; Secure; HttpOnly; SameSite=Lax; Max-Age=100; Max-Age=0",
        "indigo_zip=80202\r\nX: bad; Path=/indigo/start/; Secure; HttpOnly; SameSite=Lax; Max-Age=100"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++)assert(!start_pref_receive(url,true,bad[i],now));
    assert(start_pref_receive(url,true,"indigo_zip=\"\"; Path=/indigo/start/; Secure; HttpOnly; SameSite=Lax; Max-Age=0",now));
    assert(start_pref_take_notice()==START_PREF_CLEARED);start_pref_load();header(url,true,now);assert(!output[0]);
    writable=false;assert(start_pref_receive(url,true,cookie,now));assert(start_pref_take_notice()==START_PREF_UNSAVED);
    header(url,true,now);assert(strstr(output,"80202"));start_pref_load();header(url,true,now);assert(!output[0]);writable=true;
    for(unsigned round=0,seed=17;round<5000;round++) {
        disk_length=round%sizeof(disk);for(size_t j=0;j<disk_length;j++){seed=seed*1664525u+1013904223u;disk[j]=(char)(seed>>24);}
        start_pref_load();header(url,true,now);assert(!output[0]);
    }
    puts("start preference: exact origin/path/trust, five digits, bounded expiry, card reload, clearing and save failure pass");
}
