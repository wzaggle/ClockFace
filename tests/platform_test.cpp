#include "../src/platform_clock.hpp"
#include <QGuiApplication>
#include <cstdlib>
#include <iostream>
void check(bool ok,const char *message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(int argc,char **argv){
 QGuiApplication app(argc,argv);
 QTimeZone ny("America/New_York");check(ny.isValid(),"zone database");
 auto utc=zoned_time(1704067200,nullptr,true);
 check(utc.tm_year==124&&utc.tm_mon==0&&utc.tm_mday==1&&utc.tm_hour==0,"UTC conversion");
 auto winter=zoned_time(1704067200,&ny,false);
 check(winter.tm_year==123&&winter.tm_mon==11&&winter.tm_mday==31&&winter.tm_hour==19,"winter conversion");
 auto summer=zoned_time(1719792000,&ny,false);
 check(summer.tm_hour==20&&summer.tm_isdst==1,"DST conversion");
 auto image=clock_text_image({{"Service starts in 05:00",24,600,0xff0080ff}});
 check(image.width()==500&&image.height()==44,"text dimensions");
 bool ink=false,clear=false;
 for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){auto c=image.pixelColor(x,y);if(c.alpha()){ink=true;check(c.red()==255&&c.green()==128&&c.blue()==0,"straight alpha color");}else clear=true;}
 check(ink&&clear,"text coverage and transparent background");
 check(clock_text_image({}).isNull(),"empty text");
 check(!clock_zones().empty(),"zone enumeration");
}
