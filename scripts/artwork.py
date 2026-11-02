from PIL import Image, ImageDraw
from pathlib import Path
import math
out=Path(__file__).resolve().parents[1]/'data/artwork'
S=3
for style in range(6,9):
 im=Image.new('RGBA',(1500,1500)); d=ImageDraw.Draw(im)
 if style!=8:d.ellipse((18,18,1482,1482),fill='#faf9f5',outline='#252b33',width=12)
 for k in range(60):
  if style==8 and k%5:continue
  angle=k*math.pi/30; r=222; length=19 if k%5==0 else 7
  p=lambda radius:(750+math.sin(angle)*radius*S,750-math.cos(angle)*radius*S)
  d.line((p(r-length),p(r)),fill='#252b33',width=(6 if k%5==0 else 2)*S)
 im.resize((500,500),Image.Resampling.LANCZOS).save(out/f'dial{style}.png')
for style in range(4,7):
 for part,length,width,color in [('hour',125,13,'#252b33'),('minute',187,9,'#252b33'),('second',204,3,'#dc3030')]:
  im=Image.new('RGBA',(1500,1500));d=ImageDraw.Draw(im);w=width if style!=6 else max(2,width//3)
  if style==4 and part!='second':
   d.polygon([(750-w*S/2,780),(750-w*S/2,750-length*S+30),(750,750-length*S),(750+w*S/2,750-length*S+30),(750+w*S/2,780)],fill=color)
  else:d.line((750,795,750,750-length*S),fill=color,width=w*S)
  if style==5 and part=='second':d.ellipse((732,750-160*S-18,768,750-160*S+18),fill=color)
  d.ellipse((738,738,762,762),fill=color)
  im.resize((500,500),Image.Resampling.LANCZOS).save(out/f'{part}{style}.png')
