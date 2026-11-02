#pragma once
#ifdef _WIN32
#include "windows_clock.hpp"
using ClockZoneInfo = DYNAMIC_TIME_ZONE_INFORMATION;
#else
#include <QDateTime>
#include <QTimeZone>
#include <QImage>
#include <QPainter>
#include <QFontMetrics>
#include <algorithm>
#include <ctime>
#include <string>
#include <vector>
#include <graphics/graphics.h>
using ClockZoneInfo = QTimeZone;
struct ClockZone { std::string key,label; ClockZoneInfo info; };
inline const std::vector<ClockZone>& clock_zones() {
 static const auto zones=[] {
  std::vector<ClockZone> out;
  for(const auto &id:QTimeZone::availableTimeZoneIds()) {
   QTimeZone zone(id);
   if(zone.isValid())out.push_back({id.toStdString(),id.toStdString(),zone});
  }
  std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.label<b.label;});
  return out;
 }(); return zones;
}
inline std::tm zoned_time(std::time_t now,const ClockZoneInfo *zone,bool utc) {
 const auto tz=zone?*zone:(utc?QTimeZone("UTC"):QTimeZone::systemTimeZone());
 const auto dt=QDateTime::fromSecsSinceEpoch(qint64(now),tz);
 const auto date=dt.date();const auto time=dt.time();std::tm result{};
 result.tm_year=date.year()-1900;result.tm_mon=date.month()-1;result.tm_mday=date.day();
 result.tm_hour=time.hour();result.tm_min=time.minute();result.tm_sec=time.second();
 result.tm_wday=date.dayOfWeek()%7;result.tm_yday=date.dayOfYear()-1;result.tm_isdst=dt.isDaylightTime()?1:0;
 return result;
}
struct ClockTextLine {std::string text;int pixels;int weight;uint32_t color;};
inline QImage clock_text_image(const std::vector<ClockTextLine>& lines) {
 if(lines.empty())return {};
 QImage image(500,int(lines.size())*44,QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);
 QPainter painter(&image);painter.setRenderHint(QPainter::TextAntialiasing);
 for(size_t row=0;row<lines.size();++row){
  const auto& line=lines[row];QString text=QString::fromUtf8(line.text.data(),int(line.text.size()));
  QFont font; font.setWeight(line.weight>=600?QFont::DemiBold:QFont::Normal);
  int pixels=line.pixels; font.setPixelSize(pixels);
  while(pixels>8 && QFontMetrics(font).horizontalAdvance(text)>480)font.setPixelSize(--pixels);
  painter.setFont(font);painter.setPen(QColor(line.color&255,(line.color>>8)&255,(line.color>>16)&255));
  text=QFontMetrics(font).elidedText(text,Qt::ElideRight,480);
  painter.drawText(QRect(10,int(row)*44,480,44),Qt::AlignCenter|Qt::TextSingleLine,text);
 }
 painter.end();image=image.convertToFormat(QImage::Format_RGBA8888);
 // Keep coverage alpha but use exact straight RGB, matching the Windows backend.
 for(int y=0;y<image.height();++y){auto *pixels=image.scanLine(y);auto color=lines[size_t(y/44)].color;
  for(int x=0;x<500;++x){pixels[x*4]=color&255;pixels[x*4+1]=(color>>8)&255;pixels[x*4+2]=(color>>16)&255;}
 }
 return image;
}
inline gs_texture_t *clock_text_texture(const std::vector<ClockTextLine>& lines) {
 auto image=clock_text_image(lines);if(image.isNull())return nullptr;
 const uint8_t *data[]={image.constBits()};
 return gs_texture_create(image.width(),image.height(),GS_RGBA,1,data,0);
}
#endif
