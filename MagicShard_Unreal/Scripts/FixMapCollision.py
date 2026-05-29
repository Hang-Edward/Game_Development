import sys, unreal

MAP_MESH_PATH = "/Game/Imported/Map/map_01_forest.map_01_forest"

def log(msg):
    unreal.log("[CollisionFix] " + msg)

def main():
    mesh = unreal.load_asset(MAP_MESH_PATH)
    if mesh is None:
        log("ERROR: mesh not found at " + MAP_MESH_PATH)
        sys.exit(1)

    log("Mesh: " + mesh.get_path_name())

    body_setup = mesh.get_editor_property("body_setup")
    if body_setup is None:
        log("ERROR: BodySetup is null - creating new one")
        body_setup = unreal.BodySetup()
        mesh.set_editor_property("body_setup", body_setup)
        body_setup = mesh.get_editor_property("body_setup")
        if body_setup is None:
            log("FATAL: Cannot create BodySetup")
            sys.exit(1)

    # Set collision to use mesh triangles directly
    body_setup.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    log("Set collision_trace_flag = CTF_USE_COMPLEX_AS_SIMPLE")

    # Save
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    log("Saved. Collision is now enabled on terrain mesh.")
    log("DONE")

main()
