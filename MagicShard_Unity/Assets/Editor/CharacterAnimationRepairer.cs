using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.Animations;
using UnityEngine;

public static class CharacterAnimationRepairer
{
    private const string ControllerPath = "Assets/Animations/Controllers/CharacterAnimator.controller";
    private static readonly string[] CharacterFbxPaths =
    {
        "Assets/Models/Character/stand.fbx",
        "Assets/Models/Character/walk.fbx",
        "Assets/Models/Character/run.fbx"
    };

    [MenuItem("Tools/MagicShard/Repair Character Animator")]
    public static void RepairCharacterAnimator()
    {
        ConfigureModelImports();
        var idle = LoadClip("Assets/Models/Character/stand.fbx", "Idle");
        var walk = LoadClip("Assets/Models/Character/walk.fbx", "Walk");
        var run = LoadClip("Assets/Models/Character/run.fbx", "Run");

        if (idle == null || walk == null || run == null)
        {
            Debug.LogError($"Missing animation clip(s). Idle={idle != null}, Walk={walk != null}, Run={run != null}");
            return;
        }

        Directory.CreateDirectory("Assets/Animations/Controllers");
        var controller = AssetDatabase.LoadAssetAtPath<AnimatorController>(ControllerPath);
        if (controller == null)
            controller = AnimatorController.CreateAnimatorControllerAtPath(ControllerPath);

        RebuildController(controller, idle, walk, run);
        AssignControllerToSceneCharacter(controller);

        AssetDatabase.SaveAssets();
        AssetDatabase.Refresh();
        Debug.Log("Character Animator repaired: Idle/Walk/Run blend tree is ready.");
    }

    private static void ConfigureModelImports()
    {
        foreach (var path in CharacterFbxPaths)
        {
            var importer = AssetImporter.GetAtPath(path) as ModelImporter;
            if (importer == null)
                continue;

            importer.animationType = ModelImporterAnimationType.Human;
            importer.avatarSetup = ModelImporterAvatarSetup.CreateFromThisModel;
            importer.importAnimation = true;
            importer.materialLocation = ModelImporterMaterialLocation.External;
            ConfigureInPlaceClips(importer);
            importer.SaveAndReimport();
        }
    }

    private static void ConfigureInPlaceClips(ModelImporter importer)
    {
        var clips = importer.clipAnimations;
        if (clips == null || clips.Length == 0)
            clips = importer.defaultClipAnimations;

        foreach (var clip in clips)
        {
            clip.lockRootRotation = true;
            clip.keepOriginalOrientation = true;
            clip.lockRootHeightY = true;
            clip.keepOriginalPositionY = true;
            clip.heightFromFeet = true;
            clip.lockRootPositionXZ = true;
            clip.keepOriginalPositionXZ = true;
            clip.loopPose = true;
        }

        importer.clipAnimations = clips;
    }

    private static AnimationClip LoadClip(string path, string expectedName)
    {
        var clips = AssetDatabase.LoadAllAssetsAtPath(path)
            .OfType<AnimationClip>()
            .Where(c => !c.name.StartsWith("__preview__", System.StringComparison.OrdinalIgnoreCase))
            .ToArray();

        return clips.FirstOrDefault(c => c.name == expectedName) ??
               clips.FirstOrDefault(c => c.name.ToLowerInvariant().Contains(expectedName.ToLowerInvariant())) ??
               clips.FirstOrDefault();
    }

    private static void RebuildController(AnimatorController controller, AnimationClip idle, AnimationClip walk, AnimationClip run)
    {
        EnsureParameter(controller, "Speed", AnimatorControllerParameterType.Float);
        EnsureParameter(controller, "Sprint", AnimatorControllerParameterType.Bool);
        EnsureParameter(controller, "Crouch", AnimatorControllerParameterType.Bool);
        EnsureParameter(controller, "Block", AnimatorControllerParameterType.Bool);
        EnsureParameter(controller, "Grounded", AnimatorControllerParameterType.Bool);
        EnsureParameter(controller, "Attack", AnimatorControllerParameterType.Trigger);
        EnsureParameter(controller, "MotionSpeed", AnimatorControllerParameterType.Float);

        var layer = controller.layers[0];
        var stateMachine = layer.stateMachine;
        foreach (var state in stateMachine.states.ToArray())
            stateMachine.RemoveState(state.state);

        var blendTree = new BlendTree
        {
            name = "Locomotion",
            blendType = BlendTreeType.Simple1D,
            blendParameter = "Speed",
            useAutomaticThresholds = false,
            minThreshold = 0f,
            maxThreshold = 1f
        };

        AssetDatabase.AddObjectToAsset(blendTree, controller);
        blendTree.AddChild(idle, 0f);
        blendTree.AddChild(walk, 0.55f);
        blendTree.AddChild(run, 1f);

        var locomotion = stateMachine.AddState("Locomotion");
        locomotion.motion = blendTree;
        locomotion.writeDefaultValues = true;
        stateMachine.defaultState = locomotion;

        EditorUtility.SetDirty(controller);
    }

    private static void EnsureParameter(AnimatorController controller, string name, AnimatorControllerParameterType type)
    {
        var existing = controller.parameters.FirstOrDefault(p => p.name == name);
        if (existing != null)
            return;

        controller.AddParameter(name, type);
    }

    private static void AssignControllerToSceneCharacter(RuntimeAnimatorController controller)
    {
        var player = GameObject.FindGameObjectWithTag("Player");
        if (player == null)
            return;

        var animator = player.GetComponentInChildren<Animator>(true);
        if (animator == null)
            return;

        animator.runtimeAnimatorController = controller;
        animator.applyRootMotion = false;
        EditorUtility.SetDirty(animator);
    }
}
