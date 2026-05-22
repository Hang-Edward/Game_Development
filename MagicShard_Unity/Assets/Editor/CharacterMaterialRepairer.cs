using System.Collections.Generic;
using UnityEditor;
using UnityEngine;

[InitializeOnLoad]
public static class CharacterMaterialRepairer
{
    private const string MaterialPath = "Assets/Models/Character/Materials";
    private const string TexturePath = "Assets/Models/Character/stand.fbm";

    private static readonly Dictionary<string, string> MaterialToTexture = new Dictionary<string, string>
    {
        { "Image_0", "Image_0" },
        { "Image_2", "Image_2" },
        { "Image_3", "Image_3" },
        { "Image_5", "Image_5" },
        { "Image_7", "Image_7" },
        { "Image_8", "Image_8" },
        { "Image_10", "Image_10" },
        { "Image_12", "Image_12" },
        { "Image_14", "Image_14" },
        { "Image_16", "Image_16" },
        { "Image_17", "Image_17" },
        { "Image_19", "Image_19" },
        { "Image_21", "Image_21" },
        { "Image_22", "Image_22" },
        { "Image_24", "Image_24" },
        { "Image_25", "Image_25" },
        { "Image_26", "Image_26" },
        { "Image_27", "Image_27" },
        { "Image_28", "Image_28" },
        { "Image_29", "Image_29" },
        { "Image_73", "Image_29" },

        { "wp_mn_kzrn_00_mi.003", "Image_0" },
        { "armor_mat1.003", "Image_5" },
        { "armor_mat2.003", "Image_10" },
        { "armor_mat3.003", "Image_14" },
        { "armor_mat4.003", "Image_19" },
        { "face_mat_1.003", "Image_24" },
        { "eye_iris_1.003", "Image_26" },
        { "lash_mat_001.003", "Image_24" },
        { "hair_mat_1.003", "Image_29" }
    };

    static CharacterMaterialRepairer()
    {
        EditorApplication.delayCall += RepairCharacterMaterials;
    }

    [MenuItem("Tools/MagicShard/Repair Character Material Assets")]
    public static void RepairCharacterMaterials()
    {
        var standardShader = Shader.Find("Standard");
        if (standardShader == null)
        {
            Debug.LogError("Standard shader not found.");
            return;
        }

        int repaired = 0;
        foreach (var pair in MaterialToTexture)
        {
            var material = LoadOrCreateMaterial(pair.Key, standardShader);
            var texture = AssetDatabase.LoadAssetAtPath<Texture2D>($"{TexturePath}/{pair.Value}.png");
            if (material == null || texture == null)
            {
                Debug.LogWarning($"Missing material or texture for {pair.Key} -> {pair.Value}");
                continue;
            }

            Undo.RecordObject(material, "Repair Character Material");
            material.shader = standardShader;
            material.SetTexture("_MainTex", texture);
            material.SetColor("_Color", Color.white);
            material.SetFloat("_Metallic", 0f);
            material.SetFloat("_Glossiness", 0.5f);
            material.DisableKeyword("_EMISSION");
            material.globalIlluminationFlags = MaterialGlobalIlluminationFlags.EmissiveIsBlack;
            EditorUtility.SetDirty(material);
            repaired++;
        }

        AssetDatabase.SaveAssets();
        AssetDatabase.Refresh();
        Debug.Log($"Repaired {repaired} character material asset(s).");
    }

    private static Material LoadOrCreateMaterial(string materialName, Shader shader)
    {
        string path = $"{MaterialPath}/{materialName}.mat";
        var material = AssetDatabase.LoadAssetAtPath<Material>(path);
        if (material != null)
            return material;

        material = new Material(shader)
        {
            name = materialName
        };
        AssetDatabase.CreateAsset(material, path);
        return material;
    }
}
