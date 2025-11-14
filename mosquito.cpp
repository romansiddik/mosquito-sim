#include "mosquito.h"
#include "config.h"
#include "utils.h"
#include <cmath>
#include <ctime>
#include <cstdio>

Mosquito mosquitoes[NUM_MOSQUITOES];

// Initialize mosquitoes
void initializeMosquitoes() {
    srand(static_cast<unsigned>(time(0)));
    totalAlive = 0;
    totalKilled = 0;
    larvae.clear();
    killsPerMinute.clear();
    killsPerMinute.push_back(0);
    sprayCharges = maxSprayCharges;
    rainActive = false;
    rainTimer = 0;
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
        mosquitoes[i].x = randFloat(-0.9f, 0.9f);
        mosquitoes[i].y = randFloat(-0.9f, 0.9f);
        mosquitoes[i].dx = randFloat(-0.003f, 0.003f);
        mosquitoes[i].dy = randFloat(-0.003f, 0.003f);
        mosquitoes[i].size = randFloat(0.035f, 0.05f);
        mosquitoes[i].alive = (rand() % 2 == 0);
        mosquitoes[i].deadTimer = 0;
        mosquitoes[i].attractedToPond = (rand() % 3 == 0);
        mosquitoes[i].pondTime = 0;
        if (mosquitoes[i].alive) totalAlive++;
    }
}

// ---------------- Logic ----------------
bool isNearPondArea(float x, float y) {
    float rx = pondRadiusX * 1.8f;
    float ry = pondRadiusY * 2.5f;
    float nx = (x - pondX) / rx;
    float ny = (y - pondY) / ry;
    return (nx * nx + ny * ny) <= 1.0f;
}
bool isNearWaterBowl(float x, float y) {
    if (!waterBowlVisible) return false;
    float dx = x - waterBowlX;
    float dy = y - waterBowlY;
    return sqrtf(dx*dx + dy*dy) <= waterBowlRadius * 2.0f;
}
void spawnOneMosquito(bool pondBoost) {
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
        if (!mosquitoes[i].alive) {
            bool useBowl = waterBowlVisible && (rand() % 2 == 0);
            float angle = randFloat(0.0f, 2.0f * 3.1415926f);
            if (pondBoost && useBowl) {
                mosquitoes[i].x = waterBowlX + cosf(angle) * waterBowlRadius * 1.2f;
                mosquitoes[i].y = waterBowlY + sinf(angle) * waterBowlRadius * 1.2f;
            } else if (pondBoost) {
                float rx = pondRadiusX * 0.9f + randFloat(0.0f, 0.04f);
                float ry = pondRadiusY * 0.9f + randFloat(0.0f, 0.03f);
                mosquitoes[i].x = pondX + cosf(angle) * rx;
                mosquitoes[i].y = pondY + sinf(angle) * ry;
            } else {
                mosquitoes[i].x = randFloat(-0.9f, 0.9f);
                mosquitoes[i].y = randFloat(-0.9f, 0.9f);
            }
            mosquitoes[i].dx = randFloat(-0.004f, 0.004f);
            mosquitoes[i].dy = randFloat(-0.004f, 0.004f);
            mosquitoes[i].size = randFloat(0.035f, 0.05f);
            mosquitoes[i].alive = true;
            mosquitoes[i].deadTimer = 0;
            mosquitoes[i].attractedToPond = (rand() % 2 == 0);
            mosquitoes[i].pondTime = 0;
            totalAlive++;
            return;
        }
    }
}
void updateMosquitoesLogic() {
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
        if (mosquitoes[i].alive) {
            if (mosquitoes[i].attractedToPond) {
                float targetX = pondX, targetY = pondY;
                if (waterBowlVisible && rand() % 2 == 0) {
                    targetX = waterBowlX;
                    targetY = waterBowlY;
                }
                float dx = targetX - mosquitoes[i].x;
                float dy = targetY - mosquitoes[i].y;
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist > 0.01f) {
                    mosquitoes[i].dx += (dx / dist) * 0.001f;
                    mosquitoes[i].dy += (dy / dist) * 0.001f;
                }
            }
            mosquitoes[i].x += mosquitoes[i].dx;
            mosquitoes[i].y += mosquitoes[i].dy;
            if (mosquitoes[i].x < -0.98f || mosquitoes[i].x > 0.98f) mosquitoes[i].dx = -mosquitoes[i].dx;
            if (mosquitoes[i].y < -0.98f || mosquitoes[i].y > 0.98f) mosquitoes[i].dy = -mosquitoes[i].dy;
            if ((rand() % 1000) < 8) {
                mosquitoes[i].dx += randFloat(-0.002f, 0.002f);
                mosquitoes[i].dy += randFloat(-0.002f, 0.002f);
            }
            // Larva spawning logic
            if (isNearPondArea(mosquitoes[i].x, mosquitoes[i].y)) {
                mosquitoes[i].pondTime++;
                if (mosquitoes[i].pondTime > 200) { // ~10s
                    Larva larva = {mosquitoes[i].x + randFloat(-0.02f, 0.02f), mosquitoes[i].y + randFloat(-0.02f, 0.02f), 0};
                    larvae.push_back(larva);
                    mosquitoes[i].pondTime = 0;
                }
            } else {
                mosquitoes[i].pondTime = 0;
            }
        } else if (mosquitoes[i].deadTimer > 0) {
            mosquitoes[i].deadTimer--;
        }
    }
    // Larva update
    for (size_t i = 0; i < larvae.size(); ++i) {
        larvae[i].timer++;
        if (larvae[i].timer > 400) { // ~20s, then larva matures
            spawnOneMosquito(true);
            larvae.erase(larvae.begin() + i);
            --i;
        }
    }
    // Rain event
    if (rainActive) {
        rainTimer--;
        if (rainTimer <= 0) {
            rainActive = false;
            snprintf(popupText, sizeof(popupText), "Rain stopped. Watch for breeding sites!");
            popupTimer = popupDuration;
        }
    } else if (rand() % 1000 < 2 && !rainActive) { // Random rain trigger
        rainActive = true;
        rainTimer = rainDuration;
        for (int i = 0; i < rainSpawnCount; ++i) spawnOneMosquito(true);
        snprintf(popupText, sizeof(popupText), "Rain event! Mosquitoes spawning!");
        popupTimer = popupDuration;
#ifdef _WIN32
        Beep(500, 300);
#endif
    }
    // Spawn logic
    bool boost = waterBowlVisible || totalAlive > 5 || rainActive;
    int nearPondCount = 0, nearBowlCount = 0;
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
        if (mosquitoes[i].alive) {
            if (isNearPondArea(mosquitoes[i].x, mosquitoes[i].y)) nearPondCount++;
            if (isNearWaterBowl(mosquitoes[i].x, mosquitoes[i].y)) nearBowlCount++;
        }
    }
    if (nearPondCount > 2 || nearBowlCount > 1) boost = true;
    currentSpawnInterval = boost ? spawnIntervalHigh : spawnIntervalNormal;
    spawnCounter++;
    if (spawnCounter >= currentSpawnInterval) {
        spawnOneMosquito(boost);
        if (rand() % 100 < 60) spawnOneMosquito(boost);
        spawnCounter = 0;
    }
}
// ---------------- Spray & collision ----------------
void checkSprayCollisions() {
    if (!spraying) return;
    int killedThisSpray = 0;
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
        if (!mosquitoes[i].alive) continue;
        float dx = mosquitoes[i].x - sprayX;
        float dy = mosquitoes[i].y - sprayY;
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist <= sprayRadius + mosquitoes[i].size * 0.5f) {
            mosquitoes[i].alive = false;
            mosquitoes[i].deadTimer = 150 + (rand() % 150);
            totalAlive--;
            totalKilled++;
            killedThisSpray++;
            updateHistogram(1);
            mosquitoes[i].x += randFloat(-0.04f, 0.04f);
            mosquitoes[i].y += randFloat(-0.04f, 0.04f);
        }
    }
    // Spray larvae too
    for (size_t i = 0; i < larvae.size(); ++i) {
        float dx = larvae[i].x - sprayX;
        float dy = larvae[i].y - sprayY;
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist <= sprayRadius) {
            larvae.erase(larvae.begin() + i);
            --i;
            totalKilled++;
            killedThisSpray++;
            updateHistogram(1);
        }
    }
    if (killedThisSpray > 0) {
#ifdef _WIN32
        Beep(800, 100);
#endif
        snprintf(popupText, sizeof(popupText), "Killed %d mosquitoes/larvae!", killedThisSpray);
        popupTimer = popupDuration;
    }
}

void updateHistogram(int killedThisFrame) {
    frameCounter++;
    killsPerMinute.back() += killedThisFrame;
    if (frameCounter >= framesPerMinute) {
        killsPerMinute.push_back(0);
        frameCounter = 0;
        if (killsPerMinute.size() > 5) killsPerMinute.erase(killsPerMinute.begin());
    }
}
