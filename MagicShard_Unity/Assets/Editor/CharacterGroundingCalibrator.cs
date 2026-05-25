using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

public static class CharacterGroundingCalibrator
{
    private const string ScenePath = "Assets/Scenes/GameWorld.unity";
    private const string CharacterPrefabPath = "Assets/Models/Character/stand.fbx";
    private const string IdleClipPath = "Assets/Models/Character/stand.fbx";
    private const string WalkClipPath = "Assets/Models/Character/walk.fbx";
    private const string RunClipPath = "Assets/Models/Character/run.fbx";

    [MenuItem("Tools/MagicShard/Calibrate Character Grounding")]
    public static void CalibrateCharacterGrounding()
    {
        ConfigureCharacterImports();

        AnimationClip idle = LoadClip(IdleClipPath, "Idle");
        AnimationClip walk = LoadClip(WalkClipPath, "Walk");
        AnimationClip run = LoadClip(RunClipPath, "Run");
        if (idle == null || walk == null || run == null)
        {
            Debug.LogError($"Grounding calibration failed. Idle={idle != null}, Walk={walk != null}, Run={run != null}");
            return;
        }

        GameObject prefab = AssetDatabase.LoadAssetAtPath<GameObject>(CharacterPrefabPath);
        if (prefab == null)
        {
            Debug.LogError($"Grounding calibration failed. Missing character prefab: {CharacterPrefabPath}");
            return;
        }

        GameObject probe = null;
        try
        {
            probe = (GameObject)PrefabUtility.InstantiatePrefab(prefab);
            if (probe == null)
                probe = Object.Instantiate(prefab);

            probe.hideFlags = HideFlags.HideAndDontSave;
            probe.transform.position = Vector3.zero;
            probe.transform.rotation = Quaternion.identity;
            probe.transform.localScale = Vector3.one;

            float idleFootY = MeasureLowestFootY(probe, idle);
            float walkFootY = MeasureLowestFootY(probe, walk);
            float runFootY = MeasureLowestFootY(probe, run);

            float walkLift = ComputeLift(idleFootY, walkFootY);
            float runLift = ComputeLift(idleFootY, runFootY);

            ApplySceneLiftValues(walkLift, runLift);
            Debug.Log($"Character grounding calibrated. IdleFootY={idleFootY:F3}, WalkFootY={walkFootY:F3}, RunFootY={runFootY:F3}, WalkLift={walkLift:F3}, RunLift={runLift:F3}");
        }
        finally
        {
            if (probe != null)
                Object.DestroyImmediate(probe);
        }
    }

    public static void CalibrateCharacterGroundingBatch()
    {
        CalibrateCharacterGrounding();
    }

    private static void ConfigureCharacterImports()
    {
        foreach (string path in new[] { IdleClipPath, WalkClipPath, RunClipPath })
        {
            var importer = AssetImporter.GetAtPath(path) as ModelImporter;
            if (importer == null)
                continue;

            importer.animationType = ModelImporterAnimationType.Human;
            importer.avatarSetup = ModelImporterAvatarSetup.CreateFromThisModel;
            importer.importAnimation = true;
            importer.materialLocation = ModelImporterMaterialLocation.External;

            ModelImporterClipAnimation[] clips = importer.clipAnimations;
            if (clips == null || clips.Length == 0)
                clips = importer.defaultClipAnimations;

            foreach (var clip in clips)
            {
                clip.lockRootRotation = true;
                clip.keepOriginalOrientation = true;
                clip.lockRootHeightY = true;
                clip.keepOriginalPositionY = false;
                clip.heightFromFeet = true;
                clip.lockRootPositionXZ = true;
                clip.keepOriginalPositionXZ = true;
                clip.loopPose = true;
            }

            importer.clipAnimations = clips;
            importer.SaveAndReimport();
        }
    }

    private static AnimationClip LoadClip(string path, string expectedName)
    {
        return AssetDatabase.LoadAllAssetsAtPath(path)
            .OfType<AnimationClip>()
            .Where(c => !c.name.StartsWith("__preview__", System.StringComparison.OrdinalIgnoreCase))
            .FirstOrDefault(c => c.name == expectedName) ??
            AssetDatabase.LoadAllAssetsAtPath(path)
                .OfType<AnimationClip>()
                .FirstOrDefault(c => !c.name.StartsWith("__preview__", System.StringComparison.OrdinalIgnoreCase));
    }

    private static float MeasureLowestFootY(GameObject probe, AnimationClip clip)
    {
        float lowestY = float.PositiveInfinity;
        int sampleCount = Mathf.Max(8, Mathf.CeilToInt(clip.length * 30f));

        for (int i = 0; i <= sampleCount; i++)
        {
            float time = sampleCount == 0 ? 0f : clip.length * i / sampleCount;
            clip.SampleAnimation(probe, time);

            foreach (Transform foot in FindFootTransforms(probe))
            {
                float localY = probe.transform.InverseTransformPoint(foot.position).y;
                if (localY < lowestY)
                    lowestY = localY;
            }
        }

        if (float.IsPositiveInfinity(lowestY))
            throw new System.InvalidOperationException("No foot/toe bones were found while calibrating character grounding.");

        return lowestY;
    }

    private static IEnumerable<Transform> FindFootTransforms(GameObject root)
    {
        Animator animator = root.GetComponentInChildren<Animator>(true);
        if (animator != null && animator.isHuman)
        {
            foreach (HumanBodyBones bone in new[] { HumanBodyBones.LeftFoot, HumanBodyBones.RightFoot, HumanBodyBones.LeftToes, HumanBodyBones.RightToes })
            {
                Transform t = animator.GetBoneTransform(bone);
                if (t != null)
                    yield return t;
            }
        }

        foreach (Transform t in root.GetComponentsInChildren<Transform>(true))
        {
            string name = t.name.ToLowerInvariant();
            if ((name.Contains("foot") || name.Contains("toe")) && !name.Contains("scalecompensation"))
                yield return t;
        }
    }

    private static float ComputeLift(float idleFootY, float clipFootY)
    {
        return Mathf.Clamp(idleFootY - clipFootY, -0.5f, 1.5f);
    }

    private static void ApplySceneLiftValues(float walkLift, float runLift)
    {
        Scene scene = EditorSceneManager.GetActiveScene();
        if (!scene.IsValid() || scene.path != ScenePath)
            scene = EditorSceneManager.OpenScene(ScenePath, OpenSceneMode.Single);

        GameObject player = GameObject.FindGameObjectWithTag("Player");
        if (player == null)
            throw new FileNotFoundException("Player object with tag Player was not found in GameWorld.");

        PlayerController controller = player.GetComponent<PlayerController>();
        if (controller == null)
            throw new MissingComponentException("PlayerController was not found on Player.");

        SerializedObject serialized = new SerializedObject(controller);
        serialized.FindProperty("walkVisualLift").floatValue = walkLift;
        serialized.FindProperty("runVisualLift").floatValue = runLift;
        serialized.FindProperty("visualLiftSmoothSpeed").floatValue = 14f;
        serialized.ApplyModifiedPropertiesWithoutUndo();

        EditorUtility.SetDirty(controller);
        EditorSceneManager.MarkSceneDirty(scene);
        EditorSceneManager.SaveScene(scene);
        AssetDatabase.SaveAssets();
    }
}
