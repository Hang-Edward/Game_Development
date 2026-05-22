using UnityEngine;
using UnityEditor;
using UnityEngine.SceneManagement;

public class SceneVerifier : EditorWindow
{
    [MenuItem("Tools/MagicShard/Verify Scene")]
    public static void VerifyScene()
    {
        var scene = SceneManager.GetActiveScene();
        if (scene == null || !scene.isLoaded)
        {
            Debug.LogError("No scene loaded!");
            return;
        }

        Debug.Log($"=== Verifying Scene: {scene.name} ===");
        int pass = 0, fail = 0;

        // 1. Check Player
        var player = GameObject.FindGameObjectWithTag("Player");
        if (player != null)
        {
            pass++;
            Debug.Log($"[PASS] Player found at position {player.transform.position}");

            var cc = player.GetComponent<CharacterController>();
            if (cc != null) { pass++; Debug.Log($"[PASS] CharacterController: height={cc.height}"); }
            else { fail++; Debug.LogError("[FAIL] Missing CharacterController"); }

            var pi = player.GetComponent<UnityEngine.InputSystem.PlayerInput>();
            if (pi != null && pi.actions != null) { pass++; Debug.Log($"[PASS] PlayerInput: {pi.actions.name}"); }
            else { fail++; Debug.LogError("[FAIL] Missing PlayerInput or actions"); }

            var pc = player.GetComponent<PlayerController>();
            if (pc != null) { pass++; Debug.Log("[PASS] PlayerController"); }
            else { fail++; Debug.LogError("[FAIL] Missing PlayerController"); }

            // Check character model child
            var model = player.transform.Find("CharacterModel");
            if (model != null)
            {
                pass++;
                var animator = model.GetComponent<Animator>();
                if (animator != null && animator.runtimeAnimatorController != null)
                {
                    pass++;
                    Debug.Log($"[PASS] Animator: {animator.runtimeAnimatorController.name}");
                }
                else { fail++; Debug.LogError("[FAIL] Animator missing or no controller"); }
            }
            else { fail++; Debug.LogError("[FAIL] CharacterModel not found as child"); }
        }
        else { fail++; Debug.LogError("[FAIL] Player not found!"); }

        // 2. Check Camera
        var cam = Camera.main;
        if (cam != null)
        {
            pass++;
            var camCtrl = cam.GetComponent<CameraController>();
            if (camCtrl != null) { pass++; Debug.Log("[PASS] CameraController on MainCamera"); }
            else { fail++; Debug.LogError("[FAIL] CameraController missing on MainCamera"); }

            Debug.Log("[INFO] MainCamera does not require PlayerInput; input is read by PlayerController/CameraController.");
        }
        else { fail++; Debug.LogError("[FAIL] Main Camera not found!"); }

        // 3. Check Terrain
        var terrain = GameObject.Find("Terrain");
        if (terrain != null)
        {
            pass++;
            int colliderCount = 0;
            foreach (var mf in terrain.GetComponentsInChildren<MeshCollider>())
                colliderCount++;
            if (colliderCount > 0)
            {
                pass++;
                Debug.Log($"[PASS] Terrain has {colliderCount} MeshColliders");
            }
            else
            {
                fail++;
                Debug.LogError("[FAIL] Terrain has no MeshColliders");
            }

            int meshCount = 0;
            foreach (var mf in terrain.GetComponentsInChildren<MeshRenderer>())
                meshCount++;
            Debug.Log($"[INFO] Terrain has {meshCount} MeshRenderers");
        }
        else { fail++; Debug.LogError("[FAIL] Terrain not found!"); }

        // 4. Check UI Canvas
        var canvas = GameObject.Find("Canvas");
        if (canvas != null)
        {
            pass++;
            var uiManager = canvas.GetComponent<UIManager>();
            if (uiManager != null) { pass++; Debug.Log("[PASS] UIManager on Canvas"); }
            else { fail++; Debug.LogError("[FAIL] UIManager missing on Canvas"); }

            var fpsText = canvas.transform.Find("FPS_Text");
            if (fpsText != null) { pass++; Debug.Log("[PASS] FPS_Text found"); }
            else { fail++; Debug.LogError("[FAIL] FPS_Text missing"); }
        }
        else { fail++; Debug.LogError("[FAIL] Canvas not found!"); }

        // 5. Check Materials (not pink)
        int pinkCount = 0;
        foreach (var renderer in Object.FindObjectsOfType<Renderer>(true))
        {
            foreach (var mat in renderer.sharedMaterials)
            {
                if (mat != null && mat.shader != null &&
                    (mat.shader.name == "Hidden/InternalErrorShader" || mat.color == Color.magenta))
                {
                    pinkCount++;
                    Debug.LogWarning($"[WARN] Pink material: {mat.name} on {renderer.gameObject.name}");
                }
            }
        }
        if (pinkCount == 0)
        {
            pass++;
            Debug.Log("[PASS] No pink/missing materials");
        }
        else
        {
            fail++;
            Debug.LogWarning($"[WARN] {pinkCount} pink/missing materials found");
        }

        Debug.Log($"=== RESULT: {pass}/{pass+fail} passed, {fail} failed ===");
    }
}
