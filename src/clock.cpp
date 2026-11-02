#include <obs-module.h>
#include <graphics/image-file.h>
#include <util/bmem.h>
#include <util/platform.h>
#include <cstdio>
#include <cerrno>
#ifdef _WIN32
#include <windows.h>
#endif
#include "embedded_assets.hpp"
#include <chrono>
#include <ctime>
#include <mutex>
#include <string>
#include <algorithm>
#include "time_math.hpp"
#include "platform_clock.hpp"
#include <atomic>
OBS_DECLARE_MODULE()
MODULE_EXPORT const char *obs_module_name(void) { return "ClockFace"; }
MODULE_EXPORT const char *obs_module_description(void) { return "ClockFace 1.1.1 - native analog clock for OBS 33"; }
namespace {
constexpr float pi=3.14159265358979323846f;
struct Clock {
 std::mutex mutex;
 gs_image_file_ex images[4]{}; gs_image_file_ex halos[4]{};
 std::string imagePaths[4],haloPaths[4];
 gs_image_file_ex markings[2]{};std::string markingsPaths[2];uint32_t markingColors[2]{0xff202020,0xff202020};bool markingOverrides[2]{};bool customBackground=false;
 gs_texture_t *text=nullptr;std::string lastText;
 std::atomic<uint32_t> height{500};
 bool digital=false,date=false,format24=false,digitalSeconds=true,isoDate=false,utc=false;
 bool countdown=false;int endTime=9*3600;std::string countdownMessage;uint32_t countdownColor=0xffffffff;
 bool externalMessage=false;std::string textSourceUuid;
 uint64_t settingsRevision=0,lastTickRevision=0;std::time_t lastTickSecond=-1;
 // Published output is owned by video_tick; update only changes configuration.
 std::string outputUuid,lastOutputText;
 const ClockZoneInfo *zone=nullptr;
 float glow=.7f;bool neonFace=false,neonHands=false;uint32_t textColor=0xffffffff;
 gs_effect_t *tint=nullptr;
 gs_texrender_t *composite=nullptr; float opacity=1.f;
 int movement=2; bool seconds=true, recolor=false; uint32_t colors[4]{};
};
const char *name(void*) { return "ClockFace"; }
void load(gs_image_file_ex &im,const std::string &path) {
 gs_image_file_ex_free(&im); gs_image_file_ex_init(&im,path.c_str(),GS_IMAGE_ALPHA_STRAIGHT); gs_image_file_ex_init_texture(&im);
 if (!im.loaded) blog(LOG_WARNING,"[ClockFace] Unable to load %s",path.c_str());
}
std::string asset(const std::string &file) {
 char *p=obs_module_config_path(file.c_str()); std::string out=p?p:""; bfree(p); return out;
}
void update(void *ptr,obs_data_t *s) {
 obs_enter_graphics();
 auto &c=*static_cast<Clock*>(ptr); std::lock_guard<std::mutex> lock(c.mutex);
 if(obs_data_get_int(s,"face")==9){
  if(!obs_data_has_user_value(s,"background_image"))obs_data_set_string(s,"background_image",obs_data_get_string(s,"custom_face"));
  obs_data_set_int(s,"face",6);
 }
 if(!obs_data_get_bool(s,"background_migrated")){
  if(!obs_data_has_user_value(s,"background_enabled") && *obs_data_get_string(s,"background_image"))obs_data_set_bool(s,"background_enabled",true);
  obs_data_set_bool(s,"background_migrated",true);
 }
 c.movement=std::clamp(int(obs_data_get_int(s,"movement")),0,2);
 c.opacity=std::clamp(int(obs_data_get_int(s,"opacity")),0,100)/100.f;
 c.seconds=obs_data_get_bool(s,"seconds"); c.recolor=obs_data_get_bool(s,"recolor");
 const char *keys[]={"face_color","hour_color","minute_color","second_color"};
 for(int i=0;i<4;++i)c.colors[i]=uint32_t(obs_data_get_int(s,keys[i]));
 c.digital=obs_data_get_bool(s,"digital");c.date=obs_data_get_bool(s,"date");
 c.format24=obs_data_get_bool(s,"format24");c.digitalSeconds=obs_data_get_bool(s,"digital_seconds");
 c.isoDate=obs_data_get_bool(s,"iso_date");c.textColor=c.recolor?uint32_t(obs_data_get_int(s,"text_color")):0xffffffff;
 c.externalMessage=obs_data_get_bool(s,"external_message");c.textSourceUuid=obs_data_get_string(s,"text_source_uuid");
 c.countdown=obs_data_get_bool(s,"countdown");c.endTime=clock_end_time(obs_data_get_string(s,"end_time"));
 c.countdownMessage=obs_data_get_string(s,"countdown_message");
 c.countdownColor=c.recolor?uint32_t(obs_data_get_int(s,"countdown_color")):0xffffffff;
 c.height=clock_height(c.digital,c.date,c.countdown,c.externalMessage);c.lastText.clear();++c.settingsRevision;
 c.glow=std::clamp(int(obs_data_get_int(s,"glow")),0,100)/100.f;
 std::string tz=obs_data_get_string(s,"timezone");c.utc=tz=="UTC";c.zone=nullptr;
 if(!tz.empty() && !c.utc){
  for(const auto &z:clock_zones())if(z.key==tz){c.zone=&z.info;break;}
  if(!c.zone)blog(LOG_WARNING,"[ClockFace] Unknown time zone '%s'; using system local time",tz.c_str());
 }
 const int face=std::clamp(int(obs_data_get_int(s,"face")),0,13);
 if(!obs_data_has_user_value(s,"show_ticks") && obs_data_has_user_value(s,"show_markings")){
  bool old=obs_data_get_bool(s,"show_markings");
  obs_data_set_bool(s,"show_ticks",old && face!=3);
  bool hadNumbers=face==1||face==3||face==4||face==5||face==10||face==11||face==12;
  obs_data_set_bool(s,"show_numbers",old&&hadNumbers);
 }
 bool showTicks=obs_data_get_bool(s,"show_ticks"),showNumbers=obs_data_get_bool(s,"show_numbers");
 bool transparentNeon=obs_data_get_bool(s,"neon_transparent");
 std::string background=obs_data_get_bool(s,"background_enabled")?obs_data_get_string(s,"background_image"):"";
 if(background.empty() && face==9)background=obs_data_get_string(s,"custom_face");
 bool custom=!background.empty();c.customBackground=custom;
 const char *markColorKeys[]={"tick_color","number_color"};
 for(int i=0;i<2;++i){
  if(!obs_data_has_user_value(s,markColorKeys[i]) && custom && obs_data_has_user_value(s,"markings_color"))
   obs_data_set_int(s,markColorKeys[i],obs_data_get_int(s,"markings_color"));
  c.markingColors[i]=uint32_t(obs_data_get_int(s,markColorKeys[i]));
  c.markingOverrides[i]=c.recolor;
 }
 c.neonFace=face==13 && showTicks;
 const int hands=std::clamp(int(obs_data_get_int(s,"hands")),0,11);c.neonHands=hands==11;
 const char *parts[]={"dial","hour","minute","second"};
 for(int i=0;i<4;++i) {
  std::string path;
  if(i==0 && custom)path=background;
  else if(i==0)path=asset("artwork/base"+std::to_string(face==9?8:face)+(face==13&&transparentNeon?"t":"")+".png");
  else {path=asset("artwork/"+std::string(parts[i])+(hands?std::to_string(hands):"")+".png");}
  if(c.imagePaths[i]!=path || (i==0&&face==9)){load(c.images[i],path);c.imagePaths[i]=path;}
  std::string haloPath;
  if((i==0&&c.neonFace)||(i>0&&c.neonHands))haloPath=asset("artwork/neon_"+std::string(i==0?"face":parts[i])+"_glow.png");
  if(c.haloPaths[i]!=haloPath){
   gs_image_file_ex_free(&c.halos[i]);if(!haloPath.empty())load(c.halos[i],haloPath);c.haloPaths[i]=haloPath;
  }
 }
 for(int i=0;i<2;++i){
  bool show=i==0?showTicks:showNumbers;
  std::string path=show?asset("artwork/"+std::string(i==0?"ticks":"numbers")+std::to_string(face==9?6:face)+".png"):"";
  if(c.markingsPaths[i]!=path){
   gs_image_file_ex_free(&c.markings[i]);if(!path.empty())load(c.markings[i],path);c.markingsPaths[i]=path;
  }
 }
 if(!c.composite)c.composite=gs_texrender_create(GS_RGBA,GS_ZS_NONE);
 if(!c.tint) {auto path=asset("tint.effect"); c.tint=gs_effect_create_from_file(path.c_str(),nullptr);}
 obs_leave_graphics();
}
void *create(obs_data_t *s,obs_source_t*) {auto *c=new Clock; update(c,s);return c;}
bool is_text_source(obs_source_t *source) {
 if(!source)return false;
 const char *id=obs_source_get_unversioned_id(source);
 return id && (std::string(id)=="text_gdiplus"||std::string(id)=="text_ft2_source");
}
void clear_external_output(Clock &c) {
 if(c.outputUuid.empty())return;
 auto *target=obs_get_source_by_uuid(c.outputUuid.c_str());
 if(is_text_source(target)){
  auto *settings=obs_source_get_settings(target);
  bool ours=c.lastOutputText==obs_data_get_string(settings,"text");obs_data_release(settings);
  if(ours){auto *change=obs_data_create();obs_data_set_string(change,"text","");obs_source_update(target,change);obs_data_release(change);}
 }
 obs_source_release(target);c.outputUuid.clear();c.lastOutputText.clear();
}
void tick(void *ptr,float) {
 auto &c=*static_cast<Clock*>(ptr);
 auto now=std::chrono::system_clock::now();auto whole=std::chrono::floor<std::chrono::seconds>(now);
 auto time=std::chrono::system_clock::to_time_t(whole);
 bool enabled=false,utc=false;int end=-1;std::string uuid,message;
 const ClockZoneInfo *zone=nullptr;
 {
  std::lock_guard<std::mutex> lock(c.mutex);
  if(c.lastTickSecond==time && c.lastTickRevision==c.settingsRevision)return;
  c.lastTickSecond=time;c.lastTickRevision=c.settingsRevision;
  enabled=c.countdown&&c.externalMessage;uuid=c.textSourceUuid;message=c.countdownMessage;
  zone=c.zone;utc=c.utc;end=c.endTime;
 }
 // OBS source lookups and updates run without the clock's graphics/settings lock.
 if(!c.outputUuid.empty() && (!enabled || uuid!=c.outputUuid))clear_external_output(c);
 if(!enabled||uuid.empty())return;
 auto local=zoned_time(time,zone,utc);
 double seconds=local.tm_sec+std::chrono::duration<double>(now-whole).count();
 std::string text=clock_countdown_label(message,clock_countdown_seconds(local.tm_hour,local.tm_min,seconds,end));
 auto *target=obs_get_source_by_uuid(uuid.c_str());
 if(is_text_source(target)){
  auto *settings=obs_source_get_settings(target);bool changed=text!=obs_data_get_string(settings,"text");obs_data_release(settings);
  if(changed){auto *change=obs_data_create();obs_data_set_string(change,"text",text.c_str());obs_source_update(target,change);obs_data_release(change);}
  c.outputUuid=uuid;c.lastOutputText=text;
 }
 obs_source_release(target);
}
void destroy(void *ptr) {
 auto *c=static_cast<Clock*>(ptr);clear_external_output(*c);obs_enter_graphics();
 for(auto &im:c->images)gs_image_file_ex_free(&im);
 for(auto &im:c->halos)gs_image_file_ex_free(&im);
 for(auto &im:c->markings)gs_image_file_ex_free(&im);
 gs_texture_destroy(c->text);
 gs_texrender_destroy(c->composite);
 gs_effect_destroy(c->tint);obs_leave_graphics();delete c;
}
uint32_t size(void*) { return 500; }
uint32_t height(void *ptr) {return static_cast<Clock*>(ptr)->height.load();}
void render(void *ptr,gs_effect_t*) {
 auto &c=*static_cast<Clock*>(ptr);std::lock_guard<std::mutex> lock(c.mutex);
 auto now=std::chrono::system_clock::now();auto whole=std::chrono::floor<std::chrono::seconds>(now);
 auto t=std::chrono::system_clock::to_time_t(whole);
 auto local=zoned_time(t,c.zone,c.utc);
 double second=local.tm_sec+std::chrono::duration<double>(now-whole).count();
 if(c.digital||c.date||(c.countdown&&!c.externalMessage)){
  std::vector<ClockTextLine> lines;std::string key;
  if(c.digital)lines.push_back({clock_time_label(local,c.format24,c.digitalSeconds),34,600,c.textColor});
  if(c.date)lines.push_back({clock_date_label(local,c.isoDate),23,400,c.textColor});
  if(c.countdown&&!c.externalMessage)lines.push_back({clock_countdown_label(c.countdownMessage,clock_countdown_seconds(local.tm_hour,local.tm_min,second,c.endTime)),24,600,c.countdownColor});
  for(const auto &line:lines)key+=std::to_string(line.text.size())+":"+line.text+":"+std::to_string(line.color)+"|";
  if(key!=c.lastText){
   gs_texture_destroy(c.text);c.text=clock_text_texture(lines);c.lastText=key;
  }
 }
 auto a=clock_angles(local.tm_hour,local.tm_min,second,c.movement);
 double angles[]={0,a.hour,a.minute,a.second};
 if(!c.composite || c.opacity<=0.f)return;
 gs_texrender_reset(c.composite);
 const uint32_t canvasHeight=c.height.load();
 if(!gs_texrender_begin(c.composite,500,canvasHeight))return;
 vec4 clear{};gs_clear(GS_CLEAR_COLOR,&clear,0.f,0);
 gs_ortho(0.f,500.f,0.f,float(canvasHeight),-100.f,100.f);
 gs_blend_state_push();gs_reset_blend_state();gs_blend_function_separate(GS_BLEND_SRCALPHA,GS_BLEND_INVSRCALPHA,GS_BLEND_ONE,GS_BLEND_INVSRCALPHA);
 for(int i=0;i<4;++i) {
  if((i==3&&!c.seconds)||!c.images[i].texture)continue;
  auto *effect=c.tint?c.tint:obs_get_base_effect(OBS_EFFECT_DEFAULT);
  if(c.tint) {
   uint32_t col=c.recolor?c.colors[i]:0xffffffff; vec4 color;
   color.x=(col&255)/255.f;color.y=((col>>8)&255)/255.f;color.z=((col>>16)&255)/255.f;color.w=1.f;
   gs_effect_set_vec4(gs_effect_get_param_by_name(effect,"tint"),&color);
   gs_effect_set_float(gs_effect_get_param_by_name(effect,"opacity"),1.f);
   gs_effect_set_bool(gs_effect_get_param_by_name(effect,"recolor"),c.recolor);
   gs_effect_set_bool(gs_effect_get_param_by_name(effect,"face"),i==0);
   gs_effect_set_bool(gs_effect_get_param_by_name(effect,"premultiplied"),false);
  }
  gs_matrix_push();gs_matrix_translate3f(250,250,0);gs_matrix_rotaa4f(0,0,1,float(angles[i])*pi/180);gs_matrix_translate3f(-250,-250,0);
  if(i==0)while(gs_effect_loop(effect,"Draw"))obs_source_draw(c.images[i].texture,0,0,500,500,false);
  if(c.halos[i].texture && c.glow>0.f && c.tint){
   if(i==0 && c.markingOverrides[0]){
    uint32_t col=c.markingColors[0];vec4 color;
    color.x=(col&255)/255.f;color.y=((col>>8)&255)/255.f;color.z=((col>>16)&255)/255.f;color.w=1.f;
    gs_effect_set_vec4(gs_effect_get_param_by_name(effect,"tint"),&color);
    gs_effect_set_bool(gs_effect_get_param_by_name(effect,"recolor"),true);
    gs_effect_set_bool(gs_effect_get_param_by_name(effect,"face"),false);
   }
   gs_effect_set_float(gs_effect_get_param_by_name(effect,"opacity"),c.glow);
   while(gs_effect_loop(effect,"Draw"))obs_source_draw(c.halos[i].texture,0,0,500,500,false);
   gs_effect_set_float(gs_effect_get_param_by_name(effect,"opacity"),1.f);
  }
  if(i==0)for(int layer=0;layer<2;++layer){
   auto &marking=c.markings[layer];
   if(!marking.texture)continue;
   if(c.tint){
    vec4 color;uint32_t col=c.markingColors[layer];
    color.x=(col&255)/255.f;color.y=((col>>8)&255)/255.f;color.z=((col>>16)&255)/255.f;color.w=1.f;
    gs_effect_set_vec4(gs_effect_get_param_by_name(effect,"tint"),&color);
    gs_effect_set_bool(gs_effect_get_param_by_name(effect,"recolor"),c.markingOverrides[layer]);
    gs_effect_set_bool(gs_effect_get_param_by_name(effect,"face"),false);
   }
   while(gs_effect_loop(effect,"Draw"))obs_source_draw(marking.texture,0,0,500,500,false);
  }
  if(i!=0)while(gs_effect_loop(effect,"Draw"))obs_source_draw(c.images[i].texture,0,0,500,500,false);
  gs_matrix_pop();
 }
 if((c.digital||c.date||(c.countdown&&!c.externalMessage))&&c.text){
  auto *textEffect=c.tint?c.tint:obs_get_base_effect(OBS_EFFECT_DEFAULT);
  if(c.tint){gs_effect_set_bool(gs_effect_get_param_by_name(textEffect,"recolor"),false);gs_effect_set_float(gs_effect_get_param_by_name(textEffect,"opacity"),1.f);}
  while(gs_effect_loop(textEffect,"Draw"))obs_source_draw(c.text,0,500,500,canvasHeight-500,false);
 }
 gs_blend_state_pop();
 gs_texrender_end(c.composite);
 auto *effect=c.tint?c.tint:obs_get_base_effect(OBS_EFFECT_DEFAULT);
 if(c.tint){
  gs_effect_set_bool(gs_effect_get_param_by_name(effect,"recolor"),false);
  gs_effect_set_float(gs_effect_get_param_by_name(effect,"opacity"),c.opacity);
  gs_effect_set_bool(gs_effect_get_param_by_name(effect,"premultiplied"),true);
 }
 gs_blend_state_push();gs_blend_function(GS_BLEND_ONE,GS_BLEND_INVSRCALPHA);
 while(gs_effect_loop(effect,"Draw"))obs_source_draw(gs_texrender_get_texture(c.composite),0,0,500,canvasHeight,false);
 gs_blend_state_pop();
}
void defaults(obs_data_t *s) {
 obs_data_set_default_bool(s,"background_enabled",false);
 obs_data_set_default_bool(s,"external_message",false);obs_data_set_default_string(s,"text_source_uuid","");
 obs_data_set_default_bool(s,"countdown",false);obs_data_set_default_string(s,"end_time","09:00");
 obs_data_set_default_string(s,"countdown_message","Service starts in");obs_data_set_default_int(s,"countdown_color",0xffffffff);
 obs_data_set_default_string(s,"background_image","");obs_data_set_default_bool(s,"show_ticks",true);obs_data_set_default_bool(s,"show_numbers",true);
 obs_data_set_default_int(s,"tick_color",0xff202020);obs_data_set_default_int(s,"number_color",0xff202020);
 obs_data_set_default_string(s,"timezone","");
 obs_data_set_default_bool(s,"digital",false);obs_data_set_default_bool(s,"date",false);
 obs_data_set_default_bool(s,"format24",false);obs_data_set_default_bool(s,"digital_seconds",true);
 obs_data_set_default_bool(s,"iso_date",false);obs_data_set_default_int(s,"text_color",0xffffffff);
 obs_data_set_default_int(s,"glow",70);obs_data_set_default_bool(s,"neon_transparent",false);
 obs_data_set_default_int(s,"opacity",100);
 obs_data_set_default_int(s,"face",6);obs_data_set_default_int(s,"hands",4);
 obs_data_set_default_int(s,"movement",2);obs_data_set_default_bool(s,"seconds",true);
 obs_data_set_default_bool(s,"recolor",false);
 obs_data_set_default_int(s,"face_color",0xffffffff);obs_data_set_default_int(s,"hour_color",0xff202020);
 obs_data_set_default_int(s,"minute_color",0xff202020);obs_data_set_default_int(s,"second_color",0xff3030e0);
}
bool refresh_properties(obs_properties_t *p,obs_property_t*,obs_data_t *s) {
 int face=int(obs_data_get_int(s,"face")),hands=int(obs_data_get_int(s,"hands"));
 bool digital=obs_data_get_bool(s,"digital"),date=obs_data_get_bool(s,"date");
 obs_property_set_visible(obs_properties_get(p,"neon_transparent"),face==13);
 obs_property_set_visible(obs_properties_get(p,"glow"),face==13||hands==11);
 obs_property_set_visible(obs_properties_get(p,"format24"),digital);
 obs_property_set_visible(obs_properties_get(p,"digital_seconds"),digital);
 obs_property_set_visible(obs_properties_get(p,"iso_date"),date);
 bool overrideColors=obs_data_get_bool(s,"recolor");
 const char *colors[]={"face_color","number_color","tick_color","hour_color","minute_color","second_color","text_color","countdown_color"};
 for(auto key:colors)obs_property_set_visible(obs_properties_get(p,key),overrideColors);
 obs_property_set_visible(obs_properties_get(p,"background_image"),obs_data_get_bool(s,"background_enabled"));
 bool countdown=obs_data_get_bool(s,"countdown");
 obs_property_set_visible(obs_properties_get(p,"end_time"),countdown);
 obs_property_set_visible(obs_properties_get(p,"countdown_message"),countdown);
 obs_property_set_visible(obs_properties_get(p,"countdown_hint"),countdown);
 obs_property_set_visible(obs_properties_get(p,"external_message"),countdown);
 obs_property_set_visible(obs_properties_get(p,"text_source_uuid"),countdown&&obs_data_get_bool(s,"external_message"));
 obs_property_set_visible(obs_properties_get(p,"external_hint"),countdown&&obs_data_get_bool(s,"external_message"));
 obs_property_set_visible(obs_properties_get(p,"countdown_color"),overrideColors&&!obs_data_get_bool(s,"external_message"));
 bool valid=clock_end_time(obs_data_get_string(s,"end_time"))>=0;
 obs_property_set_description(obs_properties_get(p,"end_time"),valid?"End time (24-hour HH:MM)":"End time (invalid — use 24-hour HH:MM)");
 return true;
}
bool add_text_source(void *param,obs_source_t *source) {
 if(is_text_source(source))obs_property_list_add_string(static_cast<obs_property_t*>(param),obs_source_get_name(source),obs_source_get_uuid(source));
 return true;
}
obs_properties_t *properties(void *ptr) {
 auto *p=obs_properties_create();
 obs_properties_add_text(p,"version_info","ClockFace 1.1.1",OBS_TEXT_INFO);
 auto *f=obs_properties_add_list(p,"face","Face",OBS_COMBO_TYPE_LIST,OBS_COMBO_FORMAT_INT);
 const char *faces[]={"Original","School","Minute ticks","No ticks","Script","Black","Broadcast","Railway","Minimal (transparent)","Custom PNG","Classic (ivory / Roman)","Studio Dark","Church Tower","Neon"};
 for(int i=0;i<14;++i)if(i!=9)obs_property_list_add_int(f,faces[i],i);
 auto *h=obs_properties_add_list(p,"hands","Hands",OBS_COMBO_TYPE_LIST,OBS_COMBO_FORMAT_INT);
 const char *hands[]={"Original","Tapered","Fancy","White","Broadcast","Railway","Minimal","Classic ornate black","Classic ornate brass","Studio white / orange","Gothic brass","Neon"};
 for(int i=0;i<12;++i)obs_property_list_add_int(h,hands[i],i);
 auto *m=obs_properties_add_list(p,"movement","Second-hand movement",OBS_COMBO_TYPE_LIST,OBS_COMBO_FORMAT_INT);
 obs_property_list_add_int(m,"One-second tick",0);obs_property_list_add_int(m,"Quarter-second tick",1);obs_property_list_add_int(m,"Smooth sweep",2);
 obs_properties_add_int_slider(p,"glow","Neon glow (%)",0,100,1);
 auto *tz=obs_properties_add_list(p,"timezone","Time zone (automatic daylight saving)",OBS_COMBO_TYPE_LIST,OBS_COMBO_FORMAT_STRING);
 obs_property_list_add_string(tz,"System local time","");obs_property_list_add_string(tz,"UTC","UTC");
 for(const auto &z:clock_zones())obs_property_list_add_string(tz,z.label.c_str(),z.key.c_str());
 obs_properties_add_int_slider(p,"opacity","Opacity (%)",0,100,1);
 obs_properties_add_bool(p,"countdown","Show message");
 obs_properties_add_text(p,"end_time","End time (24-hour HH:MM)",OBS_TEXT_DEFAULT);
 obs_properties_add_text(p,"countdown_message","Message",OBS_TEXT_DEFAULT);
 obs_properties_add_bool(p,"external_message","Send to OBS text source instead of clock");
 obs_properties_add_text(p,"countdown_hint","Message appears 1 hour prior to the selected End time and clears at End time each day. Uses the selected time zone.",OBS_TEXT_INFO);
 auto *textList=obs_properties_add_list(p,"text_source_uuid","Countdown text source",OBS_COMBO_TYPE_LIST,OBS_COMBO_FORMAT_STRING);
 obs_property_list_add_string(textList,"Select a text source…","");obs_enum_sources(add_text_source,textList);
 obs_properties_add_text(p,"external_hint","Choose a dedicated text source with Read from file off. Format that source in OBS; ClockFace changes only its text.",OBS_TEXT_INFO);
 if(ptr){
  auto &c=*static_cast<Clock*>(ptr);std::string selected;
  {std::lock_guard<std::mutex> lock(c.mutex);selected=c.textSourceUuid;}
  if(!selected.empty()){
   auto *source=obs_get_source_by_uuid(selected.c_str());
   if(!is_text_source(source))obs_property_list_add_string(textList,"Previously selected text source is unavailable",selected.c_str());
   obs_source_release(source);
  }
 }
 obs_properties_add_bool(p,"show_ticks","Show tick marks");
 obs_properties_add_bool(p,"show_numbers","Show numbers");
 obs_properties_add_bool(p,"seconds","Show second hand");
 obs_properties_add_bool(p,"digital","Show digital time below dial");
 obs_properties_add_bool(p,"date","Show date below dial");
 obs_properties_add_bool(p,"format24","Digital: 24-hour time");
 obs_properties_add_bool(p,"digital_seconds","Digital: show seconds");
 obs_properties_add_bool(p,"iso_date","Date: YYYY-MM-DD");
 obs_properties_add_bool(p,"neon_transparent","Neon: transparent face");
 obs_properties_add_bool(p,"recolor","Override artwork colors (uses alpha silhouette)");
 obs_properties_add_color(p,"face_color","Face / background tint");
 obs_properties_add_color(p,"number_color","Numbers color");
 obs_properties_add_color(p,"tick_color","Tick marks color");
 obs_properties_add_color(p,"hour_color","Hour hand color");
 obs_properties_add_color(p,"minute_color","Minute hand color");
 obs_properties_add_color(p,"second_color","Second hand color");
 obs_properties_add_color(p,"text_color","Digital time / date color");
 obs_properties_add_color(p,"countdown_color","Countdown color");
 obs_properties_add_bool(p,"background_enabled","Use background image");
 obs_properties_add_path(p,"background_image","Background image (square recommended)",OBS_PATH_FILE,"Images (*.png *.jpg *.jpeg *.bmp)",nullptr);
 const char *triggers[]={"face","hands","digital","date","background_image","background_enabled","show_ticks","show_numbers","recolor","countdown","end_time","external_message"};
 for(auto key:triggers)obs_property_set_modified_callback(obs_properties_get(p,key),refresh_properties);
 // OBS applies persisted settings after properties are created.
 (void)ptr;
 return p;
}
}
namespace {
FILE *open_asset_file(const char *path) {
#ifdef _WIN32
 int count=MultiByteToWideChar(CP_UTF8,0,path,-1,nullptr,0);
 if(count<=0)return nullptr;
 std::wstring wide(count,L'\0');
 if(!MultiByteToWideChar(CP_UTF8,0,path,-1,wide.data(),count))return nullptr;
 return _wfopen(wide.c_str(),L"wb");
#else
 return fopen(path,"wb");
#endif
}
}
bool obs_module_load(void) {
 blog(LOG_INFO,"[ClockFace] 1.1.1 initializing");
 char *folder=obs_module_config_path("artwork");
 if(!folder || os_mkdirs(folder)<0) {bfree(folder);blog(LOG_ERROR,"[ClockFace] Cannot create artwork directory");return false;} bfree(folder);
 for(const auto &asset:embedded_assets) {
  char *path=obs_module_config_path(asset.name);FILE *file=path?open_asset_file(path):nullptr;
  if(!file)blog(LOG_ERROR,"[ClockFace] Cannot write %s (errno %d)",path?path:"(null)",errno);
  bfree(path);
  if(!file) {blog(LOG_ERROR,"[ClockFace] Cannot extract embedded artwork");return false;}
  bool ok=fwrite(asset.bytes,1,asset.size,file)==asset.size; if(fclose(file)!=0)ok=false;
  if(!ok){blog(LOG_ERROR,"[ClockFace] Failed writing embedded asset %s (errno %d)",asset.name,errno);return false;}
 }
 obs_source_info info{};info.id="clockface_source";info.type=OBS_SOURCE_TYPE_INPUT;
 info.output_flags=OBS_SOURCE_VIDEO|OBS_SOURCE_CUSTOM_DRAW;info.get_name=name;info.create=create;info.destroy=destroy;
 info.get_width=size;info.get_height=height;info.get_defaults=defaults;info.get_properties=properties;info.update=update;info.video_render=render;info.video_tick=tick;
 obs_register_source(&info);blog(LOG_INFO,"[ClockFace] 1.1.1 loaded");return true;
}
