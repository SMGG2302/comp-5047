#include <stdio.h>
#include "presence.h"
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
#define UPDATE(v,mm) hh_presence_update(&s,v,mm,200,250,3,5)
int main(void)
{
    hh_presence_t s={0};
    CHECK(!UPDATE(true,400)); CHECK(!s.animal_present);
    CHECK(!UPDATE(true,180)); CHECK(!UPDATE(true,180));
    CHECK(!UPDATE(false,-1)); CHECK(!s.presence_valid);
    CHECK(!UPDATE(true,180)); CHECK(!UPDATE(true,180));
    CHECK(UPDATE(true,180)); CHECK(s.animal_present);
    for(int i=0;i<100;i++)CHECK(!UPDATE(true,100));
    CHECK(s.animal_present);
    for(int i=0;i<10;i++)CHECK(!UPDATE(false,-1));
    CHECK(s.animal_present && !s.presence_valid);
    for(int i=0;i<4;i++)CHECK(!UPDATE(true,300));
    CHECK(s.animal_present);
    CHECK(!UPDATE(true,225)); /* Hysteresis band resets departure streak. */
    for(int i=0;i<4;i++)CHECK(!UPDATE(true,300));
    CHECK(s.animal_present);
    CHECK(!UPDATE(true,300)); CHECK(!s.animal_present);
    CHECK(!UPDATE(true,200)); CHECK(!UPDATE(true,200));
    CHECK(UPDATE(true,200));
    puts("PASS: entry debounce, hysteresis, invalid readings, single trigger, re-entry");
    return 0;
}
