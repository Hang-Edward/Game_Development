using UnityEditor;
using UnityEngine;

public class CharacterMaterialPostprocessor : AssetPostprocessor
{
    private const string CharacterModelPath = "Assets/Models/Character/";
    private const string CharacterMaterialPath = "Assets/Models/Character/Materials";

    private static readonly MaterialAlias[] Aliases =
    {
        new MaterialAlias("wp_mn_kzrn_00_mi.003", "wp_mn_kzrn_00_mi.003"),
        new MaterialAlias("armor_mat1.003", "armor_mat1.003"),
        new MaterialAlias("armor_mat2.003", "armor_mat2.003"),
        new MaterialAlias("armor_mat3.003", "armor_mat3.003"),
        new MaterialAlias("armor_mat4.003", "armor_mat4.003"),
        new MaterialAlias("face_mat_1.003", "face_mat_1.003"),
        new MaterialAlias("eye_iris_1.003", "eye_iris_1.003"),
        new MaterialAlias("lash_mat_001.003", "lash_mat_001.003"),
        new MaterialAlias("hair_mat_1.003", "hair_mat_1.003"),
        new MaterialAlias("lash_mat_1_0.003", "lash_mat_1.003"),
        new MaterialAlias("Image_73", "Image_73")
    };

    private void OnPreprocessModel()
    {
        if (!assetPath.StartsWith(CharacterModelPath) || !assetPath.EndsWith(".fbx"))
            return;

        var importer = assetImporter as ModelImporter;
        if (importer == null)
            return;

        importer.animationType = ModelImporterAnimationType.Human;
        importer.avatarSetup = ModelImporterAvatarSetup.CreateFromThisModel;
        importer.importAnimation = true;
        importer.materialLocation = ModelImporterMaterialLocation.External;

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

    private Material OnAssignMaterialModel(Material material, Renderer renderer)
    {
        if (!assetPath.StartsWith(CharacterModelPath))
            return null;

        foreach (var alias in Aliases)
        {
            if (!MaterialNameMatches(material.name, alias.SourceName))
                continue;

            var replacement = AssetDatabase.LoadAssetAtPath<Material>(
                $"{CharacterMaterialPath}/{alias.TargetName}.mat");
            if (replacement != null)
                return replacement;
        }

        return null;
    }

    private static bool MaterialNameMatches(string materialName, string sourceName)
    {
        materialName = NormalizeMaterialName(materialName);
        sourceName = NormalizeMaterialName(sourceName);

        return materialName == sourceName ||
               materialName.StartsWith(sourceName) ||
               materialName.Contains(sourceName);
    }

    private static string NormalizeMaterialName(string name)
    {
        string result = name.Trim();
        result = StripSuffix(result, " (Instance)");
        result = StripSuffix(result, " (Material)");
        result = StripSuffix(result, "(Material)");
        return result.EndsWith(".mat") ? result.Substring(0, result.Length - ".mat".Length) : result;
    }

    private static string StripSuffix(string value, string suffix)
    {
        return value.EndsWith(suffix) ? value.Substring(0, value.Length - suffix.Length).TrimEnd() : value;
    }

    private readonly struct MaterialAlias
    {
        public readonly string SourceName;
        public readonly string TargetName;

        public MaterialAlias(string sourceName, string targetName)
        {
            SourceName = sourceName;
            TargetName = targetName;
        }
    }
}
