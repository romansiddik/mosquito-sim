#include <GLFW/glfw3.h>
#include <cstdio>
#include <cmath>
#include <vector>
#include "config.h"
#include "utils.h"
#include "drawing.h"
#include "mosquito.h"
#include "ui.h"

// ---------------- Global variable definitions ----------------
const int NUM_MOSQUITOES = 30;
const int WINDOW_W = 900;
const int WINDOW_H = 650;
const float pondX = -0.7f;
const float pondY = -0.85f;
const float pondRadiusX = 0.25f;
const float pondRadiusY = 0.12f;
float waterBowlX = -0.4f, waterBowlY = -0.9f, waterBowlRadius = 0.05f;
bool waterBowlVisible = false;
bool draggingBowl = false;
bool spraying = false;
float sprayX = 0.0f, sprayY = 0.0f, sprayRadius = 0.02f;
const float maxSprayRadius = 0.15f;
const float sprayGrowth = 0.018f;
int sprayCharges = 5;
const int maxSprayCharges = 5;
int sprayRefillTimer = 0;
const int sprayRefillInterval = 600;
int spawnCounter = 0;
int spawnIntervalNormal = 180;
int spawnIntervalHigh = 50;
int currentSpawnInterval = spawnIntervalNormal;
bool rainActive = false;
int rainTimer = 0;
const int rainDuration = 200;
const int rainSpawnCount = 5;
int totalAlive = 0;
int totalKilled = 0;
std::vector<Larva> larvae;
char popupText[256] = "";
int popupTimer = 0;
const int popupDuration = 80;
std::vector<int> killsPerMinute;
int frameCounter = 0;
const int framesPerMinute = 1200;

// ---------------- Input & Timer ----------------

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        if (sprayCharges > 0) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            sprayX = (float)(x / width * 2.0 - 1.0);
            sprayY = (float)(1.0 - y / height * 2.0);
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
    } else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        waterBowlVisible = !waterBowlVisible;
        snprintf(popupText, sizeof(popupText), waterBowlVisible ? "Water bowl added: Increases breeding!" : "Water bowl removed: Reduces spawning.");
        popupTimer = popupDuration;
#ifdef _WIN32
        Beep(1000, 200);
#endif
    } else if (key == GLFW_KEY_T && action == GLFW_PRESS) {
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
    } else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (sprayCharges > 0) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            sprayX = (float)(x / width * 2.0 - 1.0);
            sprayY = (float)(1.0 - y / height * 2.0);
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
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        if (waterBowlVisible) {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            float nx = (float)(x / width * 2.0 - 1.0);
            float ny = (float)(1.0 - y / height * 2.0);
            float dx = nx - waterBowlX;
            float dy = ny - waterBowlY;
            if (sqrtf(dx*dx + dy*dy) < waterBowlRadius * 1.5f) {
                draggingBowl = true;
            }
        }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
        draggingBowl = false;
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (draggingBowl) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        waterBowlX = (float)(xpos / width * 2.0 - 1.0);
        waterBowlY = (float)(1.0 - ypos / height * 2.0);
    }
}

// ---------------- Setup ----------------
void initGL() {
    glClearColor(1,1,1,1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1,1,-1,1,-1,1);
    initializeMosquitoes();
}

int main(void) {
    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(WINDOW_W, WINDOW_H, "Dengue Awareness Simulation", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    initGL();

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        if (currentTime - lastTime >= 0.05) { // 50ms interval
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
            lastTime = currentTime;
        }

        display();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
