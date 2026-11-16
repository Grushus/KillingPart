# Run in the editor (execute_python over the MCP, or the Python console) to (re)build M_KPHeroine (T7f).
# The heroine is Simon's Tripo model (SourceArt/Heroine/SM_Heroine.glb, imported to /Game/KillingPart/Heroine).
# Tripo bakes one PBR set: base colour, glTF roughness/metallic (G = roughness, B = metallic) and a normal map.
# M_KPHeroine: default lit, the baked colour, plus a little self-light through EyeAdaptationInverse (like
# M_KPPlush) so she reads pastel and bright at night, and a Flash amount (FlashColor) for the hurt flash.
import unreal

mel = unreal.MaterialEditingLibrary
eal = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
MP = unreal.MaterialProperty
ST = unreal.MaterialSamplerType
TC = unreal.TextureCompressionSettings
DIR = '/Game/KillingPart/Heroine'
log = []


def node(m, cls, x, y, **props):
    e = mel.create_material_expression(m, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def link(a, aout, b, bin_):
    if not mel.connect_material_expressions(a, aout, b, bin_):
        log.append('LINK FAIL %s.%s -> %s.%s' % (type(a).__name__, aout, type(b).__name__, bin_))


def op(m, cls, a, b, x, y, aout='', bout=''):
    e = node(m, cls, x, y)
    link(a, aout, e, 'A')
    link(b, bout, e, 'B')
    return e


def scalar(m, name, val, x, y):
    return node(m, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=val)


def fix_textures():
    bc = eal.load_asset(DIR + '/T_Heroine_BaseColor')
    rm = eal.load_asset(DIR + '/T_Heroine_RM')
    n = eal.load_asset(DIR + '/T_Heroine_Normal')
    bc.set_editor_property('srgb', True)
    rm.set_editor_property('srgb', False)
    rm.set_editor_property('compression_settings', TC.TC_MASKS)
    n.set_editor_property('srgb', False)
    n.set_editor_property('compression_settings', TC.TC_NORMALMAP)
    for name in ('T_Heroine_BaseColor', 'T_Heroine_RM', 'T_Heroine_Normal'):
        eal.save_asset(DIR + '/' + name, False)
    return bc, rm, n


def build():
    bc_t, rm_t, n_t = fix_textures()
    path = DIR + '/M_KPHeroine'
    if eal.does_asset_exist(path):
        m = unreal.load_asset(path)
        mel.delete_all_material_expressions(m)
    else:
        m = at.create_asset('M_KPHeroine', DIR, unreal.Material, unreal.MaterialFactoryNew())

    bc = node(m, unreal.MaterialExpressionTextureSampleParameter2D, -1400, -300, parameter_name='BaseColor', texture=bc_t, sampler_type=ST.SAMPLERTYPE_COLOR)
    rm = node(m, unreal.MaterialExpressionTextureSampleParameter2D, -1400, 100, parameter_name='RM', texture=rm_t, sampler_type=ST.SAMPLERTYPE_MASKS)
    n = node(m, unreal.MaterialExpressionTextureSampleParameter2D, -1400, 400, parameter_name='Normal', texture=n_t, sampler_type=ST.SAMPLERTYPE_NORMAL)

    # Base colour, a touch brighter if wanted (Brightness), tinted (Tint, white by default)
    tint = node(m, unreal.MaterialExpressionVectorParameter, -1150, -450, parameter_name='Tint', default_value=unreal.LinearColor(r=1, g=1, b=1, a=1))
    tinted = op(m, unreal.MaterialExpressionMultiply, bc, tint, -950, -350, aout='RGB')
    base = op(m, unreal.MaterialExpressionMultiply, tinted, scalar(m, 'Brightness', 1.0, -1150, -200), -800, -300)
    mel.connect_material_property(base, '', MP.MP_BASE_COLOR)

    rough = op(m, unreal.MaterialExpressionMultiply, rm, scalar(m, 'RoughnessMultiplier', 1.0, -1150, 150), -900, 100, aout='G')
    mel.connect_material_property(rough, '', MP.MP_ROUGHNESS)
    metal = op(m, unreal.MaterialExpressionMultiply, rm, scalar(m, 'MetallicMultiplier', 1.0, -1150, 250), -900, 220, aout='B')
    mel.connect_material_property(metal, '', MP.MP_METALLIC)
    mel.connect_material_property(n, 'RGB', MP.MP_NORMAL)

    # Self-light so she reads at night + the hurt flash, in exposure-independent units
    glow = op(m, unreal.MaterialExpressionMultiply, base, scalar(m, 'SelfLight', 0.2, -800, -150), -600, -200)
    flash_col = node(m, unreal.MaterialExpressionVectorParameter, -800, -50, parameter_name='FlashColor', default_value=unreal.LinearColor(r=1, g=0.25, b=0.35, a=1))
    flash = op(m, unreal.MaterialExpressionMultiply, flash_col, scalar(m, 'Flash', 0.0, -800, 50), -600, -50)
    total = op(m, unreal.MaterialExpressionAdd, glow, flash, -450, -150)
    ea = node(m, unreal.MaterialExpressionEyeAdaptationInverse, -300, -150)
    link(total, '', ea, 'LightValueInput')
    mel.connect_material_property(ea, '', MP.MP_EMISSIVE_COLOR)

    mel.recompile_material(m)
    log.append('saved %s %s' % (path, eal.save_asset(path, False)))

    mesh = eal.load_asset(DIR + '/SM_Heroine')
    mesh.set_material(0, m)
    log.append('mesh %s' % eal.save_asset(DIR + '/SM_Heroine', False))


build()
print('\n'.join(log))
