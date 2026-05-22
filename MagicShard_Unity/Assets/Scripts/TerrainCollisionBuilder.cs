using UnityEngine;

[ExecuteAlways]
public class TerrainCollisionBuilder : MonoBehaviour
{
    [SerializeField] private bool addCollidersOnEnable = true;
    [SerializeField] private bool includeInactiveChildren = true;

    private void OnEnable()
    {
        if (addCollidersOnEnable)
            EnsureColliders(gameObject, includeInactiveChildren);
    }

    [ContextMenu("Ensure Terrain Colliders")]
    public void EnsureColliders()
    {
        EnsureColliders(gameObject, includeInactiveChildren);
    }

    public static int EnsureSceneTerrainColliders()
    {
        int count = 0;
        var terrain = GameObject.Find("Terrain");
        if (terrain != null)
            count += EnsureColliders(terrain, true);

        foreach (var marker in GameObject.FindGameObjectsWithTag("Untagged"))
        {
            if (marker.name.StartsWith("map_", System.StringComparison.OrdinalIgnoreCase))
                count += EnsureColliders(marker, true);
        }

        return count;
    }

    public static int EnsureColliders(GameObject root, bool includeInactive = true)
    {
        if (root == null)
            return 0;

        int added = 0;
        var meshFilters = root.GetComponentsInChildren<MeshFilter>(includeInactive);
        foreach (var meshFilter in meshFilters)
        {
            if (meshFilter.sharedMesh == null)
                continue;

            if (!LooksLikeTerrainMesh(meshFilter.sharedMesh.bounds))
                continue;

            var meshCollider = meshFilter.GetComponent<MeshCollider>();
            if (meshCollider == null)
            {
                meshCollider = meshFilter.gameObject.AddComponent<MeshCollider>();
                added++;
            }

            meshCollider.sharedMesh = meshFilter.sharedMesh;
            meshCollider.convex = false;
            meshCollider.isTrigger = false;
            meshCollider.enabled = true;
        }

        return added;
    }

    private static bool LooksLikeTerrainMesh(Bounds bounds)
    {
        return bounds.size.x > 0.01f || bounds.size.z > 0.01f;
    }
}
