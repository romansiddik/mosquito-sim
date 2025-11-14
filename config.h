#ifndef CONFIG_H
#define CONFIG_H

#include <vector>

// ---------------- Configuration ----------------
const int NUM_MOSQUITOES = 30;
const int WINDOW_W = 900;
const int WINDOW_H = 650;
// Pond position (bottom-left)
const float pondX = -0.7f;
const float pondY = -0.85f;
const float pondRadiusX = 0.25f;
const float pondRadiusY = 0.12f;
// Water bowl position
float waterBowlX = -0.4f, waterBowlY = -0.9f, waterBowlRadius = 0.05f;
bool waterBowlVisible = false;
bool draggingBowl = false;
// Spray variables
bool spraying = false;
float sprayX = 0.0f, sprayY = 0.0f, sprayRadius = 0.02f;
const float maxSprayRadius = 0.15f;
const float sprayGrowth = 0.018f;
int sprayCharges = 5; // Limited spray resource
const int maxSprayCharges = 5;
int sprayRefillTimer = 0;
const int sprayRefillInterval = 600; // 30s at 50ms per frame

// Spawn control (frames)
int spawnCounter = 0;
int spawnIntervalNormal = 180;
int spawnIntervalHigh = 50;
int currentSpawnInterval = spawnIntervalNormal;
// Rain event
bool rainActive = false;
int rainTimer = 0;
const int rainDuration = 200; // 10s at 50ms
const int rainSpawnCount = 5; // Mosquitoes spawned during rain
// Score
int totalAlive = 0;
int totalKilled = 0;
// Larva tracking
struct Larva {
    float x, y;
    int timer; // Frames spent near pond
};
std::vector<Larva> larvae;
// Educational popup
char popupText[256] = "";
int popupTimer = 0;
const int popupDuration = 80;
// Histogram data (kills per minute, assuming 1200 frames = 1 minute at 50ms)
std::vector<int> killsPerMinute;
int frameCounter = 0;
const int framesPerMinute = 1200;

#endif // CONFIG_H