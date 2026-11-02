"""Optional blank dials and transparent tick/number overlays for image backgrounds."""
from pathlib import Path
from PIL import Image, ImageDraw
import math
root=Path(__file__).resolve().parents[1]/'data/artwork'
# Background fill, marking color, extraction radius.
styles={0:('#ffffff','#000000',242),1:('#ffffff','#000000',243),2:('#ffffff','#000000',244),3:('#ffffff','#000000',243),4:('#ffffff','#000000',242),5:('#000000','#ffffff',240),6:('#faf9f5','#252b33',230),7:('#ffffff','#10151a',232),10:('#f5ecd7','#302a21',224),11:('#20262e','#f1f5f8',230),12:('#152739','#f2dfa0',216)}
for style in list(styles)+[8,13]:
 src=Image.open(root/('dial'+(str(style) if style else '')+'.png')).convert('RGBA')
 if style==8:
  blank=Image.new('RGBA',src.size);marks=src.copy()
 elif style==13:
  blank=Image.new('RGBA',src.size);d=ImageDraw.Draw(blank);d.ellipse((5,5,495,495),fill='#0d1022')
  marks=Image.open(root/'dial13t.png').convert('RGBA')
 else:
  bg,fg,radius=styles[style]
  bgRGB=tuple(bytes.fromhex(bg[1:]));fgRGB=tuple(bytes.fromhex(fg[1:]))
  # Preserve decorative outer rims for newer dials; blank their entire interior.
  if style>=6:
   blank=src.copy();d=ImageDraw.Draw(blank);r={6:239,7:240,10:224,11:239,12:222}[style]
   d.ellipse((250-r,250-r,250+r,250+r),fill=bg)
  else:
   blank=Image.new('RGBA',src.size);d=ImageDraw.Draw(blank)
   d.ellipse((1,1,498,498),fill=bg,outline='#151a1f' if style==0 else '#999999',width=1)
  marks=Image.new('RGBA',src.size);pixels=src.load();dest=marks.load()
  for y in range(500):
   for x in range(500):
    r=math.hypot(x-250,y-250)
    if r>radius or r<90:continue
    pix=pixels[x,y]
    # Invert the known solid fill, preserving antialiased edges as alpha.
    dif=max(abs(pix[k]-bgRGB[k]) for k in range(3));den=max(abs(fgRGB[k]-bgRGB[k]) for k in range(3))
    a=min(255,round(dif*255/max(1,den)))*pix[3]//255
    if a:dest[x,y]=(*fgRGB,a)
 blank.save(root/f'base{style}.png');marks.save(root/f'markings{style}.png')
Image.new('RGBA',(500,500)).save(root/'base13t.png')
