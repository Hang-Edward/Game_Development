import os
import time

import unreal


MAP_PATH = "/Game/Maps/Map01_Forest"
TIMEOUT_SECONDS = 10.0


state = {
    "start_time": time.time(),
    "handle": None,
    "finished": False,
}


def log(message):
    unreal.log("[MagicShardPIESmoke] " + message)


def fail(message):
    unreal.log_error("[MagicShardPIESmoke] " + message)
    finish(False)


def finish(success):
    if state["finished"]:
        return

    state["finished"] = True
    if state["handle"] is not None:
        unreal.unregister_slate_post_tick_callback(state["handle"])

    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if level_editor is not None:
        try:
            level_editor.editor_end_play()
        except Exception:
            pass

    log("PIE smoke test passed" if success else "PIE smoke test failed")
    unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")


def find_actor_by_class(world, actor_class):
    actors = unreal.GameplayStatics.get_all_actors_of_class(world, actor_class)
    return actors[0] if actors else None


def check_pie_world():
    pie_worlds = unreal.EditorLevelLibrary.get_pie_worlds(False)
    if not pie_worlds:
        return False

    pie_world = pie_worlds[0]
    pawn_class = unreal.load_class(None, "/Script/MagicShard.MagicShardPlayerCharacter")
    if pawn_class is None:
        fail("MagicShard player class is not loadable.")
        return True

    pawn = find_actor_by_class(pie_world, pawn_class)
    if pawn is None:
        return False

    controller = unreal.GameplayStatics.get_player_controller(pie_world, 0)
    if controller is None:
        return False

    hud = controller.get_hud()
    if hud is None:
        return False

    save_path = os.path.join(unreal.Paths.project_saved_dir(), "MagicShard", "PlayerRecords.dat")
    if not os.path.exists(save_path):
        fail("Random-access save file was not created: " + save_path)
        return True

    size = os.path.getsize(save_path)
    if size <= 0:
        fail("Random-access save file is empty: " + save_path)
        return True

    log("PIE spawned pawn: " + pawn.get_name())
    log("PIE spawned HUD: " + hud.get_name())
    log("Random-access save file size: " + str(size))
    finish(True)
    return True


def on_tick(delta_seconds):
    if state["finished"]:
        return

    if check_pie_world():
        return

    elapsed = time.time() - state["start_time"]
    if elapsed > TIMEOUT_SECONDS:
        fail("Timed out waiting for PIE pawn, HUD, and save file.")


def start_smoke_test():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if level_editor is None:
        fail("LevelEditorSubsystem is unavailable.")
        return

    log("Opening " + MAP_PATH)
    if not level_editor.load_level(MAP_PATH):
        fail("Unable to load " + MAP_PATH)
        return

    world = unreal.EditorLevelLibrary.get_editor_world()
    if world is None:
        fail("No editor world is available before PIE.")
        return

    player_start = find_actor_by_class(world, unreal.PlayerStart)
    if player_start is None:
        fail("PlayerStart was not found in the test map.")
        return

    game_mode_class = unreal.load_class(None, "/Script/MagicShard.MagicShardGameMode")
    if game_mode_class is None:
        fail("MagicShardGameMode is not loadable.")
        return

    log("Starting PIE session")
    state["handle"] = unreal.register_slate_post_tick_callback(on_tick)
    level_editor.editor_play_simulate()


start_smoke_test()
