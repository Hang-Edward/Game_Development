using UnityEngine;
using UnityEngine.SceneManagement;

// Game initialization and state management
// Replaces main.cpp's initialization block
public class GameManager : MonoBehaviour
{
    [Header("Settings")]
    [SerializeField] private int targetFPS = 60;

    [Header("References")]
    [SerializeField] private GameObject playerPrefab;
    [SerializeField] private Transform spawnPoint;
    [SerializeField] private UIManager uiManager;

    private GameObject playerInstance;
    private static GameManager instance;

    private void Awake()
    {
        if (instance != null && instance != this)
        {
            Destroy(gameObject);
            return;
        }

        instance = this;
        DontDestroyOnLoad(gameObject);

        Application.targetFrameRate = targetFPS;
        QualitySettings.vSyncCount = 1;
    }

    private void Start()
    {
        SpawnPlayer();
    }

    private void SpawnPlayer()
    {
        if (playerPrefab == null)
        {
            Debug.LogWarning("Player prefab not assigned! Looking for existing player...");
            playerInstance = GameObject.FindGameObjectWithTag("Player");
            return;
        }

        Vector3 spawnPos = spawnPoint != null ? spawnPoint.position : Vector3.zero;
        playerInstance = Instantiate(playerPrefab, spawnPos, Quaternion.identity);
        playerInstance.tag = "Player";

        // Notify UI of zone
        if (uiManager != null)
            uiManager.ShowZoneText("银风森林", 3f);
    }

    public void RestartGame()
    {
        SceneManager.LoadScene(SceneManager.GetActiveScene().buildIndex);
    }

    public void QuitGame()
    {
#if UNITY_EDITOR
        UnityEditor.EditorApplication.isPlaying = false;
#else
        Application.Quit();
#endif
    }

    public static GameManager Instance => instance;
}
