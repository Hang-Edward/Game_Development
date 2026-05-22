using UnityEngine;
using UnityEngine.UI;
using UnityEditor;
using UnityEditor.Animations;
using UnityEditor.SceneManagement;
using UnityEngine.SceneManagement;
using UnityEngine.InputSystem;
using UnityEngine.Rendering;
using TMPro;
using System.IO;
using System.Linq;

public class SetupHelper : EditorWindow
{
    [MenuItem("Tools/MagicShard/Setup Project &%s")]
    public static void SetupProject()
    {
        SetupAnimatorController();
        SetupInputSettings();
        SetupProjectSettings();
        Debug.Log("Setup complete! Now run: Tools > MagicShard > Build Scene");
    }

    [MenuItem("Tools/MagicShard/Build Scene &%b")]
    public static void BuildScene()
    {
        if (EditorApplication.isPlaying)
        {
            Debug.LogWarning("Build Scene is an editor-only setup action. Exit Play Mode before running it.");
            return;
        }

        string scenePath = "Assets/Scenes/GameWorld.unity";
        var scene = EditorSceneManager.NewScene(NewSceneSetup.DefaultGameObjects, NewSceneMode.Single);
        EditorSceneManager.SetActiveScene(scene);
        CreatePlayer();
        SetupCamera();
        CreateTerrain();
        CreateUI();
        UpgradeMaterials();
        Physics.SyncTransforms();
		Directory.CreateDirectory("Assets/Scenes");
        EditorSceneManager.SaveScene(scene, scenePath);
        Debug.Log("Scene saved. Press Play to test!");
    }

    static GameObject CreatePlayer()
    {
        var inputActions = AssetDatabase.LoadAssetAtPath<InputActionAsset>(
            "Assets/Settings/GameInput.inputactions");
        var controller = AssetDatabase.LoadAssetAtPath<AnimatorController>(
            "Assets/Animations/Controllers/CharacterAnimator.controller");
        var characterModel = AssetDatabase.LoadAssetAtPath<GameObject>(
            "Assets/Models/Character/stand.fbx");

        var player = new GameObject("Player");
        player.tag = "Player";
        var cc = player.AddComponent<CharacterController>();
        cc.height = 1.8f; cc.radius = 0.4f; cc.stepOffset = 0.3f; cc.skinWidth = 0.08f;

        var pi = player.AddComponent<PlayerInput>();
        pi.actions = inputActions;

        player.AddComponent<PlayerController>();
        player.AddComponent<CombatSystem>();
        player.AddComponent<CharacterMaterialApplier>();

        if (characterModel != null)
        {
            var model = (GameObject)PrefabUtility.InstantiatePrefab(characterModel);
            model.name = "CharacterModel";
            model.transform.SetParent(player.transform);
            model.transform.localPosition = Vector3.zero;
            model.transform.localRotation = Quaternion.identity;
            var animator = model.GetComponent<Animator>();
            if (animator == null) animator = model.AddComponent<Animator>();
            animator.runtimeAnimatorController = controller;
            var pc = player.GetComponent<PlayerController>();
            var so = new SerializedObject(pc);
            so.FindProperty("animator").objectReferenceValue = animator;
            so.ApplyModifiedProperties();
        }
        player.transform.position = new Vector3(0, 2, 0);
        Undo.RegisterCreatedObjectUndo(player, "Create Player");
        Debug.Log("Player created");
        return player;
    }

    static void SetupCamera()
    {
        var camGO = GameObject.FindWithTag("MainCamera");
        if (camGO == null)
        {
            camGO = GameObject.Find("Main Camera");
            if (camGO != null) camGO.tag = "MainCamera";
        }
        if (camGO == null)
        {
            camGO = new GameObject("Main Camera");
            camGO.tag = "MainCamera";
            camGO.AddComponent<Camera>();
            camGO.AddComponent<AudioListener>();
        }
        camGO.AddComponent<CameraController>();
    }

    static void ConfigureFBXImport(string path)
    {
        var importer = AssetImporter.GetAtPath(path) as ModelImporter;
        if (importer == null) return;

        bool changed = false;
        if (!importer.isReadable) { importer.isReadable = true; changed = true; }
        if (changed)
        {
            importer.SaveAndReimport();
            // Force sync reimport
            AssetDatabase.ImportAsset(path, ImportAssetOptions.ForceUpdate);
            Debug.Log($"Configured FBX Read/Write: {path}");
        }
    }

    static void CreateTerrain()
    {
        string fbxPath = "Assets/Models/Map/map_01_forest.fbx";
        ConfigureFBXImport(fbxPath);

        var terrainModel = AssetDatabase.LoadAssetAtPath<GameObject>(fbxPath);
        if (terrainModel != null)
        {
            var terrain = (GameObject)PrefabUtility.InstantiatePrefab(terrainModel);
            terrain.name = "Terrain";
            terrain.transform.position = Vector3.zero;

            // Add platform collider as ground backup (ensures player always has something to stand on)
            var ground = GameObject.CreatePrimitive(PrimitiveType.Plane);
            ground.name = "GroundPlane";
            ground.transform.SetParent(terrain.transform);
            ground.transform.position = new Vector3(0, 0, 0);
            ground.transform.localScale = new Vector3(200, 1, 200);
            var mr = ground.GetComponent<MeshRenderer>();
            if (mr != null) mr.enabled = false; // hide the plane

            Undo.RegisterCreatedObjectUndo(terrain, "Create Terrain");
            Debug.Log("Terrain placed with ground plane");
        }
        else
            Debug.LogWarning("map_01_forest.fbx not found.");
    }

    static void CreateUI()
    {
        var canvasObj = new GameObject("Canvas");
        var canvas = canvasObj.AddComponent<Canvas>();
        canvas.renderMode = RenderMode.ScreenSpaceOverlay;
        var scaler = canvasObj.AddComponent<CanvasScaler>();
        scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
        scaler.referenceResolution = new Vector2(1280, 720);
        canvasObj.AddComponent<GraphicRaycaster>();

        var uiManager = canvasObj.AddComponent<UIManager>();
        if (TMP_Settings.instance == null)
            Debug.Log("Run: Window > TextMeshPro > Import TMP Essentials");

        CreateText(canvasObj, "FPS_Text", "FPS: 60", 160, 30, 20, 20);
        CreateText(canvasObj, "AnimInfo_Text", "Speed: 0.00", 200, 30, 20, 50);

        var hint = CreateText(canvasObj, "Controls_Hint",
            "[WASD] Move [Shift] Sprint [Ctrl] Crouch [LMB] Attack [RMB] Block [Wheel] Zoom",
            800, 30, 240, 670);
        var rt = hint.GetComponent<RectTransform>();
        rt.pivot = new Vector2(0.5f, 1);
        rt.anchorMin = new Vector2(0.5f, 1);
        rt.anchorMax = new Vector2(0.5f, 1);
        rt.anchoredPosition = new Vector2(0, -20);

        var prompt = CreateText(canvasObj, "Center_Prompt",
            "Click to start", 300, 50, 490, 335);
        var prt = prompt.GetComponent<RectTransform>();
        prt.pivot = new Vector2(0.5f, 0.5f);
        prt.anchorMin = new Vector2(0.5f, 0.5f);
        prt.anchorMax = new Vector2(0.5f, 0.5f);
        prompt.SetActive(false);

        var so = new SerializedObject(uiManager);
        so.FindProperty("fpsText").objectReferenceValue =
            canvasObj.transform.Find("FPS_Text")?.GetComponent<TextMeshProUGUI>();
        so.FindProperty("animInfoText").objectReferenceValue =
            canvasObj.transform.Find("AnimInfo_Text")?.GetComponent<TextMeshProUGUI>();
        so.FindProperty("controlsHint").objectReferenceValue =
            canvasObj.transform.Find("Controls_Hint")?.GetComponent<TextMeshProUGUI>();
        so.FindProperty("centerPrompt").objectReferenceValue =
            canvasObj.transform.Find("Center_Prompt")?.gameObject;
        so.FindProperty("centerPromptText").objectReferenceValue =
            canvasObj.transform.Find("Center_Prompt")?.GetComponent<TextMeshProUGUI>();
        so.ApplyModifiedProperties();

        Undo.RegisterCreatedObjectUndo(canvasObj, "Create UI");
        Debug.Log("UI Canvas created");
    }

        static GameObject CreateText(GameObject parent, string name, string content,
        float w, float h, float x, float y)
    {
        var obj = new GameObject(name);
        obj.transform.SetParent(parent.transform);
        var text = obj.AddComponent<TextMeshProUGUI>();
        text.text = content;
        text.fontSize = 15;
        text.color = Color.white;
        text.alignment = TextAlignmentOptions.TopLeft;
        var rt = obj.GetComponent<RectTransform>();
        rt.sizeDelta = new Vector2(w, h);
        rt.anchoredPosition = new Vector2(x, y);
        rt.pivot = new Vector2(0, 1);
        rt.anchorMin = new Vector2(0, 1);
        rt.anchorMax = new Vector2(0, 1);
        return obj;
    }

    static void UpgradeMaterials()
    {
        AssetDatabase.Refresh();

        bool usingRenderPipeline = GraphicsSettings.defaultRenderPipeline != null || QualitySettings.renderPipeline != null;
        Shader targetShader = usingRenderPipeline
            ? Shader.Find("Universal Render Pipeline/Lit")
            : Shader.Find("Standard");

        if (targetShader == null)
        {
            Debug.LogError("Target material shader not found.");
            return;
        }

        // Convert imported materials to the shader that matches the active render pipeline.
        foreach (var mat in AssetDatabase.FindAssets("t:Material")
            .Select(g => AssetDatabase.LoadAssetAtPath<Material>(AssetDatabase.GUIDToAssetPath(g)))
            .Where(m => m != null && m.shader != null &&
                (m.shader.name.Contains("Standard") || m.shader.name.Contains("Universal Render Pipeline") ||
                 m.shader.name.Contains("gltf") ||
                 m.shader.name == "Hidden/InternalErrorShader")))
        {
            var mainTex = mat.HasProperty("_BaseMap") ? mat.GetTexture("_BaseMap") : null;
            if (mainTex == null && mat.HasProperty("_MainTex"))
                mainTex = mat.GetTexture("_MainTex");

            var color = mat.HasProperty("_Color") ? mat.GetColor("_Color") :
                mat.HasProperty("_BaseColor") ? mat.GetColor("_BaseColor") : Color.white;
            var normalMap = mat.HasProperty("_BumpMap") ? mat.GetTexture("_BumpMap") : null;
            var metallicMap = mat.HasProperty("_MetallicGlossMap") ? mat.GetTexture("_MetallicGlossMap") : null;
            var occlusionMap = mat.HasProperty("_OcclusionMap") ? mat.GetTexture("_OcclusionMap") : null;
            var emissionMap = mat.HasProperty("_EmissionMap") ? mat.GetTexture("_EmissionMap") : null;
            var emissionColor = mat.HasProperty("_EmissionColor") ? mat.GetColor("_EmissionColor") : Color.black;
            var smoothness = mat.HasProperty("_Glossiness") ? mat.GetFloat("_Glossiness") :
                mat.HasProperty("_Smoothness") ? mat.GetFloat("_Smoothness") : 0.5f;
            var metallic = mat.HasProperty("_Metallic") ? mat.GetFloat("_Metallic") : 0f;

            mat.shader = targetShader;

            if (usingRenderPipeline)
            {
                if (mainTex != null) mat.SetTexture("_BaseMap", mainTex);
                mat.SetColor("_BaseColor", color);
                if (normalMap != null) mat.SetTexture("_BumpMap", normalMap);
                if (metallicMap != null) mat.SetTexture("_MetallicGlossMap", metallicMap);
                if (occlusionMap != null) mat.SetTexture("_OcclusionMap", occlusionMap);
                if (emissionMap != null) mat.SetTexture("_EmissionMap", emissionMap);
                mat.SetColor("_EmissionColor", emissionColor);
                mat.SetFloat("_Smoothness", smoothness);
                mat.SetFloat("_Metallic", metallic);
            }
            else
            {
                if (mainTex != null) mat.SetTexture("_MainTex", mainTex);
                mat.SetColor("_Color", color);
                if (normalMap != null) mat.SetTexture("_BumpMap", normalMap);
                if (metallicMap != null) mat.SetTexture("_MetallicGlossMap", metallicMap);
                if (occlusionMap != null) mat.SetTexture("_OcclusionMap", occlusionMap);
                if (emissionMap != null) mat.SetTexture("_EmissionMap", emissionMap);
                mat.SetColor("_EmissionColor", emissionColor);
                mat.SetFloat("_Glossiness", smoothness);
                mat.SetFloat("_Metallic", metallic);
            }

            EditorUtility.SetDirty(mat);
        }

        AssetDatabase.SaveAssets();
        Debug.Log($"Materials converted to {targetShader.name}");
    }

    [MenuItem("Tools/MagicShard/Create Animator Controller")]
    public static void SetupAnimatorController()
    {
        string path = "Assets/Animations/Controllers/CharacterAnimator.controller";
        bool exists = File.Exists(Application.dataPath + "/../" + path);
        Directory.CreateDirectory("Assets/Animations/Controllers");
        AnimatorController controller;
        if (exists)
        {
            controller = AssetDatabase.LoadAssetAtPath<AnimatorController>(path);
            Debug.Log("Updating existing Animator Controller");
        }
        else
        {
            controller = AnimatorController.CreateAnimatorControllerAtPath(path);
            if (controller == null) { Debug.LogError("Failed"); return; }
            controller.AddParameter("Speed", AnimatorControllerParameterType.Float);
            controller.AddParameter("Sprint", AnimatorControllerParameterType.Bool);
            controller.AddParameter("Crouch", AnimatorControllerParameterType.Bool);
            controller.AddParameter("Block", AnimatorControllerParameterType.Bool);
            controller.AddParameter("Grounded", AnimatorControllerParameterType.Bool);
            controller.AddParameter("Attack", AnimatorControllerParameterType.Trigger);
            controller.AddParameter("MotionSpeed", AnimatorControllerParameterType.Float);
        }
        var clips = AssetDatabase.FindAssets("t:AnimationClip", new[] { "Assets/Models/Character" })
            .Select(g => AssetDatabase.LoadAssetAtPath<AnimationClip>(AssetDatabase.GUIDToAssetPath(g)))
            .Where(c => c != null).ToArray();
        AnimationClip idleClip = FindClip(clips, "idle");
        AnimationClip walkClip = FindClip(clips, "walk");
        AnimationClip runClip = FindClip(clips, "run");

        var layer = controller.layers[0];
        var sm = layer.stateMachine;
        BlendTree blendTree = null;
        foreach (var childState in sm.states)
            if (childState.state.name == "Locomotion" && childState.state.motion is BlendTree bt)
            { blendTree = bt; break; }
        if (blendTree == null)
        {
            blendTree = new BlendTree();
            controller.CreateBlendTreeInController("Locomotion", out blendTree);
        }
        blendTree.blendType = BlendTreeType.Simple1D;
        blendTree.blendParameter = "Speed";
        blendTree.useAutomaticThresholds = true;
        blendTree.children = new ChildMotion[0];
        if (idleClip != null) blendTree.AddChild(idleClip, 0);
        else Debug.LogWarning("Idle clip not found.");
        if (walkClip != null) blendTree.AddChild(walkClip, 3);
        else Debug.LogWarning("Walk clip not found.");
        if (runClip != null) blendTree.AddChild(runClip, 6);
        else Debug.LogWarning("Run clip not found.");
        if (!exists)
        {
            var locoState = sm.AddState("Locomotion");
            locoState.motion = blendTree;
            sm.defaultState = locoState;
        }
        AssetDatabase.SaveAssets();
        Debug.Log("Animator updated: " + idleClip?.name + "/" + walkClip?.name + "/" + runClip?.name);
    }

    static AnimationClip FindClip(AnimationClip[] clips, params string[] names)
    {
        foreach (var name in names)
        {
            var match = clips.FirstOrDefault(c => c.name.ToLower().Contains(name.ToLower()));
            if (match != null) return match;
        }
        return null;
    }

    [MenuItem("Tools/MagicShard/Setup Input Settings")]
    public static void SetupInputSettings()
    {
        EditorSettings.serializationMode = SerializationMode.ForceText;
        var symbols = PlayerSettings.GetScriptingDefineSymbolsForGroup(BuildTargetGroup.Standalone);
        if (!symbols.Contains("ENABLE_INPUT_SYSTEM"))
        {
            if (symbols.Length > 0) symbols += ";";
            symbols += "ENABLE_INPUT_SYSTEM";
            PlayerSettings.SetScriptingDefineSymbolsForGroup(BuildTargetGroup.Standalone, symbols);
        }
        Debug.Log("Input settings configured");
    }

    [MenuItem("Tools/MagicShard/Setup Project Settings")]
    public static void SetupProjectSettings()
    {
        PlayerSettings.companyName = "MagicShard";
        PlayerSettings.productName = "Magic Fragment";
        Debug.Log("Project settings configured");
    }
}
