# Helpers for testing Killing Part over the MCP (exec this file in execute_python, then call them with await).
# Not shipped; used by Claude Code to start Play, run console commands and save game-camera screenshots.
import base64
import os
import unreal

SHOTS = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), 'Screenshots', 'KP')


def game_world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()


def cmd(text):
    unreal.SystemLibrary.execute_console_command(game_world(), text)


async def wait(seconds):
    for _ in range(int(seconds * 10)):
        await asyncio.sleep(0.1)


async def start_play(settle=4.0):
    if game_world() is None:
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
    await wait(settle)


def stop_play():
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()


async def shot(name, max_res=1600):
    r = await call_tool('SceneCaptureToolset.SceneCaptureToolset.CaptureGameViewport', world=None, maxResolution=max_res)
    img = r['image']
    data = img['data'] if isinstance(img, dict) else img
    os.makedirs(SHOTS, exist_ok=True)
    path = os.path.join(SHOTS, name)
    open(path, 'wb').write(base64.b64decode(data))
    return path


async def closeup(name, target, back=230, up=170, yaw=0.0, pitch=-35.0, fov=40.0, w=900, h=600):
    cam = {'location': {'x': target.x - back, 'y': target.y, 'z': target.z + up},
           'rotation': {'pitch': pitch, 'yaw': yaw, 'roll': 0}, 'fOV': fov}
    r = await call_tool('SceneCaptureToolset.SceneCaptureToolset.Capture', world=game_world(), camera=cam, mode='Lit',
                        options={'width': w, 'height': h, 'screenPercentage': 1, 'visualizationSubMode': ''})
    img = r['image']
    data = img['data'] if isinstance(img, dict) else img
    path = os.path.join(SHOTS, name)
    open(path, 'wb').write(base64.b64decode(data))
    return path


def actors(class_name):
    cls = unreal.load_class(None, '/Script/CardIsland.' + class_name)
    return unreal.GameplayStatics.get_all_actors_of_class(game_world(), cls)


async def kp_log(lines=40, contains='LogKP'):
    r = await call_tool('ConsoleToolset.LogsToolset.GetLogEntries', logCategory='LogKP')
    return r
