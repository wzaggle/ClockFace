"""Create independent tick and numeral layers for every selectable face."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import math
import numpy as np
from scipy import ndimage
root=Path(__file__).resolve().parents[1]/'data/artwork';S=3
styles=list(range(9))+list(range(10,14))
# Extract original raster glyphs without changing the original files.
thresholds={0:175,1:231,2:175,3:240,4:241,5:211}
colors={0:'#000000',1:'#000000',2:'#000000',3:'#000000',4:'#000000',5:'#ffffff',6:'#252b33',7:'#10151a',8:'#252b33',10:'#6d5b36',11:'#ecf3f6',12:'#bda76e',13:'#61f1ff'}
font='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
serif='/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf'
romans=['XII','I','II','III','IV','V','VI','VII','VIII','IX','X','XI']
def point(i,r):a=i*math.pi/30;return ((250+math.sin(a)*r)*S,(250-math.cos(a)*r)*S)
def make_ticks(style):
 im=Image.new('RGBA',(1500,1500));d=ImageDraw.Draw(im)
 if style in (8,):limit=range(0,60,5)
 else:limit=range(60)
 r={6:222,7:228,8:222,10:219,11:225,12:211,13:219}.get(style,222)
 major={7:34,10:8,12:9,13:23}.get(style,19);minor={7:13,10:3,12:3,13:8}.get(style,7)
 for k in limit:
  width={7:11,10:2,12:2,13:5}.get(style,6) if k%5==0 else {7:3,10:1,11:1,12:1}.get(style,2)
  d.line((point(k,r-(major if k%5==0 else minor)),point(k,r)),fill=colors[style],width=width*S)
 if style==11:d.line((point(0,204),point(0,225)),fill='#ff9138',width=6*S)
 if style==13:d.ellipse((14*S,14*S,486*S,486*S),outline='#ad83ff',width=2*S)
 return im.resize((500,500),Image.Resampling.LANCZOS)
def make_numbers(style):
 im=Image.new('RGBA',(1500,1500));d=ImageDraw.Draw(im)
 roman=style in (10,12);labels=romans if roman else ['12']+list(map(str,range(1,12)))
 size=29 if roman else 27;r=180 if style==10 else 172 if style==12 else 176
 color='#302a21' if style==10 else '#f2dfa0' if style==12 else '#f1f5f8' if style==11 else colors[style]
 f=ImageFont.truetype(serif if roman else font,size*S)
 for i,label in enumerate(labels):d.text(point(i*5,r),label,fill=color,font=f,anchor='mm')
 return im.resize((500,500),Image.Resampling.LANCZOS)
for n in styles:
 if n<=5:
  marks=Image.open(root/f'markings{n}.png').convert('RGBA');ticks=Image.new('RGBA',(500,500));numbers=ticks.copy()
  data=np.array(marks);alpha=data[:,:,3];labels,count=ndimage.label(alpha>8)
  nearest=ndimage.distance_transform_edt(labels==0,return_distances=False,return_indices=True)
  expanded=labels[tuple(nearest)]
  isTick=np.zeros(count+1,dtype=bool)
  yy,xx=np.indices(alpha.shape);radius=np.hypot(xx-250,yy-250)
  for component in range(1,count+1):
   meanRadius=radius[labels==component].mean()
   isTick[component]=meanRadius<thresholds[n] if n==5 else meanRadius>=thresholds[n]
  tickData=data.copy();tickData[~isTick[expanded]]=0
  numberData=data.copy();numberData[isTick[expanded]]=0
  ticks=Image.fromarray(tickData);numbers=Image.fromarray(numberData)
  if n in (0,2):ticks=marks;numbers=make_numbers(n)
  if n==3:ticks=make_ticks(n);numbers=marks
 else:ticks=make_ticks(n);numbers=make_numbers(n)
 ticks.save(root/f'ticks{n}.png');numbers.save(root/f'numbers{n}.png')
