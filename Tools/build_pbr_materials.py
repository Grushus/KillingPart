# Run in the editor (execute_python over the MCP, or the Python console) to (re)build the PBRgen materials.
# Imports SourceArt/Fabric and SourceArt/Wood (Simon's PBRgen exports, 2K) into
# /Game/KillingPart/Materials/Textures and builds:
#   M_KPPlush: plush fabric for the plushie bodies and ears. Colour comes from the Color parameter (set per
#     plushie); the texture only adds detail (its luminance over its average, so the pastel stays exact on
#     average). Cloth shading with a lighter fuzz rim, plus a little self-light through EyeAdaptationInverse
#     so the pastels survive the night exposure.
#   M_KPWood: the table, projected from the top in world space (the table is a scaled cube), TileSize uu per tile.
# The height maps are not imported (no Nanite or tessellation on our meshes).
import os
import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
TC = unreal.TextureCompressionSettings
ST = unreal.MaterialSamplerType
MP = unreal.MaterialProperty
PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
TEX = '/Game/KillingPart/Materials/Textures'
log = []

SLOTS = {'BC': (True, TC.TC_DEFAULT), 'N': (False, TC.TC_NORMALMAP), 'ORM': (False, TC.TC_MASKS)}


def import_set(folder, stem):
    tasks = []
    for slot in SLOTS:
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', os.path.join(PROJECT, 'SourceArt', folder, '%s_%s.png' % (stem, slot)))
        task.set_editor_property('destination_path', TEX)
        task.set_editor_property('automated', True)
        task.set_editor_property('replace_existing', True)
        task.set_editor_property('save', False)
        tasks.append(task)
    at.import_asset_tasks(tasks)
    out = {}
    for slot, (srgb, comp) in SLOTS.items():
        path = '%s/%s_%s' % (TEX, stem, slot)
        t = eal.load_asset(path)
        if t is None:
            log.append('IMPORT FAIL ' + path)
            continue
        t.set_editor_property('srgb', srgb)
        t.set_editor_property('compression_settings', comp)
        eal.save_asset(path, False)
        out[slot] = t
    return out


class G:
    def __init__(self, m):
        self.m = m

    def node(self, cls, x, y, **props):
        e = mel.create_material_expression(self.m, cls, x, y)
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    def link(self, a, aout, b, bin_):
        if not mel.connect_material_expressions(a, aout, b, bin_):
            log.append('LINK FAIL %s.%s -> %s.%s' % (type(a).__name__, aout, type(b).__name__, bin_))

    def scalar(self, name, val, x, y):
        return self.node(unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=val)

    def op(self, cls, a, b, x, y, aout='', bout=''):
        e = self.node(cls, x, y)
        self.link(a, aout, e, 'A')
        self.link(b, bout, e, 'B')
        return e

    def const(self, v, x, y):
        return self.node(unreal.MaterialExpressionConstant, x, y, r=v)

    def tex(self, name, texture, sampler, uv, x, y):
        e = self.node(unreal.MaterialExpressionTextureSampleParameter2D, x, y, parameter_name=name, texture=texture, sampler_type=sampler)
        self.link(uv, '', e, 'UVs')
        return e


def fresh(name):
    path = '/Game/KillingPart/Materials/' + name
    if eal.does_asset_exist(path):
        m = unreal.load_asset(path)
        mel.delete_all_material_expressions(m)
    else:
        m = at.create_asset(name, '/Game/KillingPart/Materials', unreal.Material, unreal.MaterialFactoryNew())
    return path, m


def normal_strength(g, n, strength, x, y):
    xy = g.op(unreal.MaterialExpressionAppendVector, strength, strength, x, y)
    xyz = g.op(unreal.MaterialExpressionAppendVector, xy, g.const(1.0, x, y + 120), x + 150, y)
    return g.op(unreal.MaterialExpressionMultiply, n, xyz, x + 300, y, aout='RGB')


def build_plush(t):
    path, m = fresh('M_KPPlush')
    g = G(m)
    m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_CLOTH)

    color = g.node(unreal.MaterialExpressionVectorParameter, -1800, -500, parameter_name='Color',
                   default_value=unreal.LinearColor(r=0.9, g=0.8, b=0.75, a=1))
    tu = g.scalar('TilingU', 6.0, -2000, 0)
    tv = g.scalar('TilingV', 3.0, -2000, 100)
    tiling = g.op(unreal.MaterialExpressionAppendVector, tu, tv, -1850, 50)
    uv = g.op(unreal.MaterialExpressionMultiply, g.node(unreal.MaterialExpressionTextureCoordinate, -1850, -80), tiling, -1700, 0)

    bc = g.tex('BC', t['BC'], ST.SAMPLERTYPE_COLOR, uv, -1500, -300)
    n = g.tex('N', t['N'], ST.SAMPLERTYPE_NORMAL, uv, -1500, 0)
    orm = g.tex('ORM', t['ORM'], ST.SAMPLERTYPE_MASKS, uv, -1500, 300)

    # Detail = luminance / its average (about 0.40 linear), pulled toward 1 by DetailStrength
    lum = g.node(unreal.MaterialExpressionDotProduct, -1200, -300)
    g.link(bc, 'RGB', lum, 'A')
    w = g.node(unreal.MaterialExpressionConstant3Vector, -1350, -200, constant=unreal.LinearColor(r=0.2126, g=0.7152, b=0.0722, a=1))
    g.link(w, '', lum, 'B')
    det = g.op(unreal.MaterialExpressionDivide, lum, g.scalar('DetailAverage', 0.40, -1350, -100), -1050, -300)
    dstr = g.scalar('DetailStrength', 0.8, -1050, -150)
    detail = g.node(unreal.MaterialExpressionLinearInterpolate, -900, -300)
    g.link(g.const(1.0, -1050, -400), '', detail, 'A')
    g.link(det, '', detail, 'B')
    g.link(dstr, '', detail, 'Alpha')
    base = g.op(unreal.MaterialExpressionMultiply, color, detail, -700, -450)
    mel.connect_material_property(base, '', MP.MP_BASE_COLOR)

    nrm = normal_strength(g, n, g.scalar('NormalStrength', 1.0, -1300, 120), -1200, 80)
    mel.connect_material_property(nrm, '', MP.MP_NORMAL)

    rough = g.op(unreal.MaterialExpressionMultiply, orm, g.scalar('RoughnessMultiplier', 1.0, -1200, 450), -900, 350, aout='G')
    mel.connect_material_property(rough, '', MP.MP_ROUGHNESS)
    ao = g.node(unreal.MaterialExpressionLinearInterpolate, -900, 500)
    g.link(g.const(1.0, -1050, 520), '', ao, 'A')
    g.link(orm, 'R', ao, 'B')
    g.link(g.scalar('AOStrength', 0.5, -1050, 620), '', ao, 'Alpha')
    mel.connect_material_property(ao, '', MP.MP_AMBIENT_OCCLUSION)
    mel.connect_material_property(g.const(0.0, -700, 250), '', MP.MP_METALLIC)
    mel.connect_material_property(g.const(0.35, -700, 300), '', MP.MP_SPECULAR)

    # Fuzz: a lighter tint of the pastel (SubsurfaceColor is the fuzz colour for Cloth, CustomData0 the amount)
    fuzz = g.node(unreal.MaterialExpressionLinearInterpolate, -700, 700)
    g.link(color, '', fuzz, 'A')
    g.link(g.const(1.0, -850, 760), '', fuzz, 'B')
    g.link(g.scalar('FuzzLighten', 0.55, -850, 860), '', fuzz, 'Alpha')
    mel.connect_material_property(fuzz, '', MP.MP_SUBSURFACE_COLOR)
    mel.connect_material_property(g.scalar('Cloth', 0.9, -700, 950), '', MP.MP_CUSTOM_DATA0)

    # Self-light so the pastel reads at night (in exposure-independent units, like M_KPUnlit)
    glow = g.op(unreal.MaterialExpressionMultiply, base, g.scalar('SelfLight', 0.12, -700, -250), -500, -300)
    ea = g.node(unreal.MaterialExpressionEyeAdaptationInverse, -300, -300)
    g.link(glow, '', ea, 'LightValueInput')
    mel.connect_material_property(ea, '', MP.MP_EMISSIVE_COLOR)

    mel.recompile_material(m)
    log.append('saved %s %s' % (path, eal.save_asset(path, False)))


def build_wood(t):
    # T7b: the walnut no longer visibly repeats: bigger tiles (TileSize), a second sample of the same maps at
    # another scale and offset blended in by a large soft noise (MacroSize), and a macro brightness variation
    # (MacroShade) on top. The grain keeps its direction (no rotation: it would cross the grain).
    path, m = fresh('M_KPWood')
    g = G(m)
    m.set_editor_property('used_with_instanced_static_meshes', True)
    wp = g.node(unreal.MaterialExpressionWorldPosition, -2600, 0)
    xy = g.node(unreal.MaterialExpressionComponentMask, -2450, 0, r=True, g=True, b=False, a=False)
    g.link(wp, '', xy, '')
    tile = g.scalar('TileSize', 1100.0, -2450, 120)
    uv = g.op(unreal.MaterialExpressionDivide, xy, tile, -2250, 0)
    tile_b = g.op(unreal.MaterialExpressionMultiply, tile, g.const(1.37, -2450, 220), -2300, 200)
    uv_b0 = g.op(unreal.MaterialExpressionDivide, xy, tile_b, -2150, 200)
    off = g.node(unreal.MaterialExpressionConstant2Vector, -2300, 320, r=0.31, g=0.57)
    uv_b = g.op(unreal.MaterialExpressionAdd, uv_b0, off, -2000, 200)
    macro = g.scalar('MacroSize', 2600.0, -2450, 450)
    wp_s = g.op(unreal.MaterialExpressionDivide, wp, macro, -2250, 450)
    noise = g.node(unreal.MaterialExpressionNoise, -2050, 450, scale=1.0, levels=2, turbulence=False,
                   output_min=0.0, output_max=1.0, quality=1)
    g.link(wp_s, '', noise, 'World Position')
    # a sharper blend between the two samples (0.3-0.7 of the noise)
    nmask = g.node(unreal.MaterialExpressionSaturate, -1550, 450)
    g.link(g.op(unreal.MaterialExpressionMultiply, g.op(unreal.MaterialExpressionSubtract, noise, g.const(0.3, -1900, 560), -1800, 450), g.const(2.5, -1800, 560), -1650, 450), '', nmask, '')
    bca = g.tex('BC', t['BC'], ST.SAMPLERTYPE_COLOR, uv, -1500, -400)
    bcb = g.tex('BC_B', t['BC'], ST.SAMPLERTYPE_COLOR, uv_b, -1500, -250)
    na = g.tex('N', t['N'], ST.SAMPLERTYPE_NORMAL, uv, -1500, -50)
    nb = g.tex('N_B', t['N'], ST.SAMPLERTYPE_NORMAL, uv_b, -1500, 100)
    orma = g.tex('ORM', t['ORM'], ST.SAMPLERTYPE_MASKS, uv, -1500, 280)
    ormb = g.tex('ORM_B', t['ORM'], ST.SAMPLERTYPE_MASKS, uv_b, -1500, 430)

    def lerp(a, b, alpha, x, y, aout=''):
        e = g.node(unreal.MaterialExpressionLinearInterpolate, x, y)
        g.link(a, aout, e, 'A')
        g.link(b, aout, e, 'B')
        g.link(alpha, '', e, 'Alpha')
        return e
    bc = lerp(bca, bcb, nmask, -1250, -350, 'RGB')
    n = lerp(na, nb, nmask, -1250, 0, 'RGB')
    orm = lerp(orma, ormb, nmask, -1250, 350, 'RGB')
    # macro shade: a second, larger noise darkens and lightens areas a little
    wp_s2 = g.op(unreal.MaterialExpressionDivide, wp, g.op(unreal.MaterialExpressionMultiply, macro, g.const(2.3, -2450, 650), -2300, 650), -2150, 650)
    noise2 = g.node(unreal.MaterialExpressionNoise, -2000, 650, scale=1.0, levels=2, turbulence=False,
                    output_min=0.0, output_max=1.0, quality=1)
    g.link(wp_s2, '', noise2, 'World Position')
    shade_amt = g.scalar('MacroShade', 0.22, -1900, 760)
    shade = g.op(unreal.MaterialExpressionAdd, g.op(unreal.MaterialExpressionMultiply,
                 g.op(unreal.MaterialExpressionSubtract, noise2, g.const(0.5, -1850, 700), -1750, 650), shade_amt, -1600, 650),
                 g.const(1.0, -1600, 760), -1450, 650)
    tint = g.node(unreal.MaterialExpressionVectorParameter, -1300, -550, parameter_name='Tint',
                  default_value=unreal.LinearColor(r=1, g=1, b=1, a=1))
    base = g.op(unreal.MaterialExpressionMultiply, bc, tint, -1000, -450)
    base = g.op(unreal.MaterialExpressionMultiply, base, g.scalar('Brightness', 1.0, -1150, -300), -850, -450)
    base = g.op(unreal.MaterialExpressionMultiply, base, shade, -700, -450)
    mel.connect_material_property(base, '', MP.MP_BASE_COLOR)
    strength = g.scalar('NormalStrength', 1.0, -1300, 120)
    nxy = g.op(unreal.MaterialExpressionAppendVector, strength, strength, -1100, 80)
    nxyz = g.op(unreal.MaterialExpressionAppendVector, nxy, g.const(1.0, -1100, 200), -950, 80)
    nrm = g.op(unreal.MaterialExpressionMultiply, n, nxyz, -800, 80)
    mel.connect_material_property(nrm, '', MP.MP_NORMAL)
    orm_g = g.node(unreal.MaterialExpressionComponentMask, -1100, 400, r=False, g=True, b=False, a=False)
    g.link(orm, '', orm_g, '')
    orm_r = g.node(unreal.MaterialExpressionComponentMask, -1100, 550, r=True, g=False, b=False, a=False)
    g.link(orm, '', orm_r, '')
    rough = g.op(unreal.MaterialExpressionMultiply, orm_g, g.scalar('RoughnessMultiplier', 1.0, -1200, 500), -900, 400)
    mel.connect_material_property(rough, '', MP.MP_ROUGHNESS)
    mel.connect_material_property(orm_r, '', MP.MP_AMBIENT_OCCLUSION)
    mel.connect_material_property(g.const(0.0, -700, 250), '', MP.MP_METALLIC)
    mel.recompile_material(m)
    log.append('saved %s %s' % (path, eal.save_loaded_asset(m, False)))


build_plush(import_set('Fabric', 'T_Soft_Minky_Plush'))
build_wood(import_set('Wood', 'T_Worn_Walnut_Wood'))
log
