"""Generate monochrome glass badges with technology marks and Inter outlines.
Install fonttools brotli uharfbuzz; run with --root <repository>.
These are documentation build tools, never firmware dependencies.
"""
from pathlib import Path
from functools import lru_cache
import argparse, io, json, html
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont
from fontTools.pens.svgPathPen import SVGPathPen
import uharfbuzz as hb

parser = argparse.ArgumentParser()
parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
ROOT = parser.parse_args().root.resolve()
A = ROOT / 'docs/assets'
SOURCE = A / 'sources/lucide'
WHITE, SURFACE = '#f5f5f5', '#111111'

@lru_cache(None)
def face(weight):
    variable = TTFont(ROOT / 'GatewayNode/web/inter-latin.woff2')
    font = instantiateVariableFont(variable, {'wght': weight}, inplace=False)
    font.flavor = None
    data = io.BytesIO()
    font.save(data)
    shaped = hb.Font(hb.Face(data.getvalue()))
    upem = font['head'].unitsPerEm
    shaped.scale = (upem, upem)
    hb.ot_font_set_funcs(shaped)
    return font, font.getGlyphSet(), shaped, upem

def shape(value, size, weight=600):
    font, glyphs, shaped, upem = face(weight)
    buf = hb.Buffer()
    buf.add_str(value)
    buf.guess_segment_properties()
    hb.shape(shaped, buf)
    return font, glyphs, buf.glyph_infos, buf.glyph_positions, size / upem

def measure(value, size, weight=600):
    *_, positions, scale = shape(value, size, weight)
    return sum(p.x_advance for p in positions) * scale

def text(value, x, baseline, size, color=WHITE, weight=600):
    font, glyphs, infos, positions, scale = shape(value, size, weight)
    result, cursor = [], 0
    for info, pos in zip(infos, positions):
        pen = SVGPathPen(glyphs)
        glyphs[font.getGlyphName(info.codepoint)].draw(pen)
        path = pen.getCommands()
        if path:
            result.append(f'<path transform="translate({x+(cursor+pos.x_offset)*scale:.3f} {baseline-pos.y_offset*scale:.3f}) scale({scale:.6f} {-scale:.6f})" d="{path}"/>')
        cursor += pos.x_advance
    return f'<g fill="{color}">' + ''.join(result) + '</g>'

def icon(name, x, y, size, color, stroke=1.85):
    source = (SOURCE / (name + '.svg')).read_text(encoding='utf-8')
    inside = source[source.index('>')+1:source.rindex('</svg>')]
    return f'<g transform="translate({x} {y}) scale({size/24:.6f})" fill="none" stroke="{color}" stroke-width="{stroke}" stroke-linecap="round" stroke-linejoin="round">{inside}</g>'

def svg(width, height, label, content):
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" role="img" aria-label="{html.escape(label,quote=True)}"><title>{html.escape(label)}</title>{content}</svg>\n'

def brand(name,x,y,size):
    source=(A/'sources/simple-icons'/(name+'.svg')).read_text(encoding='utf-8')
    inside=source[source.index('>')+1:source.rindex('</svg>')]
    return f'<g transform="translate({x} {y}) scale({size/24:.6f})" fill="#f3f3f3" stroke="none">{inside}</g>'

BADGES = [
    ('esp32','ESP32','Classic','espressif'),
    ('esp32-s3','ESP32-S3','Gateway','espressif'),
    ('esp-idf','ESP-IDF','6.1','espressif'),
    ('platformio','PlatformIO','Build','platformio'),
    ('ble-mesh','Bluetooth','Mesh','bluetooth'),
    ('wifi','Wi-Fi','2.4 GHz','wifi'),
    ('firmware','Firmware','C / C++','cplusplus'),
    ('web','Dashboard','HTML / CSS / JS','web-stack'),
    ('builds','Recorded builds','3 passed','wrench'),
    ('browser-tests','Browser tests','24 passed','flask-conical'),
    ('native-checks','Native checks','3 passed','shield-check'),
    ('hardware','Core hardware','Verified','circle-check'),
]
BRAND_NAMES={'espressif','platformio','bluetooth','cplusplus'}

def badge(slug,label,value,symbol):
    width=round(max(measure(label,11.5,500),measure(value,12.7))+58)
    content=f'''<defs>
      <linearGradient id="glass" x1="0" y1="0" x2=".25" y2="1"><stop stop-color="#373737"/><stop offset=".42" stop-color="#1b1b1b"/><stop offset="1" stop-color="#0b0b0b"/></linearGradient>
      <linearGradient id="rim" x1="0" y1="0" x2=".5" y2="1"><stop stop-color="#f8f8f8" stop-opacity=".85"/><stop offset=".48" stop-color="#929292" stop-opacity=".28"/><stop offset="1" stop-color="#eeeeee" stop-opacity=".58"/></linearGradient>
      <linearGradient id="sheen"><stop stop-color="white" stop-opacity=".14"/><stop offset="1" stop-color="white" stop-opacity="0"/></linearGradient>
      <filter id="glow" x="-35%" y="-70%" width="170%" height="240%"><feGaussianBlur stdDeviation="2.4"/></filter>
      <clipPath id="clip"><rect x="4.5" y="4.5" width="{width-9}" height="46" rx="10"/></clipPath>
    </defs>
    <rect x="5" y="8" width="{width-10}" height="40" rx="10" fill="none" stroke="white" stroke-opacity=".28" stroke-width="2" filter="url(#glow)"/>
    <rect x="4.5" y="4.5" width="{width-9}" height="46" rx="10" fill="url(#glass)" stroke="url(#rim)"/>
    <g clip-path="url(#clip)"><path d="M0 5H{width}V13C{width*.58:.1f} 0 {width*.4:.1f} 40 0 31Z" fill="url(#sheen)"/></g>
    <path d="M16 6H{width-16}" stroke="white" stroke-opacity=".23"/>
    '''
    if symbol in BRAND_NAMES:content+=brand(symbol,14,18,20)
    elif symbol=='web-stack':
        content+=brand('html5',12,11,13)+brand('css',27,11,13)+brand('javascript',19,28,13)
    else:content+=icon(symbol,14,18,20,'#eeeeee')
    content+=text(label,44,22,11.5,'#c9c9c9',500)
    content+=text(value,44,40,12.7,'#fafafa')
    return svg(width,56,label+': '+value,content)

SECTIONS=[('overview','house'),('architecture','network'),('hardware','cpu'),
    ('setup','terminal'),('verification','shield-check'),('records','camera'),
    ('troubleshooting','wrench'),('references','book-open')]
(A/'badges').mkdir(parents=True,exist_ok=True)
(A/'icons').mkdir(parents=True,exist_ok=True)
for slug,label,value,symbol in BADGES:
    (A/'badges'/f'{slug}.svg').write_text(badge(slug,label,value,symbol),encoding='utf-8')
for slug,symbol in SECTIONS:
    (A/'icons'/f'{slug}.svg').write_text(svg(28,28,symbol,icon(symbol,2,2,24,'#737373')),encoding='utf-8')
manifest={'hero':'readme-banner.png, original approved monochrome artwork, preserved without edits',
    'font':'Actual Inter outlines in badge lettering; GitHub controls README body typography',
    'badge_count':len(BADGES),'section_icons':len(SECTIONS),
    'finish':'Static grayscale glass surfaces, silver rim, reflection, and soft white halo',
    'logos':'Simple Icons technology marks; Lucide for generic Wi-Fi, section, and verification symbols',
    'badges':[{'file':slug+'.svg','label':label,'value':value,'icon':symbol} for slug,label,value,symbol in BADGES]}
(A/'graphics-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print('Built 12 monochrome glass badges with technology logos and 8 neutral section icons; original banner preserved.')
