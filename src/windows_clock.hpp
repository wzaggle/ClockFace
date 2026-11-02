#pragma once
#include <windows.h>
#include <algorithm>
#include <string>
#include <vector>
#include <ctime>
#include <cstdint>
#include <graphics/graphics.h>

inline std::string utf8(const wchar_t *text) {
 int n=WideCharToMultiByte(CP_UTF8,0,text,-1,nullptr,0,nullptr,nullptr);
 if(n<=1)return {};
 std::string out(n,'\0');WideCharToMultiByte(CP_UTF8,0,text,-1,out.data(),n,nullptr,nullptr);out.pop_back();return out;
}
struct ClockZone {std::string key,label;DYNAMIC_TIME_ZONE_INFORMATION info;};
inline const std::vector<ClockZone>& clock_zones() {
 static const auto zones=[] {
  std::vector<ClockZone> out;
  for(DWORD i=0;;++i){
   DYNAMIC_TIME_ZONE_INFORMATION z{};
   DWORD status=EnumDynamicTimeZoneInformation(i,&z);
   if(status==ERROR_NO_MORE_ITEMS)break;
   if(status!=ERROR_SUCCESS)break;
   z.DynamicDaylightTimeDisabled=FALSE;
   out.push_back({utf8(z.TimeZoneKeyName),utf8(z.StandardName),z});
  }
  std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.label<b.label;});return out;
 }();return zones;
}
inline std::tm zoned_time(std::time_t now,const DYNAMIC_TIME_ZONE_INFORMATION *zone,bool utc) {
 std::tm result{};
 if(!zone && !utc){localtime_s(&result,&now);return result;}
 gmtime_s(&result,&now);
 if(utc)return result;
 SYSTEMTIME universal{},local{};
 universal.wYear=WORD(result.tm_year+1900);universal.wMonth=WORD(result.tm_mon+1);
 universal.wDay=WORD(result.tm_mday);universal.wHour=WORD(result.tm_hour);
 universal.wMinute=WORD(result.tm_min);universal.wSecond=WORD(result.tm_sec);
 if(SystemTimeToTzSpecificLocalTimeEx(zone,&universal,&local)){
  result.tm_year=local.wYear-1900;result.tm_mon=local.wMonth-1;result.tm_mday=local.wDay;
  result.tm_hour=local.wHour;result.tm_min=local.wMinute;result.tm_sec=local.wSecond;result.tm_wday=local.wDayOfWeek;
 }
 return result;
}
// Grayscale GDI rasterization, then explicit straight-alpha pixels for OBS.
// No FILE pointers or graphic objects cross C-runtime boundaries.
struct ClockTextLine {std::string text;int pixels;int weight;uint32_t color;};
inline gs_texture_t *clock_text_texture(const std::vector<ClockTextLine> &lines) {
 constexpr int width=500;const int height=int(lines.size())*44;
 if(height<=0)return nullptr;
 HDC dc=CreateCompatibleDC(nullptr);if(!dc)return nullptr;
 BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=width;
 info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 void *bits=nullptr;HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
 if(!bitmap || !bits){if(bitmap)DeleteObject(bitmap);DeleteDC(dc);return nullptr;}
 auto oldBitmap=SelectObject(dc,bitmap);memset(bits,0,width*height*4);
 SetBkMode(dc,TRANSPARENT);
 for(size_t row=0;row<lines.size();++row){
  const auto &line=lines[row];if(line.text.empty())continue;
  // Measure and shrink long countdown messages to stay within the source.
  int n=MultiByteToWideChar(CP_UTF8,0,line.text.c_str(),-1,nullptr,0);
  if(n<=0)continue;
  std::wstring wide(n,L'\0');MultiByteToWideChar(CP_UTF8,0,line.text.c_str(),-1,wide.data(),n);
  int pixels=line.pixels;HFONT font=nullptr;HGDIOBJ oldFont=nullptr;
  while(pixels>=8){
   font=CreateFontW(-pixels,0,0,0,line.weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");
   if(!font)break;
   oldFont=SelectObject(dc,font);SIZE size{};GetTextExtentPoint32W(dc,wide.c_str(),n-1,&size);
   if(size.cx<=480 || pixels==8)break;
   SelectObject(dc,oldFont);DeleteObject(font);font=nullptr;--pixels;
  }
  if(!font)continue;
  // Draw a white coverage mask; each row is recolored after antialiasing.
  SetTextColor(dc,RGB(255,255,255));
  RECT rect{0,int(row)*44,width,int(row+1)*44};DrawTextW(dc,wide.c_str(),-1,&rect,DT_CENTER|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
  SelectObject(dc,oldFont);DeleteObject(font);
 }
 GdiFlush();
 auto *pixels=static_cast<uint8_t*>(bits);
 for(int y=0;y<height;++y){uint32_t color=lines[y/44].color;
  for(int x=0;x<width;++x){auto *p=pixels+(y*width+x)*4;uint8_t alpha=std::max({p[0],p[1],p[2]});p[0]=(color>>16)&255;p[1]=(color>>8)&255;p[2]=color&255;p[3]=alpha;}
 }
 const uint8_t *data[]={pixels};auto *texture=gs_texture_create(width,height,GS_BGRA,1,data,0);
 SelectObject(dc,oldBitmap);DeleteObject(bitmap);DeleteDC(dc);return texture;
}
