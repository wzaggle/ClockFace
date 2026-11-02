"""Generate native clock artwork; original user-supplied PNGs are untouched."""
from PIL import Image, ImageDraw, ImageFont, ImageFilter
from pathlib import Path
import math
out=Path(__file__).resolve().parents[1]/'data/artwork'
S=3
serif='/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf'
sans='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
roman=['XII','I','II','III','IV','V','VI','VII','VIII','IX','X','XI']
def blank():return Image.new('RGBA',(500*S,500*S))
def pt(angle,r):return (S*(250+math.sin(angle)*r),S*(250-math.cos(angle)*r))
def ellipse(d,box,**kw):d.ellipse(tuple(v*S for v in box),**kw)
def save(im,name):im.resize((500,500),Image.Resampling.LANCZOS).save(out/name)
def ticks(d,color,r=221,major=17,minor=7,wide=5,thin=2):
 for i in range(60):
  a=i*math.pi/30;d.line((pt(a,r-(major if i%5==0 else minor)),pt(a,r)),fill=color,width=(wide if i%5==0 else thin)*S)
def numbers(d,labels,color,font,r=175,size=30):
 f=ImageFont.truetype(font,size*S)
 for i,label in enumerate(labels):d.text(pt(i*math.pi/6,r),label,font=f,fill=color,anchor='mm',stroke_width=0)
# A railway dial is now clearly distinct from Broadcast.
im=blank();d=ImageDraw.Draw(im);ellipse(d,(6,6,494,494),fill='#ffffff',outline='#14181d',width=2*S)
ticks(d,'#10151a',r=228,major=34,minor=13,wide=11,thin=3);save(im,'dial7.png')
# Classic: Roman numerals, warm ivory, concentric brass rim.
im=blank();d=ImageDraw.Draw(im)
ellipse(d,(5,5,495,495),fill='#b79a58',outline='#584729',width=3*S)
ellipse(d,(14,14,486,486),fill='#f5ecd7',outline='#76623a',width=2*S)
ellipse(d,(25,25,475,475),outline='#b9a776',width=S)
ticks(d,'#6d5b36',r=219,major=8,minor=3,wide=2,thin=1)
numbers(d,roman,'#302a21',serif,r=180,size=29);save(im,'dial10.png')
# Studio Dark: charcoal, bright numerals, orange accent at twelve.
im=blank();d=ImageDraw.Draw(im)
ellipse(d,(6,6,494,494),fill='#20262e',outline='#778490',width=3*S)
ticks(d,'#ecf3f6',r=225,major=20,minor=6,wide=5,thin=1)
numbers(d,['12','1','2','3','4','5','6','7','8','9','10','11'],'#f1f5f8',sans,r=176,size=27)
d.line((pt(0,204),pt(0,225)),fill='#ff9138',width=6*S);save(im,'dial11.png')
# Church Tower: dark blue dial, decorative brass rim, traditional Roman numerals.
im=blank();d=ImageDraw.Draw(im)
ellipse(d,(4,4,496,496),fill='#4f401f',outline='#dfc57d',width=3*S)
ellipse(d,(12,12,488,488),fill='#b49a50',outline='#76612e',width=2*S)
ellipse(d,(23,23,477,477),fill='#152739',outline='#ecd89c',width=2*S)
ellipse(d,(35,35,465,465),outline='#8c7849',width=S)
for i in range(48):
 a=i*math.pi/24;x,y=pt(a,236);r=2*S;d.ellipse((x-r,y-r,x+r,y+r),fill='#f3df9e')
ticks(d,'#bda76e',r=211,major=9,minor=3,wide=2,thin=1)
numbers(d,roman,'#f2dfa0',serif,r=172,size=30)
for i in range(12):
 a=(i+.5)*math.pi/6;x,y=pt(a,210);r=2*S;d.polygon([(x,y-r),(x+r,y),(x,y+r),(x-r,y)],fill='#e0c67c')
save(im,'dial12.png')
# Neon core and a separate halo, allowing independent glow intensity.
core=blank();d=ImageDraw.Draw(core);ticks(d,'#61f1ff',r=219,major=23,minor=8,wide=5,thin=2)
ellipse(d,(14,14,486,486),outline='#ad83ff',width=2*S)
glow=core.filter(ImageFilter.GaussianBlur(7*S));save(glow,'neon_face_glow.png')
save(core,'dial13t.png')
bg=blank();d=ImageDraw.Draw(bg);ellipse(d,(5,5,495,495),fill='#0d1022');bg.alpha_composite(core);save(bg,'dial13.png')
# Ornate black / brass, Studio, Gothic, and Neon hands.
for style in range(7,12):
 for part,length,width in [('hour',124,13),('minute',184,9),('second',204,3)]:
  im=blank();d=ImageDraw.Draw(im)
  color={7:'#24211c',8:'#ac8435',9:('#ff9138' if part=='second' else '#f4f7fa'),10:'#efda96',11:('#ff73d8' if part=='second' else '#7af6ff')}[style]
  y=250-length;w=width
  def poly(points,**kw):d.polygon([(x*S,y*S) for x,y in points],**kw)
  if part=='second':d.line((250*S,274*S,250*S,y*S),fill=color,width=2*S)
  elif style in (7,8):
   poly([(250,y),(250-w,y+23),(247,y+43),(247,220),(250+w,237),(253,255),(250,269),(247,255),(250-w,237),(253,220),(253,y+43),(250+w,y+23)],fill=color)
   ellipse(d,(242,200,258,216),outline=color,width=3*S)
   ellipse(d,(244,259,256,271),outline=color,width=2*S)
  elif style==10:
   poly([(250,y),(241,y+25),(247,y+23),(247,208),(237,224),(246,239),(246,260),(250,270),(254,260),(254,239),(263,224),(253,208),(253,y+23),(259,y+25)],fill=color)
   ellipse(d,(245,215,255,225),fill='#152739')
  else:
   poly([(250,y),(250-w/2,y+12),(250-w/2,261),(250+w/2,261),(250+w/2,y+12)],fill=color)
  ellipse(d,(245,245,255,255),fill=color)
  save(im,f'{part}{style}.png')
  if style==11:save(im.filter(ImageFilter.GaussianBlur(6*S)),f'neon_{part}_glow.png')
