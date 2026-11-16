# Run in the editor (execute_python over the MCP, or the Python console) to rebuild M_KPCap.
# Builds /Game/KillingPart/Materials/M_KPCap: the inside of a cut plushie.
# r = |LocalPos / Radii| is 1 on the body surface, so the cap rim is r = 1 for any cut.
# Bands from the rim inwards: fabric skin (SkinColor), stuffing, then wet dark red with blotches
# (Director's cut) or lumpy white stuffing (Theatrical = 1).
mel = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
PATH = '/Game/KillingPart/Materials/M_KPCap'
log = []

if unreal.EditorAssetLibrary.does_asset_exist(PATH):
    m = unreal.load_asset(PATH)
    mel.delete_all_material_expressions(m)
else:
    m = at.create_asset('M_KPCap', '/Game/KillingPart/Materials', unreal.Material, unreal.MaterialFactoryNew())

X = [-2400]
def node(cls, x, y, **props):
    e = mel.create_material_expression(m, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e

def link(a, aout, b, bin_):
    ok = mel.connect_material_expressions(a, aout, b, bin_)
    if not ok:
        log.append('LINK FAIL %s.%s -> %s.%s' % (type(a).__name__, aout, type(b).__name__, bin_))
    return ok

def scalar(name, val, x, y):
    return node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=val)

def vec(name, col, x, y):
    return node(unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name, default_value=col)

def const3(r, g, b, x, y):
    return node(unreal.MaterialExpressionConstant3Vector, x, y, constant=unreal.LinearColor(r=r, g=g, b=b, a=1.0))

def const(v, x, y):
    return node(unreal.MaterialExpressionConstant, x, y, r=v)

def op(cls, a, b, x, y, aout='', bout=''):
    e = node(cls, x, y)
    link(a, aout, e, 'A')
    link(b, bout, e, 'B')
    return e

def lerp(a, b, alpha, x, y):
    e = node(unreal.MaterialExpressionLinearInterpolate, x, y)
    link(a, '', e, 'A')
    link(b, '', e, 'B')
    link(alpha, '', e, 'Alpha')
    return e

def sat(a, x, y):
    e = node(unreal.MaterialExpressionSaturate, x, y)
    link(a, '', e, '')
    return e

# --- parameters
radii = vec('Radii', unreal.LinearColor(r=50, g=50, b=46, a=0), -2400, -400)
skin = vec('SkinColor', unreal.LinearColor(r=0.9, g=0.8, b=0.7, a=1), -2400, -250)
theat = scalar('Theatrical', 0.0, -2400, -100)
skinw = scalar('SkinWidth', 0.07, -2400, 0)
stuffw = scalar('StuffWidth', 0.08, -2400, 100)
glow = scalar('StuffGlow', 0.35, -2400, 200)

# --- local position and r
wp = node(unreal.MaterialExpressionWorldPosition, -2400, 400)
tp = node(unreal.MaterialExpressionTransformPosition, -2200, 400,
          transform_source_type=unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD,
          transform_type=unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
link(wp, '', tp, '')
div = op(unreal.MaterialExpressionDivide, tp, radii, -2000, 300, bout='')
mask3 = node(unreal.MaterialExpressionComponentMask, -1850, 300, r=True, g=True, b=True, a=False)
link(div, '', mask3, '')
r = node(unreal.MaterialExpressionLength, -1700, 300)
link(mask3, '', r, '')

# --- noise: blotches (coarse) and edge wobble / lumps (fine), in local units
def noise(scale, x, y, levels=2):
    n = node(unreal.MaterialExpressionNoise, x, y, scale=scale, levels=levels, output_min=0.0, output_max=1.0, quality=1)
    link(tp, '', n, 'World Position')
    return n
n_big = noise(0.022, -2000, 600, 2)
n_lump = noise(0.07, -2000, 1000, 3)
n_fine = noise(0.16, -2000, 800, 2)

# r with a little wobble so the bands aren't perfect circles
wob = op(unreal.MaterialExpressionSubtract, n_fine, const(0.5, -1850, 900), -1700, 800)
wob2 = op(unreal.MaterialExpressionMultiply, wob, const(0.035, -1700, 950), -1550, 800)
rw = op(unreal.MaterialExpressionAdd, r, wob2, -1400, 400)

# masks: skin = rw > 1 - SkinWidth ; stuff = rw > 1 - SkinWidth - StuffWidth  (soft over 0.012)
one = const(1.0, -1700, 100)
t_skin = op(unreal.MaterialExpressionSubtract, one, skinw, -1550, 50)
t_stuff = op(unreal.MaterialExpressionSubtract, t_skin, stuffw, -1400, 100)
soft = const(83.0, -1400, 200)  # 1 / 0.012
d_skin = op(unreal.MaterialExpressionSubtract, rw, t_skin, -1250, 300)
m_skin = sat(op(unreal.MaterialExpressionMultiply, d_skin, soft, -1100, 300), -950, 300)
d_stuff = op(unreal.MaterialExpressionSubtract, rw, t_stuff, -1250, 450)
m_stuff = sat(op(unreal.MaterialExpressionMultiply, d_stuff, soft, -1100, 450), -950, 450)

# --- Director's inside: wet red with darker blotches
blot = sat(op(unreal.MaterialExpressionMultiply,
              op(unreal.MaterialExpressionSubtract, n_big, const(0.52, -1850, 700), -1700, 650),
              const(3.0, -1700, 700), -1550, 650), -1400, 650)
red = const3(0.55, 0.012, 0.03, -1400, 750)
dark = const3(0.17, 0.003, 0.01, -1400, 850)
inner_d = lerp(red, dark, blot, -1200, 750)
stuff_d = const3(0.92, 0.66, 0.70, -1200, 900)

# --- Theatrical inside: lumpy white stuffing (fine noise shades it a little)
lump = op(unreal.MaterialExpressionMultiply, n_lump, const(0.34, -1400, 1050), -1250, 1000)
white = op(unreal.MaterialExpressionAdd, const(0.62, -1250, 1100), lump, -1100, 1000)
inner_t = op(unreal.MaterialExpressionMultiply, white, const3(1.0, 0.985, 0.96, -1100, 1100), -950, 1000)
stuff_t = const3(0.95, 0.94, 0.92, -950, 1150)

inner = lerp(inner_d, inner_t, theat, -800, 800)
stuff = lerp(stuff_d, stuff_t, theat, -800, 950)
base0 = lerp(inner, stuff, m_stuff, -600, 800)
base = lerp(base0, skin, m_skin, -400, 700)
mel.connect_material_property(base, '', unreal.MaterialProperty.MP_BASE_COLOR)

# roughness: wet inside (blotches a bit less glossy), soft stuffing, fabric skin
rough_d = lerp(const(0.10, -1000, 1300), const(0.3, -1000, 1350), blot, -850, 1300)
rough_in = lerp(rough_d, const(0.95, -850, 1400), theat, -700, 1300)
rough0 = lerp(rough_in, const(0.9, -700, 1450), m_stuff, -550, 1300)
rough = lerp(rough0, const(0.8, -550, 1500), m_skin, -400, 1300)
mel.connect_material_property(rough, '', unreal.MaterialProperty.MP_ROUGHNESS)
spec = lerp(const(0.7, -550, 1600), const(0.4, -550, 1650), theat, -400, 1600)
mel.connect_material_property(spec, '', unreal.MaterialProperty.MP_SPECULAR)

# a little self-light on the stuffing so it reads white under the night light (not fully unlit)
gm = lerp(op(unreal.MaterialExpressionMultiply, m_stuff, const(0.6, -800, 1800), -650, 1800), one, theat, -500, 1800)
gm2 = op(unreal.MaterialExpressionMultiply, gm, op(unreal.MaterialExpressionSubtract, one, m_skin, -650, 1900), -350, 1800)
gm3 = op(unreal.MaterialExpressionMultiply, gm2, glow, -200, 1800)
lv = op(unreal.MaterialExpressionMultiply, base, gm3, -50, 1700)
ea = node(unreal.MaterialExpressionEyeAdaptationInverse, 100, 1700)
link(lv, '', ea, 'LightValueInput')
mel.connect_material_property(ea, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)

mel.recompile_material(m)
log.append('saved %s' % unreal.EditorAssetLibrary.save_asset(PATH, False))
log
