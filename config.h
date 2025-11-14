#ifndef CONFIG_H
#define CONFIG_H

#include <vector>

// ---------------- Configuration ----------------
extern const int NUM_MOSQUITOES;
extern const int WINDOW_W;
extern const int WINDOW_H;
// Pond position (bottom-left)
extern const float pondX;
extern const float pondY;
extern const float pondRadiusX;
extern const float pondRadiusY;
// Water bowl position
extern float waterBowlX, waterBowlY, waterBowlRadius;
extern bool waterBowlVisible;
extern bool draggingBowl;
// Spray variables
extern bool spraying;
extern float sprayX, sprayY, sprayRadius;
extern const float maxSprayRadius;
extern const float sprayGrowth;
extern int sprayCharges; // Limited spray resource
extern const int maxSprayCharges;
extern int sprayRefillTimer;
extern const int sprayRefillInterval; // 30s at 50ms per frame

// Spawn control (frames)
extern int spawnCounter;
extern int spawnIntervalNormal;
extern int spawnIntervalHigh;
extern int currentSpawnInterval;
// Rain event
extern bool rainActive;
extern int rainTimer;
extern const int rainDuration; // 10s at 50ms
extern const int rainSpawnCount; // Mosquitoes spawned during rain
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
extern const int popupDuration;
// Histogram data (kills per minute, assuming 1200 frames = 1 minute at 50ms)
extern std::vector<int> killsPerMinute;
extern int frameCounter;
extern const int framesPerMinute;

#endif // CONFIG_H