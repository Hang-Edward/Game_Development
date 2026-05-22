using System.IO;
using UnityEngine;

#if UNITY_EDITOR
using UnityEditor;
#endif

[ExecuteAlways]
public class CharacterMaterialApplier : MonoBehaviour
{
    private const string CharacterMaterialPath = "Assets/Models/Character/Materials";
    private int applyAttempts;
    private static readonly string[] DiffuseFallbackMaterials =
    {
        "Image_5",
        "Image_14",
        "Image_19",
        "Image_10",
        "Image_0",
        "Image_29"
    };

    private static readonly MaterialAlias[] ExplicitMaterialAliases =
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

    private void Awake()
    {
        ApplyMaterials();
    }

    private void OnEnable()
    {
        ApplyMaterials();
    }

    private void LateUpdate()
    {
        if (applyAttempts >= 10)
            return;

        applyAttempts++;
        ApplyMaterials();
    }

    [ContextMenu("Apply Character Materials")]
    public void ApplyMaterials()
    {
#if UNITY_EDITOR
        foreach (var renderer in GetComponentsInChildren<Renderer>(true))
        {
            var materials = renderer.sharedMaterials;
            bool changed = false;

            for (int i = 0; i < materials.Length; i++)
            {
                var material = materials[i];
                if (material == null)
                    continue;

                string materialName = CleanMaterialName(material.name);
                var replacement = LoadCharacterMaterial(materialName);
                if (replacement == null && (LooksLikeGeneratedMaterialName(materialName) || LooksLikeUntexturedWhiteMaterial(material)))
                    replacement = LoadFallbackDiffuseMaterial(i);

                if (replacement == null)
                    continue;

                if (materials[i] != replacement)
                {
                    materials[i] = replacement;
                    changed = true;
                }
            }

            if (changed)
            {
                renderer.sharedMaterials = materials;
                EditorUtility.SetDirty(renderer);
            }
        }
#endif
    }

#if UNITY_EDITOR
    private static Material LoadCharacterMaterial(string materialName)
    {
        materialName = NormalizeMaterialName(materialName);

        foreach (var alias in ExplicitMaterialAliases)
        {
            if (MaterialNameMatches(materialName, alias.SourceName))
                return AssetDatabase.LoadAssetAtPath<Material>($"{CharacterMaterialPath}/{alias.TargetName}.mat");
        }

        string directPath = $"{CharacterMaterialPath}/{materialName}.mat";
        var material = AssetDatabase.LoadAssetAtPath<Material>(directPath);
        if (material != null)
            return material;

        if (materialName == "Image_73")
            return AssetDatabase.LoadAssetAtPath<Material>($"{CharacterMaterialPath}/Image_29.mat");

        if (materialName == "lash_mat_1_0.003")
            return AssetDatabase.LoadAssetAtPath<Material>($"{CharacterMaterialPath}/lash_mat_1.003.mat");

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

    private static Material LoadFallbackDiffuseMaterial(int materialIndex)
    {
        string name = DiffuseFallbackMaterials[materialIndex % DiffuseFallbackMaterials.Length];
        return AssetDatabase.LoadAssetAtPath<Material>($"{CharacterMaterialPath}/{name}.mat");
    }

    private static bool LooksLikeUntexturedWhiteMaterial(Material material)
    {
        Texture mainTexture = null;
        if (material.HasProperty("_MainTex"))
            mainTexture = material.GetTexture("_MainTex");
        if (mainTexture == null && material.HasProperty("_BaseMap"))
            mainTexture = material.GetTexture("_BaseMap");

        if (mainTexture != null)
            return false;

        Color color = Color.white;
        if (material.HasProperty("_Color"))
            color = material.GetColor("_Color");
        else if (material.HasProperty("_BaseColor"))
            color = material.GetColor("_BaseColor");

        return color.r > 0.85f && color.g > 0.85f && color.b > 0.85f;
    }

    private static bool LooksLikeGeneratedMaterialName(string materialName)
    {
        return materialName.StartsWith("Material") ||
               materialName.StartsWith("Default") ||
               materialName.StartsWith("lambert") ||
               materialName.StartsWith("phong");
    }

    private static string CleanMaterialName(string name)
    {
        return NormalizeMaterialName(name);
    }

    private static string NormalizeMaterialName(string name)
    {
        string result = name.Trim();
        result = StripSuffix(result, " (Instance)");
        result = StripSuffix(result, " (Material)");
        result = StripSuffix(result, "(Material)");

        string extension = Path.GetExtension(result);
        if (extension == ".mat")
            result = Path.GetFileNameWithoutExtension(result);

        return result;
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
#endif
}
