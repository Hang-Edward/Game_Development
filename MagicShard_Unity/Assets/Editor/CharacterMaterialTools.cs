using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

public static class CharacterMaterialTools
{
    [MenuItem("Tools/MagicShard/Apply Character Materials")]
    public static void ApplyCharacterMaterials()
    {
        CharacterMaterialRepairer.RepairCharacterMaterials();

        int count = 0;
        foreach (var applier in Object.FindObjectsOfType<CharacterMaterialApplier>(true))
        {
            applier.ApplyMaterials();
            EditorUtility.SetDirty(applier);
            count++;
        }

        if (count == 0)
        {
            var player = GameObject.FindGameObjectWithTag("Player");
            if (player != null)
            {
                var applier = player.AddComponent<CharacterMaterialApplier>();
                applier.ApplyMaterials();
                EditorUtility.SetDirty(player);
                count = 1;
            }
        }

        EditorSceneManager.MarkSceneDirty(EditorSceneManager.GetActiveScene());
        Debug.Log($"Applied character materials on {count} character root(s).");
    }
}
