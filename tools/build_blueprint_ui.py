"""Build the blueprint menu atlas (Pillow, Windows Agency/Consolas fonts).
Run from any working directory. Generated PNGs are committed; fonts and Python
are not runtime dependencies. Artwork is deterministic, not an AI screenshot.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import math
ROOT=Path(__file__).resolve().parents[1]
C=(159,239,232,255); D=(54,112,115,255); BLACK=(0,0,0,255)

def text(im,xy,label,size,fill=C,italic=False,max_width=None):
    font=ImageFont.truetype(r'C:/Windows/Fonts/AGENCYB.TTF' if italic else r'C:/Windows/Fonts/consola.ttf',size*4)
    box=font.getbbox(label); layer=Image.new('RGBA',(box[2]+size*4,box[3]-box[1]+8),(0,0,0,0))
    ImageDraw.Draw(layer).text((2,-box[1]),label,font=font,fill=fill)
    if italic:
        layer=layer.transform((layer.width+layer.height//5,layer.height),Image.Transform.AFFINE,(1,.18,-layer.height*.18,0,1,0),Image.Resampling.BICUBIC)
    width=layer.width//4
    if max_width: width=min(width,max_width)
    layer=layer.resize((width,layer.height//4),Image.Resampling.LANCZOS)
    im.alpha_composite(layer,xy)

def panel(draw,x,y,w,h,fill=BLACK,edge=C,c=10):
    p=[(x+c,y),(x+w-c,y),(x+w,y+c),(x+w,y+h),(x+c,y+h),(x,y+h-c),(x,y+c),(x+c,y)]
    draw.polygon(p,fill=fill);draw.line(p,fill=edge,width=1)

shell=Image.new('RGBA',(512,512),(0,0,0,0));d=ImageDraw.Draw(shell)
text(shell,(18,25),'DROIDCAMPOS',42,italic=True,max_width=235)
d.line([(16,71),(200,71),(206,77),(252,77)],fill=C)
text(shell,(20,81),'ETIP Corporation.',10)
panel(d,16,108,236,282)
d.line([(20,119),(20,369),(33,384),(79,384),(85,390)],fill=D)
text(shell,(32,119),'COMMAND TERMINAL',10)
d.line((26,137,242,137),fill=D)
for x in range(140,242,6): d.line((x,110,x-10,120),fill=D)
for x in range(36,230,6): d.line((x,378,x-8,386),fill=D)
for x in (42,48,54,230,236): d.rectangle((x,382,x+2,384),fill=C)
# Open black viewport; only registration marks and a technical base.
for x,s in [(274,1),(492,-1)]:
    for y,t in [(26,1),(344,-1)]: d.line([(x+s*9,y),(x,y),(x,y+t*9)],fill=C)
    d.line((x,112,x,300),fill=D)
    for y in (112,174,238,300): d.line((x-2,y,x+2,y),fill=D)
text(shell,(345,205),'3D VIEWPORT',9,fill=D)
text(shell,(369,224),'DC-01',9,fill=D)
d.ellipse((286,346,480,370),outline=D);d.ellipse((305,350,461,366),outline=D)
d.line((383,343,383,374),fill=D)
d.line((290,380,476,380),fill=D)
for x in (290,476):d.line((x,377,x,383),fill=D)
# Footer dedicated to progressive sync, without a baked-in percentage.
d.line([(16,406),(258,406),(264,400),(496,400)],fill=D)
panel(d,318,404,178,30,edge=D,c=5)
text(shell,(325,409),'UNIT DC-01',8)
text(shell,(325,422),'NEURAL SYNC',8)
# Atlas: 3 rows x 2 states. Transparent padded power-of-two textures.
buttons=Image.new('RGBA',(512,256),(0,0,0,0));bd=ImageDraw.Draw(buttons)
for i,label in enumerate(('NEW GAME','LOAD GAME','SETTINGS')):
    for selected in (False,True):
        x=256 if selected else 0;y=i*64
        panel(bd,x+2,y+2,212,52,fill=C if selected else BLACK,edge=C if selected else D,c=9)
        ink=BLACK if selected else C
        text(buttons,(x+10,y+19),f'0{i+1}',11,ink)
        bd.line((x+32,y+15,x+32,y+40),fill=ink)
        text(buttons,(x+43,y+12),label,27,ink,True,max_width=157)
        for z in range(3):bd.line((x+190+z*4,y+43,x+194+z*4,y+39),fill=ink)
for im,name in [(shell,'ui_blueprint.png'),(buttons,'ui_menu_buttons.png')]:
    im.quantize(colors=256, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.NONE).save(ROOT/'game/res'/name, bits=8)
