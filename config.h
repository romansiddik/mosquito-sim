#ifndef CONFIG_H
#define CONFIG_H

#include <vector>

// ---------------- Configuration ----------------
constexpr int NUM_MOSQUITOES = 30;
constexpr int WINDOW_W = 900;
constexpr int WINDOW_H = 650;
// Pond position (bottom-left)
constexpr float pondX = -0.7f;
constexpr float pondY = -0.85f;
constexpr float pondRadiusX = 0.25f;
constexpr float pondRadiusY = 0.12f;
// Water bowl position
extern float waterBowlX, waterBowlY, waterBowlRadius;
extern bool waterBowlVisible;
extern bool draggingBowl;
// Spray variables
extern bool spraying;
extern float sprayX, sprayY, sprayRadius;
constexpr float maxSprayRadius = 0.15f;
constexpr float sprayGrowth = 0.018f;
extern int sprayCharges; // Limited spray resource
constexpr int maxSprayCharges = 5;
extern int sprayRefillTimer;
constexpr int sprayRefillInterval = 600; // 30s at 50ms per frame

// Spawn control (frames)
extern int spawnCounter;
extern int spawnIntervalNormal;
extern int spawnIntervalHigh;
extern int currentSpawnInterval;
// Rain event
extern bool rainActive;
extern int rainTimer;
constexpr int rainDuration = 200; // 10s at 50ms
constexpr int rainSpawnCount = 5; // Mosquitoes spawned during rain
// Score
extern int totalAlive;
extern int totalKilled;
// Larva tracking
struct Larva {
    float x, y;
    int timer; // Frames spent near pond
};
extern std::vector<Larva> larvae;
// Educational popup
extern char popupText[256];
extern int popupTimer;
constexpr int popupDuration = 80;
// Histogram data (kills per minute, assuming 1200 frames = 1 minute at 50ms)
extern std::vector<int> killsPerMinute;
extern int frameCounter;
constexpr int framesPerMinute = 1200;

#endif // CONFIG_H