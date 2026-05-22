using UnityEngine;
using TMPro;

// Replaces main.cpp's DrawText/DrawTextEx calls
public class UIManager : MonoBehaviour
{
    [Header("UI References")]
    [SerializeField] private TextMeshProUGUI fpsText;
    [SerializeField] private TextMeshProUGUI animInfoText;
    [SerializeField] private TextMeshProUGUI controlsHint;
    [SerializeField] private GameObject centerPrompt;
    [SerializeField] private TextMeshProUGUI centerPromptText;
    [SerializeField] private TextMeshProUGUI zoneText;

    [Header("Settings")]
    [SerializeField] private float fpsUpdateInterval = 0.5f;

    private CameraController cameraController;
    private Animator playerAnimator;
    private float fpsTimer;

    private void Start()
    {
        cameraController = FindObjectOfType<CameraController>();

        GameObject player = GameObject.FindGameObjectWithTag("Player");
        if (player != null)
            playerAnimator = player.GetComponentInChildren<Animator>();

        if (controlsHint != null)
            controlsHint.text = "[WASD] Move [Shift] Sprint [Ctrl] Crouch [LMB] Attack [RMB] Block [Wheel] Zoom";

        if (centerPromptText != null)
            centerPromptText.text = "Click to start";
    }

    private void Update()
    {
        fpsTimer += Time.deltaTime;
        if (fpsTimer >= fpsUpdateInterval && fpsText != null)
        {
            fpsText.text = $"FPS: {Mathf.RoundToInt(1f / Time.unscaledDeltaTime)}";
            fpsTimer = 0f;
        }

        if (animInfoText != null && playerAnimator != null)
        {
            AnimatorStateInfo state = playerAnimator.GetCurrentAnimatorStateInfo(0);
            float speed = playerAnimator.GetFloat("Speed");
            animInfoText.text = $"State: {state.shortNameHash} Speed: {speed:F2} ({state.normalizedTime:P0})";
        }

        if (centerPrompt != null && cameraController != null)
        {
            centerPrompt.SetActive(!cameraController.IsCursorCaptured());
        }
    }

    public void ShowZoneText(string zoneName, float duration = 3f)
    {
        if (zoneText != null)
        {
            zoneText.text = zoneName;
            CancelInvoke(nameof(HideZoneText));
            Invoke(nameof(HideZoneText), duration);
        }
    }

    private void HideZoneText()
    {
        if (zoneText != null)
            zoneText.text = "";
    }
}
