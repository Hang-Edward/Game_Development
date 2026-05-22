using UnityEngine;
using UnityEngine.InputSystem;

public class CameraController : MonoBehaviour
{
    [Header("Target")]
    [SerializeField] private Transform target;
    [SerializeField] private Vector3 targetOffset = new Vector3(0, 0.65f, 0);

    [Header("Orbit")]
    [SerializeField] private float distance = 6f;
    [SerializeField] private float minDistance = 2f;
    [SerializeField] private float maxDistance = 12f;
    [SerializeField] private float mouseSensitivity = 0.2f;
    [SerializeField] private float scrollSpeed = 0.08f;
    [SerializeField] private float yaw;
    [SerializeField] private float pitch = 25f;
    [SerializeField] private float minPitch = -30f;
    [SerializeField] private float maxPitch = 85f;

    [Header("Smoothing")]
    [SerializeField] private bool smooth = true;
    [SerializeField] private float smoothTime = 0.08f;

    private Vector3 smoothVelocity;
    private bool cursorCaptured = true;

    private void Awake()
    {
        if (target == null)
        {
            var player = GameObject.FindGameObjectWithTag("Player");
            if (player != null) target = player.transform;
        }
        Cursor.lockState = CursorLockMode.Locked;
        Cursor.visible = false;
    }

    private void LateUpdate()
    {
        if (target == null) return;

        // Read mouse input directly
        if (cursorCaptured)
        {
            Vector2 delta = Mouse.current.delta.ReadValue();
            yaw += delta.x * mouseSensitivity;
            pitch -= delta.y * mouseSensitivity;
            float scrollDelta = Mouse.current.scroll.ReadValue().y;
            if (Mathf.Abs(scrollDelta) > 0.01f)
                distance -= scrollDelta * scrollSpeed;
        }

        // ESC to toggle cursor
        if (Keyboard.current.escapeKey.wasPressedThisFrame && cursorCaptured)
        {
            cursorCaptured = false;
            Cursor.lockState = CursorLockMode.None;
            Cursor.visible = true;
        }
        else if (Mouse.current.leftButton.wasPressedThisFrame && !cursorCaptured)
        {
            cursorCaptured = true;
            Cursor.lockState = CursorLockMode.Locked;
            Cursor.visible = false;
        }

        if (!cursorCaptured) return;

        pitch = Mathf.Clamp(pitch, minPitch, maxPitch);
        distance = Mathf.Clamp(distance, minDistance, maxDistance);

        Quaternion rotation = Quaternion.Euler(pitch, yaw, 0);
        Vector3 targetPosition = target.position + targetOffset;
        Vector3 desiredPosition = targetPosition - (rotation * Vector3.forward * distance);

        if (smooth)
            transform.position = Vector3.SmoothDamp(transform.position, desiredPosition, ref smoothVelocity, smoothTime);
        else
            transform.position = desiredPosition;

        transform.LookAt(targetPosition);
    }

    public bool IsCursorCaptured() => cursorCaptured;
}
