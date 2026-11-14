"""Genera le immagini della procedura guidata dell'installer (logo NINA) a partire dalle risorse del progetto.
Uso:  python installer/make_wizard_images.py     (serve Pillow)"""
import os
from PIL import Image, ImageDraw, ImageFilter

here = os.path.dirname(os.path.abspath(__file__))
assets = os.path.join(here, '..', 'Source', 'assets')
logo = Image.open(os.path.join(assets, 'nina_logo.png')).convert('RGBA')
tile = Image.open(os.path.join(assets, 'nina_icon_512.png')).convert('RGBA')


def background(w, h):
    img = Image.new('RGB', (w, h), (20, 22, 27))
    px = img.load()
    for y in range(h):
        t = y / max(1, h - 1)
        c = (int(30 - 14 * t), int(33 - 15 * t), int(40 - 17 * t))
        for x in range(w):
            px[x, y] = c
    return img


def large(scale):
    w, h = int(164 * scale), int(314 * scale)
    img = background(w, h).convert('RGBA')
    # emblema grande al centro, logo con il claim in basso
    ts = int(w * 0.78)
    t = tile.resize((ts, ts), Image.LANCZOS)
    glow = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    glow.paste(Image.new('RGBA', (ts, ts), (255, 110, 20, 90)), ((w - ts) // 2, int(h * 0.14)), t.getchannel('A'))
    glow = glow.filter(ImageFilter.GaussianBlur(w * 0.06))
    img.alpha_composite(glow)
    img.alpha_composite(t, ((w - ts) // 2, int(h * 0.14)))
    lw = int(w * 0.92)
    lh = int(lw * logo.height / logo.width)
    lg = logo.resize((lw, lh), Image.LANCZOS)
    img.alpha_composite(lg, ((w - lw) // 2, int(h * 0.14) + ts + int(h * 0.06)))
    return img.convert('RGB')


def small(scale):
    w, h = int(55 * scale), int(58 * scale)
    img = Image.new('RGB', (w, h), (255, 255, 255))                      # sfondo bianco: e' l'angolo delle pagine interne
    s = min(w, h) - 4
    t = tile.resize((s, s), Image.LANCZOS)
    img.paste(t, ((w - s) // 2, (h - s) // 2), t)
    return img


for pct, sc in (('100', 1.0), ('125', 1.25), ('150', 1.5), ('200', 2.0)):
    large(sc).save(os.path.join(here, 'wizard-large-%s.bmp' % pct))
    small(sc).save(os.path.join(here, 'wizard-small-%s.bmp' % pct))
print('ok')
