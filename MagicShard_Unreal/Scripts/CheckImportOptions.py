import unreal

options = unreal.FbxImportUI()
for p in sorted(dir(options)):
    if not p.startswith("_"):
        try:
            val = options.get_editor_property(p)
            if val is not None:
                low = p.lower()
                if any(k in low for k in ["interchange","legacy","fbx","import","type"]):
                    print(f"  {p} = {val}")
        except:
            pass
