#include "../src/time_math.hpp"
#include <cassert>
#include <cmath>
int main(){
 auto a=clock_angles(0,0,0,2);assert(a.hour==0&&a.minute==0&&a.second==0);
 a=clock_angles(15,30,30.75,0);assert(a.second==180);assert(std::abs(a.hour-105.25625)<1e-7);
 a=clock_angles(12,59,59.99,1);assert(a.second==358.5);
 a=clock_angles(12,0,0.5,2);assert(a.second==3);
 a=clock_angles(23,59,59.99,2);assert(a.hour<360&&a.minute<360&&a.second<360);
 std::tm midnight{};midnight.tm_year=126;midnight.tm_mon=10;midnight.tm_mday=2;midnight.tm_wday=1;
 assert(clock_time_label(midnight,true,true)=="00:00:00");
 assert(clock_time_label(midnight,false,false)=="12:00 AM");
 midnight.tm_hour=12;assert(clock_time_label(midnight,false,true)=="12:00:00 PM");
 midnight.tm_hour=23;midnight.tm_min=59;midnight.tm_sec=58;
 assert(clock_time_label(midnight,true,false)=="23:59");
 assert(clock_time_label(midnight,false,true)=="11:59:58 PM");
 assert(clock_date_label(midnight,true)=="2026-11-02");
 assert(clock_date_label(midnight,false)=="Mon, Nov 02, 2026");
 assert(clock_height(false,false)==500);assert(clock_height(true,false)==544);
 assert(clock_height(false,true)==544);assert(clock_height(true,true)==588);

 assert(clock_end_time("09:00")==32400);assert(clock_end_time("9:30")==34200);
 assert(clock_end_time("00:00")==0);assert(clock_end_time("23:59")==86340);
 for(auto bad:{"","24:00","12:60","9:3","09:30 AM","-1:00"," 9:00","abc"})assert(clock_end_time(bad)==-1);
 int end=clock_end_time("10:00");
 assert(clock_countdown_seconds(8,59,59,end)==0);
 assert(clock_countdown_seconds(9,0,0,end)==3600);
 assert(clock_countdown_seconds(9,30,0,end)==1800);
 assert(clock_countdown_seconds(9,59,59.8,end)==1);
 assert(clock_countdown_seconds(10,0,0,end)==0);
 assert(clock_countdown_seconds(10,0,0.1,end)==0);
 assert(clock_countdown_seconds(11,0,0,end)==0);
 assert(clock_countdown_seconds(23,30,0,clock_end_time("00:15"))==2700);
 assert(clock_countdown_seconds(23,0,0,0)==3600);
 assert(clock_countdown_seconds(0,0,0,0)==0);
 assert(clock_countdown_seconds(9,0,0,-1)==0);
 assert(clock_countdown_label("Service starts in",3600)=="Service starts in 60:00");
 assert(clock_countdown_label("",65)=="01:05");
 assert(clock_countdown_label("Service starts in",0).empty());
 for(int mask=0;mask<8;++mask)assert(clock_height(mask&1,mask&2,mask&4)==500u+44u*((mask&1?1u:0u)+(mask&2?1u:0u)+(mask&4?1u:0u)));

 assert(clock_countdown_label("Service starts in",3599)=="Service starts in 59:59");
 assert(clock_countdown_label("Service starts in",1)=="Service starts in 00:01");
 assert(clock_countdown_label("Service starts in",0).empty());
 assert(clock_height(false,false,true,true)==500);
 assert(clock_height(true,true,true,true)==588);
 assert(clock_height(true,true,true,false)==632);
}
