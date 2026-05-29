# MagicShard Unreal Scripts

PowerShell examples:

```powershell
cd "D:\VScode Projects\Game_Development\MagicShard_Unreal"
.\Scripts\Launch-Editor.ps1
.\Scripts\Build-Unreal.ps1
.\Scripts\Build-Launcher.ps1
.\Scripts\ImportLegacyAssets.py
.\Scripts\ImportUnityMaterialTextures.py
.\Scripts\SetupImportedWorld.py
.\Scripts\Run-RuntimeSmoke.ps1
```

Use `Launch-Editor.ps1` instead of launching the bare Unreal Editor from the Epic Launcher. The project disables UE 5.7's NNE runtime plugins, while the bare engine project browser can still load them before project settings are applied.

Script list:

- `Find-UnrealEditor.ps1`: locates the installed Unreal Editor.
- `Launch-Editor.ps1`: opens this project directly.
- `Build-Unreal.ps1`: builds the MagicShard Unreal C++ editor target.
- `Build-Launcher.ps1`: builds `map1.exe`, a small one-click project launcher.
- `CreatePrototypeTestMap.py`: creates the prototype runtime test map from Unreal Python.
- `ImportLegacyAssets.py`: imports the main character Idle/Walk/Run FBX files and the forest map FBX.
- `ImportUnityMaterialTextures.py`: ports Unity material texture assignments into Unreal material instances.
- `SetupImportedWorld.py`: creates `/Game/Maps/Map01_Forest` with the imported map mesh and PlayerStart.
- `Run-RuntimeSmoke.ps1`: starts the prototype map in runtime mode and checks startup smoke-test markers.
