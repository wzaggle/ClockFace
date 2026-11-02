#pragma once
#include <cmath>
#include <cstdio>
struct ClockAngles { double hour, minute, second; };
inline ClockAngles clock_angles(int hour, int minute, double second, int movement) {
 double s = movement == 0 ? std::floor(second) : movement == 1 ? std::floor(second*4)/4 : second;
 return {30*((hour%12)+(minute+second/60)/60),6*(minute+second/60),6*s};
}

#include <ctime>
#include <string>
inline std::string clock_time_label(const std::tm &local,bool format24,bool seconds) {
 char out[64]{};
 std::strftime(out,sizeof(out),format24?(seconds?"%H:%M:%S":"%H:%M"):(seconds?"%I:%M:%S %p":"%I:%M %p"),&local);
 return out;
}
inline std::string clock_date_label(const std::tm &local,bool iso) {
 char out[64]{};std::strftime(out,sizeof(out),iso?"%Y-%m-%d":"%a, %b %d, %Y",&local);return out;
}
inline unsigned clock_height(bool digital,bool date,bool countdown=false,bool external=false){return 500+44*(unsigned(digital)+unsigned(date)+unsigned(countdown&&!external));}

inline int clock_end_time(const std::string &text) {
 // Accept H:MM or HH:MM, with a 24-hour end time and no ambiguous AM/PM.
 const auto colon=text.find(':');
 if(colon<1 || colon>2 || text.size()!=colon+3)return -1;
 int hour=0,minute=0;
 for(size_t i=0;i<colon;++i){if(text[i]<'0'||text[i]>'9')return -1;hour=hour*10+text[i]-'0';}
 for(size_t i=colon+1;i<text.size();++i){if(text[i]<'0'||text[i]>'9')return -1;minute=minute*10+text[i]-'0';}
 return hour<24&&minute<60?hour*3600+minute*60:-1;
}
inline int clock_countdown_seconds(int hour,int minute,double second,int endTime) {
 if(endTime<0 || endTime>=86400)return 0;
 double remaining=std::fmod(endTime-(hour*3600+minute*60+second)+86400.0,86400.0);
 return remaining>0 && remaining<=3600 ? int(std::ceil(remaining)) : 0;
}
inline std::string clock_countdown_label(const std::string &message,int remaining) {
 if(remaining<=0)return {};
 char buffer[24];std::snprintf(buffer,sizeof(buffer),"%02d:%02d",remaining/60,remaining%60);
 return message.empty()?std::string(buffer):message+" "+buffer;
}
