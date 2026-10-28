#include <stdio.h>
#include "presence.h"
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
#define UPDATE(v,mm) hh_presence_update(&s,v,mm,189,220,3,5)
int main(void)
{
    hh_presence_t s={0};
    CHECK(!UPDATE(true,230)); CHECK(!s.animal_present);
    for(int i=0;i<10;i++) CHECK(!UPDATE(true,190));
    CHECK(!s.animal_present);
    CHECK(!UPDATE(true,180)); CHECK(!UPDATE(true,180));
    CHECK(!UPDATE(false,-1)); CHECK(!s.presence_valid);
    CHECK(!UPDATE(true,180)); CHECK(!UPDATE(true,180));
    CHECK(UPDATE(true,180)); CHECK(s.animal_present);
    for(int i=0;i<100;i++)CHECK(!UPDATE(true,100));
    CHECK(s.animal_present);
    for(int i=0;i<10;i++)CHECK(!UPDATE(false,-1));
    CHECK(s.animal_present && !s.presence_valid);
    for(int i=0;i<4;i++)CHECK(!UPDATE(true,220));
    CHECK(s.animal_present);
    CHECK(!UPDATE(true,210)); /* Hysteresis band resets departure streak. */
    for(int i=0;i<4;i++)CHECK(!UPDATE(true,220));
    CHECK(s.animal_present);
    CHECK(!UPDATE(true,220)); CHECK(!s.animal_present);
    CHECK(!UPDATE(true,189)); CHECK(!UPDATE(true,189));
    CHECK(UPDATE(true,189));
    puts("PASS: entry debounce, hysteresis, invalid readings, single trigger, re-entry");
    return 0;
}
