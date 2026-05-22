using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;

public static class TerrainCollisionTools
{
    [MenuItem("Tools/MagicShard/Repair Terrain Colliders")]
    public static void RepairTerrainColliders()
    {
        int added = 0;
        var terrain = GameObject.Find("Terrain");
        if (terrain != null)
        {
            if (terrain.GetComponent<TerrainCollisionBuilder>() == null)
                terrain.AddComponent<TerrainCollisionBuilder>();

            added += TerrainCollisionBuilder.EnsureColliders(terrain, true);
            EditorUtility.SetDirty(terrain);
        }

        EditorSceneManager.MarkSceneDirty(EditorSceneManager.GetActiveScene());
        Debug.Log($"Terrain collider repair complete. Added {added} MeshCollider(s).");
    }
}
