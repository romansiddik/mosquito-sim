#include "drawing.h"
#include "config.h"
#include "utils.h"
#include <GLFW/glfw3.h>
#include <cmath>

// ---------------- Drawing helpers ----------------
void drawCircle(float cx, float cy, float rx, float ry, int segments) {
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
    }

    glDisable(GL_BLEND);
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
    drawHistogram();
}