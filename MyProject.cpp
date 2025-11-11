#include <GL/glut.h>
void displayText(const char* text, float x, float y, void* font);
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <vector>
#ifdef _WIN32
#include <windows.h> // For Beep sound
#endif

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
// Menu IDs
enum MenuOptions { MENU_RESTART, MENU_TOGGLE_BOWL, MENU_EXIT, MENU_TRIGGER_RAIN };
// Utility random
float randFloat(float a, float b) {
    return a + static_cast<float>(rand()) / RAND_MAX * (b - a);
}
// ---------------- Mosquito struct ----------------
struct Mosquito {
    float x, y;
    float dx, dy;
    float size;
    bool alive;
    int deadTimer;
    bool attractedToPond;
    int pondTime; // For larva spawning
};
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
// ---------------- Drawing helpers ----------------
void drawCircle(float cx, float cy, float rx, float ry, int segments = 48) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < segments; ++i) {
        float a = 2.0f * 3.1415926f * i / segments;
        glVertex2f(cx + cosf(a) * rx, cy + sinf(a) * ry);
    }
    glEnd();
}
void drawMosquito(float x, float y, float size) {
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex2f(x - size / 2.0f, y);
    glVertex2f(x + size / 2.0f, y);
    glEnd();
    glColor3f(0.12f, 0.12f, 0.12f);
    drawCircle(x + size / 2.0f, y, size * 0.18f, size * 0.18f, 16);
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x - size * 0.1f, y + size * 0.15f);
    glVertex2f(x - size * 0.45f, y + size * 0.45f);
    glVertex2f(x + size * 0.15f, y + size * 0.2f);
    glEnd();
}
void drawLarva(float x, float y) {
    glColor3f(0.2f, 0.2f, 0.2f);
    drawCircle(x, y, 0.01f, 0.01f, 16);
}
void drawHouse(float x, float y, float w, float h) {
    glColor3f(0.55f, 0.27f, 0.07f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    glColor3f(0.08f, 0.08f, 0.45f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x, y + h);
    glVertex2f(x + w / 2.0f, y + h + h / 2.0f);
    glVertex2f(x + w, y + h);
    glEnd();
}
void drawTree(float x, float y) {
    glColor3f(0.54f, 0.27f, 0.07f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + 0.05f, y);
    glVertex2f(x + 0.05f, y + 0.3f);
    glVertex2f(x, y + 0.3f);
    glEnd();
    glColor3f(0.0f, 0.5f, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x - 0.1f, y + 0.3f);
    glVertex2f(x + 0.15f, y + 0.5f);
    glVertex2f(x + 0.3f, y + 0.3f);
    glEnd();
    drawCircle(x + 0.05f, y + 0.4f, 0.1f, 0.1f, 16);
    drawCircle(x + 0.15f, y + 0.35f, 0.08f, 0.08f, 16);
}
void drawGrass(float x, float y) {
    glColor3f(0.1f, 0.6f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x, y);
    glVertex2f(x + 0.02f, y + 0.1f);
    glVertex2f(x + 0.04f, y);
    glEnd();
}
void drawPond() {
    glColor3f(0.05f, 0.35f, 0.9f);
    drawCircle(pondX, pondY, pondRadiusX, pondRadiusY, 72);
    glColor3f(0.1f, 0.4f, 0.95f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 36; ++i) {
        float a = 2.0f * 3.1415926f * i / 36;
        glVertex2f(pondX + cosf(a) * pondRadiusX * 0.8f, pondY + sinf(a) * pondRadiusY * 0.8f);
    }
    glEnd();
}
void drawWaterBowl() {
    if (!waterBowlVisible) return;
    glColor3f(0.45f, 0.22f, 0.07f);
    drawCircle(waterBowlX, waterBowlY, waterBowlRadius, waterBowlRadius, 32);
    glColor3f(0.4f, 0.75f, 0.95f);
    drawCircle(waterBowlX, waterBowlY, waterBowlRadius * 0.8f, waterBowlRadius * 0.8f, 32);
}
void drawRain() {
    if (!rainActive) return;
    glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
    for (int i = 0; i < 50; ++i) {
        float x = randFloat(-1.0f, 1.0f);
        float y = randFloat(-1.0f, 1.0f);
        glBegin(GL_LINES);
        glVertex2f(x, y);
        glVertex2f(x, y - 0.05f);
        glEnd();
    }
}
// ---------------- Histogram ----------------
// ---------- Replace existing drawHistogram() with this ----------
void drawHistogram() {
    if (killsPerMinute.empty()) return;

    // background panel
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, 0.85f);
    glBegin(GL_QUADS);
      glVertex2f(0.48f, -0.98f);
      glVertex2f(0.98f, -0.98f);
      glVertex2f(0.98f, -0.48f);
      glVertex2f(0.48f, -0.48f);
    glEnd();

    // calculate layout
    size_t bars = killsPerMinute.size();
    const float marginLeft = 0.52f;
    const float marginBottom = -0.95f;
    const float areaWidth = 0.98f - marginLeft; // available width
    const float maxBarWidth = 0.12f;            // cap width per bar
    float barWidth = fminf(maxBarWidth, areaWidth / (float)bars);
    float gap = barWidth * 0.15f;
    float usableBar = barWidth - gap;

    // find max for scaling (avoid div by zero)
    int maxKills = 1;
    for (int k : killsPerMinute) if (k > maxKills) maxKills = k;
    const float maxHeight = 0.35f;

    // draw bars
    for (size_t i = 0; i < bars; ++i) {
        float normalized = (killsPerMinute[i] / (float)maxKills);
        float height = normalized * maxHeight;
        float x0 = marginLeft + i * barWidth + gap * 0.5f;
        float x1 = x0 + usableBar;
        float y0 = marginBottom;
        float y1 = marginBottom + height;

        // bar fill
        glColor3f(0.16f, 0.6f, 0.16f);
        glBegin(GL_QUADS);
          glVertex2f(x0, y0);
          glVertex2f(x1, y0);
          glVertex2f(x1, y1);
          glVertex2f(x0, y1);
        glEnd();

        // small label (value) above each bar
        char valbuf[16];
        snprintf(valbuf, sizeof(valbuf), "%d", killsPerMinute[i]);
        displayText(valbuf, x0, y1 + 0.01f, GLUT_BITMAP_HELVETICA_12);
    }

    // axis / title
    glColor3f(0.0f, 0.0f, 0.0f);
    displayText("Kills per Minute", 0.50f, -0.50f, GLUT_BITMAP_HELVETICA_12);

    glDisable(GL_BLEND);
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
// ---------------- Display ----------------
void displayText(const char* text, float x, float y, void* font = GLUT_BITMAP_HELVETICA_18) {
    glColor3f(0.0f, 0.0f, 0.0f);
    glRasterPos2f(x, y);
    for (size_t i = 0; i < strlen(text); ++i) glutBitmapCharacter(font, text[i]);
}
void displayUI() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(-0.98f, 0.85f);
    glVertex2f(-0.5f, 0.85f);
    glVertex2f(-0.5f, 0.98f);
    glVertex2f(-0.98f, 0.98f);
    glEnd();
    glDisable(GL_BLEND);
    char buf[128];
    snprintf(buf, sizeof(buf), "Alive: %d", totalAlive);
    displayText(buf, -0.95f, 0.94f);
    snprintf(buf, sizeof(buf), "Killed: %d", totalKilled);
    displayText(buf, -0.95f, 0.89f);
    snprintf(buf, sizeof(buf), "Spawn Rate: %s", (currentSpawnInterval == spawnIntervalHigh ? "High" : "Normal"));
    displayText(buf, -0.7f, 0.94f);
    snprintf(buf, sizeof(buf), "Spray Charges: %d/%d", sprayCharges, maxSprayCharges);
    displayText(buf, -0.7f, 0.89f);
}
void displayInstructions() {
    const char* lines[] = {
        "Controls:",
        "Left-Click: Spray (if charges > 0)",
        "S: Random Spray",
        "R: Toggle Water Bowl",
        "T: Trigger Rain Event",
        "Drag Bowl: Right-Click & Move",
        "Right-Click: Menu",
        "ESC: Exit"
    };
    float y = 0.75f;
    for (int i = 0; i < (int)(sizeof(lines)/sizeof(lines[0])); ++i) {
        displayText(lines[i], 0.6f, y, GLUT_BITMAP_HELVETICA_12);
        y -= 0.04f;
    }
}
void displayPopup() {
    if (popupTimer > 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.9f, 0.9f, 0.1f, 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(-0.4f, 0.0f);
        glVertex2f(0.4f, 0.0f);
        glVertex2f(0.4f, 0.15f);
        glVertex2f(-0.4f, 0.15f);
        glEnd();
        glDisable(GL_BLEND);
        displayText(popupText, -0.35f, 0.05f, GLUT_BITMAP_TIMES_ROMAN_24);
    }
}
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glBegin(GL_QUADS);
    glColor3f(0.53f, 0.81f, 0.92f);
    glVertex2f(-1,1);
    glVertex2f(1,1);
    glColor3f(0.8f, 0.9f, 0.95f);
    glVertex2f(1,-1);
    glVertex2f(-1,-1);
    glEnd();
    glColor3f(1.0f, 0.9f, 0.0f);
    drawCircle(0.8f, 0.8f, 0.1f, 0.1f, 32);
    glColor3f(1,1,1);
    drawCircle(-0.8f, 0.75f, 0.08f, 0.04f, 24);
    drawCircle(-0.55f, 0.8f, 0.07f, 0.035f, 24);
    drawCircle(0.3f, 0.7f, 0.09f, 0.045f, 24);
    for (int i = 0; i < 20; ++i) drawGrass(randFloat(-1.0f, 1.0f), -0.95f);
    drawHouse(-0.9f, -0.8f, 0.3f, 0.3f);
    drawHouse(-0.5f, -0.8f, 0.4f, 0.4f);
    drawHouse(0.6f, -0.85f, 0.25f, 0.25f);
    drawTree(-0.2f, -0.6f);
    drawTree(0.7f, -0.7f);
    drawTree(0.2f, -0.75f);
    drawPond();
    drawWaterBowl();
    for (size_t i = 0; i < larvae.size(); ++i) drawLarva(larvae[i].x, larvae[i].y);
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
        if (mosquitoes[i].alive) drawMosquito(mosquitoes[i].x, mosquitoes[i].y, mosquitoes[i].size);
    }
    if (spraying) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.08f, 0.5f, 1.0f, 0.45f);
        drawCircle(sprayX, sprayY, sprayRadius, sprayRadius, 36);
        glDisable(GL_BLEND);
    }
    drawRain();
    displayText("Dengue Awareness Simulation", -0.95f, 0.98f, GLUT_BITMAP_TIMES_ROMAN_24);
    displayUI();
    displayInstructions();
    displayPopup();
    drawHistogram();
    glutSwapBuffers();
}
// ---------------- Input & Timer ----------------
void timerFunc(int value) {
    updateMosquitoesLogic();
    if (spraying) {
        checkSprayCollisions();
        sprayRadius += sprayGrowth;
        if (sprayRadius > maxSprayRadius) {
            spraying = false;
            sprayRadius = 0.02f;
        }
#ifdef _WIN32
        Beep(600, 50);
#endif
    }
    sprayRefillTimer++;
    if (sprayRefillTimer >= sprayRefillInterval && sprayCharges < maxSprayCharges) {
        sprayCharges++;
        sprayRefillTimer = 0;
        snprintf(popupText, sizeof(popupText), "Spray charge refilled! %d/%d", sprayCharges, maxSprayCharges);
        popupTimer = popupDuration;
#ifdef _WIN32
        Beep(1200, 200);
#endif
    }
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
        if (!mosquitoes[i].alive && mosquitoes[i].deadTimer <= 0) {
            if (totalAlive < 5) spawnOneMosquito(false);
        } else if (!mosquitoes[i].alive) {
            int dec = (waterBowlVisible || isNearPondArea(mosquitoes[i].x, mosquitoes[i].y)) ? 2 : 1;
            mosquitoes[i].deadTimer -= dec;
            if (mosquitoes[i].deadTimer <= 0) spawnOneMosquito(false);
        }
    }
    if (popupTimer > 0) popupTimer--;
    glutPostRedisplay();
    glutTimerFunc(50, timerFunc, 0);
}
void keyboard(unsigned char key, int x, int y) {
    if (key == 's' || key == 'S') {
        if (sprayCharges > 0) {
            sprayX = randFloat(-0.9f, 0.9f);
            sprayY = randFloat(-0.9f, 0.9f);
            sprayRadius = 0.02f;
            spraying = true;
            sprayCharges--;
            snprintf(popupText, sizeof(popupText), "Random spray! Charges left: %d", sprayCharges);
            popupTimer = popupDuration;
        } else {
            snprintf(popupText, sizeof(popupText), "No spray charges! Wait for refill.");
            popupTimer = popupDuration;
#ifdef _WIN32
            Beep(400, 200);
#endif
        }
    } else if (key == 'r' || key == 'R') {
        waterBowlVisible = !waterBowlVisible;
        snprintf(popupText, sizeof(popupText), waterBowlVisible ? "Water bowl added: Increases breeding!" : "Water bowl removed: Reduces spawning.");
        popupTimer = popupDuration;
#ifdef _WIN32
        Beep(1000, 200);
#endif
    } else if (key == 't' || key == 'T') {
        if (!rainActive) {
            rainActive = true;
            rainTimer = rainDuration;
            for (int i = 0; i < rainSpawnCount; ++i) spawnOneMosquito(true);
            snprintf(popupText, sizeof(popupText), "Manual rain event triggered!");
            popupTimer = popupDuration;
#ifdef _WIN32
            Beep(500, 300);
#endif
        }
    } else if (key == 27) exit(0);
}
void mouse(int button, int state, int mx, int my) {
    int winW = glutGet(GLUT_WINDOW_WIDTH);
    int winH = glutGet(GLUT_WINDOW_HEIGHT);
    float nx = (2.0f * mx / winW) - 1.0f;
    float ny = 1.0f - (2.0f * my / winH);
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (sprayCharges > 0) {
            sprayX = nx;
            sprayY = ny;
            sprayRadius = 0.02f;
            spraying = true;
            sprayCharges--;
            snprintf(popupText, sizeof(popupText), "Spray at mouse! Charges left: %d", sprayCharges);
            popupTimer = popupDuration;
        } else {
            snprintf(popupText, sizeof(popupText), "No spray charges! Wait for refill.");
            popupTimer = popupDuration;
#ifdef _WIN32
            Beep(400, 200);
#endif
        }
    } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        if (waterBowlVisible) {
            float dx = nx - waterBowlX;
            float dy = ny - waterBowlY;
            if (sqrtf(dx*dx + dy*dy) < waterBowlRadius * 1.5f) {
                draggingBowl = true;
            }
        }
    } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {
        draggingBowl = false;
    }
}
void motion(int mx, int my) {
    if (draggingBowl) {
        int winW = glutGet(GLUT_WINDOW_WIDTH);
        int winH = glutGet(GLUT_WINDOW_HEIGHT);
        waterBowlX = (2.0f * mx / winW) - 1.0f;
        waterBowlY = 1.0f - (2.0f * my / winH);
        glutPostRedisplay();
    }
}
void menuFunc(int option) {
    switch (option) {
        case MENU_RESTART:
            initializeMosquitoes();
            snprintf(popupText, sizeof(popupText), "Simulation Restarted!");
            popupTimer = popupDuration;
            break;
        case MENU_TOGGLE_BOWL:
            waterBowlVisible = !waterBowlVisible;
            snprintf(popupText, sizeof(popupText), waterBowlVisible ? "Water Bowl Toggled On" : "Water Bowl Toggled Off");
            popupTimer = popupDuration;
            break;
        case MENU_TRIGGER_RAIN:
            if (!rainActive) {
                rainActive = true;
                rainTimer = rainDuration;
                for (int i = 0; i < rainSpawnCount; ++i) spawnOneMosquito(true);
                snprintf(popupText, sizeof(popupText), "Rain event triggered!");
                popupTimer = popupDuration;
#ifdef _WIN32
                Beep(500, 300);
#endif
            }
            break;
        case MENU_EXIT:
            exit(0);
            break;
    }
}
// ---------------- Setup ----------------
void initGL() {
    glClearColor(1,1,1,1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1,1,-1,1);
    initializeMosquitoes();
}
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_W, WINDOW_H);
    glutCreateWindow("Dengue Awareness Simulation");
    initGL();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutTimerFunc(50, timerFunc, 0);
    glutCreateMenu(menuFunc);
    glutAddMenuEntry("Restart", MENU_RESTART);
    glutAddMenuEntry("Toggle Water Bowl", MENU_TOGGLE_BOWL);
    glutAddMenuEntry("Trigger Rain", MENU_TRIGGER_RAIN);
    glutAddMenuEntry("Exit", MENU_EXIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    glutMainLoop();
    return 0;
}