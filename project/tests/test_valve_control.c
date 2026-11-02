#include <stdio.h>
#include "valve_control.h"
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
#define UPDATE(t,light,valid,temp,fully) hh_valve_control_update(&s,t,light,valid,temp,fully)
int main(void)
{
    hh_valve_control_t s={0};
    CHECK(!UPDATE(0,true,true,30,false));
    CHECK(!UPDATE(0,false,true,31,false));
    CHECK(!UPDATE(0,true,false,31,false));
    CHECK(!UPDATE(0,true,true,NAN,false));
    CHECK(UPDATE(0,true,true,31,false));
    CHECK(UPDATE(13000,false,true,25,false)); /* Ramp does not start hold timer. */
    CHECK(!s.timing_started);
    CHECK(UPDATE(15000,false,true,25,true));
    CHECK(s.next_check_ms==75000);
    CHECK(UPDATE(74999,false,true,25,true));
    CHECK(UPDATE(75000,false,true,26,true)); /* Strictly below 26 required. */
    CHECK(s.next_check_ms==135000);
    CHECK(UPDATE(80000,false,true,25,true)); /* No checking between minutes. */
    CHECK(UPDATE(135000,false,false,25,true)); /* Invalid: hold another minute. */
    CHECK(s.next_check_ms==195000);
    CHECK(!UPDATE(195000,false,true,25.99f,true));
    CHECK(!s.timing_started);
    CHECK(!UPDATE(200000,true,true,29,false));
    CHECK(UPDATE(201000,true,true,30.01f,false));
    CHECK(UPDATE(215000,true,true,35,true));
    CHECK(UPDATE(999000,false,true,29,true)); /* Delayed check starts new interval. */
    CHECK(s.next_check_ms==1059000);
    CHECK(UPDATE(1000000,false,true,24,true));
    CHECK(!UPDATE(1059000,false,true,24,true));
    puts("PASS: light/temperature entry, strict thresholds, full-open hold, minute checks, invalid readings and re-entry");
    return 0;
}
