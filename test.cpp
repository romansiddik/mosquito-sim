#include <GL/glut.h>
#include <GL/glu.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstring>
#include <random>
#include <cmath>  // For sin/cos in ripples
#include <cstdlib>  // For rand() and RAND_MAX
#include <cstdio>  // For sprintf (add if not present)
#include <ctime>   // For time()

#ifdef min
#undef min  // Or other code
#endif
#ifdef _WIN32
#include <windows.h> // For Beep
#endif

// --- Constants ---
#define NUM_MOSQUITOES 50
#define MAX_LARVAE 100
#define WINDOW_W 1024
#define WINDOW_H 768
#define POPUP_DURATION 150
#define MAX_SPRAY_CHARGES 5
#define RAIN_DURATION 500
#define RAIN_SPAWN_COUNT 3
#define WIND_DURATION 600
#define FOG_DURATION 400
#define CLEANUP_DURATION 1000
#define SPAWN_INTERVAL_NORMAL 100
#define SPAWN_INTERVAL_HIGH 60
#define NUM_RAINDROPS 50

// World positions
#define POND_X 0.0f
#define POND_Y -0.2f
#define POND_RX 0.4f
#define POND_RY 0.2f

// Menu options
#define MENU_RESTART 1
#define MENU_TOGGLE_BOWL 2
#define MENU_TRIGGER_RAIN 3
#define MENU_EXIT 4

// --- Structures ---
struct Mosquito {
    float x, y, z;
    float dx, dy;
    float size;
    bool alive;
    int deadTimer;
    bool attractedToPond;
    int pondTime;
};

struct Larva {
    float x;      // Position x
    float y;      // Position y
    float size;   // Visual/collision size
    int timer;    // Frame counter for logic (e.g., growth)
    bool alive;   // Flag for active state

    // Optional constructor for safe initialization
    Larva(float px = 0.0f, float py = 0.0f, float ps = 0.01f, int pt = 0, bool pa = true)
        : x(px), y(py), size(ps), timer(pt), alive(pa) {}
};

struct Raindrop {
    float x, y, z;
    float speed;
};

// --- Global Variables ---
Mosquito mosquitoes[NUM_MOSQUITOES];
std::vector<Larva> larvae;
Raindrop rain[NUM_RAINDROPS];
          // For cylinders/cones
float g_treeSwayAngle = 5.0f;

int totalAlive = 0;
int totalKilled = 0;
int spawnCounter = 0;
int currentSpawnInterval = SPAWN_INTERVAL_NORMAL;
int difficultyTimer = 0;


// --- Global State & Constants ---

// Spray variables
bool spraying = false;
float sprayX, sprayY, sprayRadius;
int sprayTimer = 0;
int sprayCharges = MAX_SPRAY_CHARGES;

// Environmental variables
bool dayTime = true;
bool waterBowlVisible = true;
float waterBowlX = 0.5f, waterBowlY = 0.0f, waterBowlRadius = 0.1f;
bool draggingBowl = false;
int rainTimer = 0;
bool rainActive = false;
int windTimer = 0;
bool windActive = false;
float windForce = 0.0f;
float treeSwayAngle = 0.0f;
int fogTimer = 0;
bool fogActive = false;
int cleanupTimer = 0;
float cloudOffset = 0.0f; 

// Popup
char popupText[128] = "Welcome! Protect your yard from dengue!";
int popupTimer = POPUP_DURATION;

// Mouse tracking
int lastMouseX = 0;
int lastMouseY = 0;
int windowWidth = WINDOW_W;
int windowHeight = WINDOW_H;

// Histogram data
#define HISTOGRAM_SIZE 20
int killedHistory[HISTOGRAM_SIZE] = {0};
int historyIndex = 0;

// Random number generator
std::mt19937 rng(std::random_device{}());

// --- Function Declarations ---
void spawnOneMosquito(bool pondBoost);
void checkSprayCollisions(int& killedThisFrame);
void doSpray(float x, float y);
void updateHistogram(int killedCount);
void initializeMosquitoes();
void initializeRain();
float randFloat(float min, float max);
void checkGLError(const char* func);
void displayText(float x, float y, const char* text, void* font);
void drawCircle(float cx, float cy, float cz, float r, int segments);

void drawLarva(float x, float y, float size);
void drawCloud(float x, float y);
void drawRain();
void drawPond();
void drawWaterBowl();
void drawWindEffect();
void drawFog();
void drawHistogram();
float screenToWorldX(int x, int w);
float screenToWorldY(int y, int h);
void updateMosquitoesLogic();
void displayUI();
void displayInstructions();
void displayPopup();
void display();
void menuFunc(int option);
void reshape(int w, int h);
void initGL();
void keyboard(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void timerFunc(int value);

// --- General Helpers ---
float randFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

void checkGLError(const char* func) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL Error in %s: %s\n", func, gluErrorString(err));
        
    }
}

void initializeMosquitoes() {
    totalAlive = 0;
    totalKilled = 0;
    spawnCounter = 0;
    currentSpawnInterval = SPAWN_INTERVAL_NORMAL;
    difficultyTimer = 0;
    spraying = false;
    sprayCharges = MAX_SPRAY_CHARGES;
    waterBowlVisible = true;
    waterBowlX = 0.5f;
    waterBowlY = 0.0f;
    rainActive = false;
    windActive = false;
    fogActive = false;
    cleanupTimer = 0;
    larvae.clear();
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
        mosquitoes[i].alive = false;
        mosquitoes[i].deadTimer = 0;
    }
    for (int i = 0; i < HISTOGRAM_SIZE; ++i) killedHistory[i] = 0;
    historyIndex = 0;
    for (int i = 0; i < 5; ++i) spawnOneMosquito(true);
}

void initializeRain() {
    for (int i = 0; i < NUM_RAINDROPS; ++i) {
        rain[i].x = randFloat(-1.0f, 1.0f);
        rain[i].y = randFloat(-1.0f, 1.0f);
        rain[i].z = randFloat(-1.0f, 1.0f);
        rain[i].speed = randFloat(0.01f, 0.03f);
    }
}

void updateHistogram(int killedCount) {
    killedHistory[historyIndex] = killedCount;
    historyIndex = (historyIndex + 1) % HISTOGRAM_SIZE;
}

float screenToWorldX(int x, int w) {
    return (float)x / w * 2.0f - 1.0f;
}

float screenToWorldY(int y, int h) {
    return 1.0f - (float)y / h * 2.0f;
}

// --- Drawing Functions ---
void displayText(float x, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_10) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(font, *text++);
    }
}

void drawCircle(float cx, float cy, float cz, float r, int segments) {
    glPushMatrix();
    glTranslatef(cx, cy, cz);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * 3.1415926f / segments;
        glVertex3f(cosf(angle) * r, sinf(angle) * r, 0.0f);
    }
    glEnd();
    glPopMatrix();
}

void drawMosquito(float x, float y, float z, float size, float r, float g, float b, bool bloodFed = false) {
    GLUquadricObj* quadric = gluNewQuadric();
    gluQuadricDrawStyle(quadric, GLU_FILL);  // Use GLU_LINE for wireframe if desired

    glPushMatrix();
    glTranslatef(x, y, z);

    // Thorax (central body part, ellipsoid shape)
    glPushMatrix();
    glColor3f(r * 0.8f, g * 0.8f, b * 0.8f);  // Slightly darker for thorax
    glScalef(size * 0.15f, size * 0.1f, size * 0.1f);  // Elongate slightly
    glutSolidSphere(1.0f, 12, 12);
    glPopMatrix();

    // Head (small sphere attached to thorax)
    glPushMatrix();
    glTranslatef(size * 0.15f, 0.0f, 0.0f);  // Position in front of thorax
    glColor3f(r * 0.5f, g * 0.5f, b * 0.5f);  // Dark head
    glutSolidSphere(size * 0.08f, 10, 10);
    glPopMatrix();

    // Proboscis (thin cylinder extending from head)
    glPushMatrix();
    glTranslatef(size * 0.23f, 0.0f, 0.0f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);  // Align along x-axis
    glColor3f(r * 0.3f, g * 0.3f, b * 0.3f);  // Dark proboscis
    gluCylinder(quadric, size * 0.01f, size * 0.005f, size * 0.1f, 8, 1);  // Tapered
    glPopMatrix();

    // Antennae (two thin cylinders from head)
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(size * 0.15f, size * 0.05f * i, size * 0.05f);
        glRotatef(45.0f, 0.0f, 0.0f, 1.0f);  // Angle outward
        glColor3f(r * 0.4f, g * 0.4f, b * 0.4f);
        gluCylinder(quadric, size * 0.005f, size * 0.002f, size * 0.12f, 6, 1);
        glPopMatrix();
    }

    // Abdomen (elongated, cylinder or scaled sphere; red if blood-fed)
    glPushMatrix();
    glTranslatef(-size * 0.15f, 0.0f, 0.0f);  // Behind thorax
    if (bloodFed) {
        glColor3f(1.0f, 0.0f, 0.0f);  // Red for blood-fed
    } else {
        glColor3f(r, g, b);
    }
    glScalef(size * 0.2f, size * 0.08f, size * 0.08f);  // Elongated
    glutSolidSphere(1.0f, 12, 12);
    glPopMatrix();

    // Wings (two semi-transparent triangles)
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(0.0f, size * 0.05f * i, size * 0.05f);
        glRotatef(30.0f * i, 1.0f, 0.0f, 0.0f);  // Fan out
        glColor4f(1.0f, 1.0f, 1.0f, 0.6f);  // Semi-transparent white
        glBegin(GL_TRIANGLES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(size * 0.3f, size * 0.1f, 0.0f);
        glVertex3f(0.0f, size * 0.2f, 0.0f);
        glEnd();
        glPopMatrix();
    }

    // Legs (six simple cylinders)
    float legAngles[3] = {-45.0f, 0.0f, 45.0f};  // Front, mid, hind positions
    for (int leg = 0; leg < 3; ++leg) {
        for (int side = -1; side <= 1; side += 2) {
            glPushMatrix();
            glTranslatef(size * (0.05f - leg * 0.1f), size * 0.05f * side, 0.0f);
            glRotatef(legAngles[leg], 0.0f, 0.0f, 1.0f);  // Angle legs
            glColor3f(r * 0.6f, g * 0.6f, b * 0.6f);
            gluCylinder(quadric, size * 0.01f, size * 0.005f, size * 0.3f, 6, 1);
            glPopMatrix();
        }
    }

    gluDeleteQuadric(quadric);
    glPopMatrix();
}

void drawLarva(float x, float y, float size) {
    GLUquadricObj* quadric = gluNewQuadric();
    gluQuadricDrawStyle(quadric, GLU_FILL);

    glPushMatrix();
    glTranslatef(x, y, 0.0f);  // Center at (x, y, 0); adjust z as needed
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);  // Orient vertically, head "down"

    // Head (small sphere, darker)
    glPushMatrix();
    glTranslatef(0.0f, size * 0.1f, 0.0f);
    glColor4f(0.4f, 0.3f, 0.2f, 0.8f);  // Semi-transparent brown
    glutSolidSphere(size * 0.05f, 10, 10);
    glPopMatrix();

    // Mouth brushes (simplified as lines fanning out from head)
    glColor4f(0.3f, 0.3f, 0.3f, 0.6f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < 6; ++i) {
        float angle = i * 60.0f * 3.14159f / 180.0f;
        glVertex3f(0.0f, size * 0.1f, 0.0f);
        glVertex3f(size * 0.03f * sin(angle), size * 0.1f + size * 0.05f, size * 0.03f * cos(angle));
    }
    glEnd();

    // Antennae (two thin cylinders from head)
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(size * 0.03f * side, size * 0.1f, 0.0f);
        glRotatef(45.0f * side, 0.0f, 0.0f, 1.0f);
        glColor4f(0.4f, 0.3f, 0.2f, 0.8f);
        gluCylinder(quadric, size * 0.005f, size * 0.002f, size * 0.08f, 6, 1);
        glPopMatrix();
    }

    // Thorax (ellipsoid, wider)
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f);
    glScalef(size * 0.08f, size * 0.15f, size * 0.08f);  // Elongated along y
    glColor4f(0.5f, 0.4f, 0.3f, 0.7f);
    glutSolidSphere(1.0f, 12, 12);
    glPopMatrix();

    // Abdomen (8 tapered cylinder segments)
    float segmentLength = size * 0.05f;
    float currentY = -size * 0.1f;  // Start after thorax
    for (int seg = 0; seg < 8; ++seg) {
        glPushMatrix();
        glTranslatef(0.0f, currentY, 0.0f);
        glColor4f(0.6f + seg * 0.05f, 0.5f + seg * 0.05f, 0.4f, 0.7f);  // Lighten slightly toward tail
        gluCylinder(quadric, size * (0.04f - seg * 0.002f), size * (0.035f - seg * 0.002f), segmentLength, 8, 1);
        currentY -= segmentLength;
        glPopMatrix();
    }

    // Siphon (thin cylinder at rear)
    glPushMatrix();
    glTranslatef(0.0f, currentY - size * 0.05f, 0.0f);
    glColor4f(0.3f, 0.2f, 0.1f, 0.8f);  // Darker tube
    gluCylinder(quadric, size * 0.01f, size * 0.005f, size * 0.1f, 6, 1);
    glPopMatrix();

    // Optional lateral hairs (simplified lines along abdomen)
    glColor4f(0.3f, 0.3f, 0.3f, 0.5f);
    glBegin(GL_LINES);
    for (int seg = 1; seg < 8; seg += 2) {
        float hairY = -size * 0.1f - seg * segmentLength;
        for (int side = -1; side <= 1; side += 2) {
            glVertex3f(size * 0.03f * side, hairY, 0.0f);
            glVertex3f(size * 0.05f * side, hairY - size * 0.02f, 0.0f);
        }
    }
    glEnd();

    gluDeleteQuadric(quadric);
    glPopMatrix();
}

void drawWaterBowl() {
    if (!waterBowlVisible) return;
    glColor3f(0.45f, 0.22f, 0.07f);  // Brown bowl
    drawCircle(waterBowlX, waterBowlY, 0.0f, waterBowlRadius, waterBowlRadius, 32);
    glColor3f(0.4f, 0.75f, 0.95f);  // Blue water
    drawCircle(waterBowlX, waterBowlY, 0.0f, waterBowlRadius * 0.8f, waterBowlRadius * 0.8f, 32);
}

void drawWindEffect() {
    if (!windActive) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.9f, 0.9f, 0.9f, 0.3f);  // Translucent white lines for wind
    glBegin(GL_LINES);
    for (int i = 0; i < 20; ++i) {
        float x = randFloat(-1.0f, 1.0f);
        float y = randFloat(-1.0f, 1.0f);
        glVertex2f(x, y);
        glVertex2f(x + windForce * 10.0f, y);
    }
    glEnd();
    glDisable(GL_BLEND);
}

void drawFog() {
    if (!fogActive) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.7f, 0.7f, 0.7f, 0.5f);  // Gray fog overlay
    glBegin(GL_QUADS);
    glVertex2f(-1, -1);
    glVertex2f(1, -1);
    glVertex2f(1, 1);
    glVertex2f(-1, 1);
    glEnd();
    glDisable(GL_BLEND);
}

void drawCloud(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, -1.0f);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Create a fluffy 3D cloud using multiple overlapping spheres
    // Main cloud body with gradient coloring
    float baseAlpha = 0.75f;
    
    // Back layers (darker, for depth)
    glColor4f(0.85f, 0.85f, 0.85f, baseAlpha * 0.6f);
    glutSolidSphere(0.07f, 16, 16);
    
    glColor4f(0.80f, 0.80f, 0.80f, baseAlpha * 0.5f);
    glPushMatrix();
    glTranslatef(-0.08f, -0.02f, -0.03f);
    glutSolidSphere(0.065f, 16, 16);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0.08f, -0.02f, -0.03f);
    glutSolidSphere(0.065f, 16, 16);
    glPopMatrix();
    
    // Middle layers (medium brightness)
    glColor4f(0.92f, 0.92f, 0.92f, baseAlpha * 0.75f);
    glPushMatrix();
    glTranslatef(-0.12f, 0.0f, 0.0f);
    glutSolidSphere(0.06f, 16, 16);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0.12f, 0.0f, 0.0f);
    glutSolidSphere(0.06f, 16, 16);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0.0f, 0.06f, 0.0f);
    glutSolidSphere(0.07f, 16, 16);
    glPopMatrix();
    
    // Front layers (brightest, for highlights)
    glColor4f(0.98f, 0.98f, 0.98f, baseAlpha * 0.85f);
    glPushMatrix();
    glTranslatef(-0.05f, 0.04f, 0.04f);
    glutSolidSphere(0.055f, 16, 16);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0.05f, 0.04f, 0.04f);
    glutSolidSphere(0.055f, 16, 16);
    glPopMatrix();
    
    // Extra puffs for fluffiness
    glColor4f(0.95f, 0.95f, 0.95f, baseAlpha * 0.7f);
    glPushMatrix();
    glTranslatef(-0.14f, 0.03f, 0.02f);
    glutSolidSphere(0.04f, 14, 14);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0.14f, 0.03f, 0.02f);
    glutSolidSphere(0.04f, 14, 14);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(0.0f, -0.05f, 0.03f);
    glutSolidSphere(0.045f, 14, 14);
    glPopMatrix();
    
    // Subtle shadow on bottom (darker, for depth perception)
    glColor4f(0.7f, 0.7f, 0.75f, baseAlpha * 0.4f);
    glPushMatrix();
    glTranslatef(0.0f, -0.08f, 0.0f);
    glScalef(1.0f, 0.3f, 1.0f);
    glutSolidSphere(0.09f, 16, 16);
    glPopMatrix();
    
    // Additional cloud puff 1 (right side)
    glColor4f(0.88f, 0.88f, 0.88f, baseAlpha * 0.65f);
    glPushMatrix();
    glTranslatef(0.20f, 0.02f, 0.01f);
    glutSolidSphere(0.065f, 16, 16);
    glPopMatrix();
    
    glColor4f(0.94f, 0.94f, 0.94f, baseAlpha * 0.7f);
    glPushMatrix();
    glTranslatef(0.26f, 0.05f, 0.02f);
    glutSolidSphere(0.05f, 14, 14);
    glPopMatrix();
    
    // Additional cloud puff 2 (left side)
    glColor4f(0.88f, 0.88f, 0.88f, baseAlpha * 0.65f);
    glPushMatrix();
    glTranslatef(-0.20f, 0.02f, 0.01f);
    glutSolidSphere(0.065f, 16, 16);
    glPopMatrix();
    
    glColor4f(0.94f, 0.94f, 0.94f, baseAlpha * 0.7f);
    glPushMatrix();
    glTranslatef(-0.26f, 0.05f, 0.02f);
    glutSolidSphere(0.05f, 14, 14);
    glPopMatrix();
    
    glDisable(GL_BLEND);
    glPopMatrix();
}

void drawRain() {
    if (!rainActive) return;
    glColor4f(0.5f, 0.7f, 1.0f, 0.7f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    for (int i = 0; i < NUM_RAINDROPS; ++i) {
        glVertex3f(rain[i].x, rain[i].y, 0.0f);
        glVertex3f(rain[i].x - windForce * 0.1f, rain[i].y + 0.02f, 0.0f);
    }
    glEnd();
    glLineWidth(1.0f);
}




void drawHistogram() {
    // Smart auto-scaling: compute max from history
    float maxKills = 0.0f;
    for (int j = 0; j < HISTOGRAM_SIZE; ++j) {
        int idx = (historyIndex - j + HISTOGRAM_SIZE) % HISTOGRAM_SIZE;
        if (killedHistory[idx] > maxKills) maxKills = killedHistory[idx];
    }
    float maxBarHeight = 0.18f;  // Max bar height in screen units
    float heightScale = (maxKills > 0.0f) ? maxBarHeight / maxKills : 0.01f;

    float barWidth = 0.02f;
    float spacing = 0.005f;
    float startX = 0.5f;
    float baseY = -0.95f;
    float depth = 0.004f;  // Extrusion depth for 3D effect

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // Title
    displayText(startX, baseY + 0.22f, "Kills (Recent Frames):", GLUT_BITMAP_HELVETICA_12);

    // Y-axis labels
    char buf[32];
    sprintf(buf, "%d", (int)maxKills);
    displayText(startX - 0.1f, baseY + maxBarHeight + 0.015f, buf, GLUT_BITMAP_HELVETICA_10);
    displayText(startX - 0.1f, baseY - 0.025f, "0", GLUT_BITMAP_HELVETICA_10);

    // Axes (thin gray lines)
    glColor3f(0.4f, 0.4f, 0.4f);
    glLineWidth(1.2f);
    glBegin(GL_LINES);
    // X-axis
    glVertex3f(startX - 0.03f, baseY, 0.0f);
    glVertex3f(startX + HISTOGRAM_SIZE * (barWidth + spacing), baseY, 0.0f);
    // Y-axis
    glVertex3f(startX - 0.03f, baseY, 0.0f);
    glVertex3f(startX - 0.03f, baseY + maxBarHeight, 0.0f);
    glEnd();

    // Horizontal grid lines (faint)
    glColor4f(0.4f, 0.4f, 0.4f, 0.5f);
    glLineWidth(0.8f);
    glBegin(GL_LINES);
    for (float gy = 0.25f; gy < 1.0f; gy += 0.25f) {
        float y = baseY + gy * maxBarHeight;
        glVertex3f(startX - 0.02f, y, 0.0f);
        glVertex3f(startX + HISTOGRAM_SIZE * (barWidth + spacing) * 0.95f, y, 0.0f);
    }
    glEnd();
    glLineWidth(1.0f);

    // 3D Bars (back-to-front order for depth)
    for (int i = 0; i < HISTOGRAM_SIZE; ++i) {
        int idx = (historyIndex - i + HISTOGRAM_SIZE) % HISTOGRAM_SIZE;
        float kills = (float)killedHistory[idx];
        float barHeight = kills * heightScale;
        if (barHeight < 0.001f) continue;  // Skip zero-height

        float x1 = startX + i * (barWidth + spacing);
        float x2 = x1 + barWidth;
        float z1 = 0.0f;   // Front
        float z2 = -depth; // Back

        float recency = 1.0f - (float)i / HISTOGRAM_SIZE;  // Recent brighter
        float r = 0.9f * recency;
        float g = 0.2f * recency;
        float b = 0.2f;

        // Drop shadow (shifted, semi-transparent black)
        glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
        glBegin(GL_QUADS);
        glVertex3f(x1 + 0.002f, baseY, z1 + 0.002f);
        glVertex3f(x2 + 0.002f, baseY, z1 + 0.002f);
        glVertex3f(x2 + 0.002f, baseY + barHeight, z1 + 0.002f);
        glVertex3f(x1 + 0.002f, baseY + barHeight, z1 + 0.002f);
        glEnd();

        // Back face (darkest)
        glColor3f(r * 0.6f, g * 0.6f, b * 0.6f);
        glBegin(GL_QUADS);
        glVertex3f(x1, baseY, z2);
        glVertex3f(x2, baseY, z2);
        glVertex3f(x2, baseY + barHeight, z2);
        glVertex3f(x1, baseY + barHeight, z2);
        glEnd();

        // Left side (darker)
        glColor3f(r * 0.7f, g * 0.7f, b * 0.7f);
        glBegin(GL_QUADS);
        glVertex3f(x1, baseY, z2);
        glVertex3f(x1, baseY, z1);
        glVertex3f(x1, baseY + barHeight, z1);
        glVertex3f(x1, baseY + barHeight, z2);
        glEnd();

        // Right side
        glBegin(GL_QUADS);
        glVertex3f(x2, baseY, z1);
        glVertex3f(x2, baseY, z2);
        glVertex3f(x2, baseY + barHeight, z2);
        glVertex3f(x2, baseY + barHeight, z1);
        glEnd();

        // Front face (brightest)
        glColor3f(r, g, b);
        glBegin(GL_QUADS);
        glVertex3f(x1, baseY, z1);
        glVertex3f(x2, baseY, z1);
        glVertex3f(x2, baseY + barHeight, z1);
        glVertex3f(x1, baseY + barHeight, z1);
        glEnd();

        // Top face (lightest, highlight)
        glColor3f(r * 1.3f, g * 1.3f, std::min(1.0f, b * 1.3f));
        glBegin(GL_QUADS);
        glVertex3f(x1, baseY + barHeight, z1);
        glVertex3f(x2, baseY + barHeight, z1);
        glVertex3f(x2, baseY + barHeight, z2);
        glVertex3f(x1, baseY + barHeight, z2);
        glEnd();
    }

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}

// --- Logic Helpers ---
void doSpray(float x, float y) {
    if (sprayCharges <= 0) {
        snprintf(popupText, sizeof(popupText), "No spray charges left!");
        popupTimer = POPUP_DURATION;
        #ifdef _WIN32
        Beep(350, 150);
        #else
        fprintf(stderr, "Audio: No spray charges left!\n");
        #endif
        return;
    }
    spraying = true;
    sprayX = x;
    sprayY = y;
    sprayRadius = 0.2f;
    sprayTimer = 30;
    sprayCharges--;
    
    // Add visual particles for spray effect
    for (int i = 0; i < 8; ++i) {
        float angle = i * 2.0f * 3.1415926f / 8.0f;
        float px = x + cosf(angle) * 0.08f;
        float py = y + sinf(angle) * 0.08f;
        Larva particle = {px, py, 0.005f, 0};
        larvae.push_back(particle);
    }
    
    snprintf(popupText, sizeof(popupText), "Spraying! Charges left: %d", sprayCharges);
    popupTimer = POPUP_DURATION;
    #ifdef _WIN32
    Beep(1100, 150);
    Beep(900, 100);
    #else
    fprintf(stderr, "Audio: Spraying!\n");
    #endif
}

bool isNearPondArea(float x, float y) {
    float rx = POND_RX * 1.5f, ry = POND_RY * 2.0f;
    float nx = (x - POND_X) / rx, ny = (y - POND_Y) / ry;
    // Check left side and lower area
    return (nx * nx + ny * ny) <= 1.0f && x < POND_X && y < POND_Y;
}

bool isNearWaterBowl(float x, float y) {
    if (!waterBowlVisible) return false;
    float dx = x - waterBowlX, dy = y - waterBowlY;
    return sqrtf(dx * dx + dy * dy) <= waterBowlRadius * 1.8f;
}

void spawnOneMosquito(bool pondBoost) {
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
        if (!mosquitoes[i].alive) {
            bool useBowl = waterBowlVisible && (randFloat(0.0f, 1.0f) < 0.5f);
            float angle = randFloat(0.0f, 2.0f * 3.1415926f);
            if (pondBoost && useBowl) {
                mosquitoes[i].x = waterBowlX + cosf(angle) * waterBowlRadius * 1.3f;
                mosquitoes[i].y = waterBowlY + sinf(angle) * waterBowlRadius * 1.3f;
            } else if (pondBoost) {
                float rx = POND_RX * 0.95f, ry = POND_RY * 0.95f;
                mosquitoes[i].x = POND_X + cosf(angle) * rx;
                mosquitoes[i].y = POND_Y + sinf(angle) * ry;
            } else {
                mosquitoes[i].x = randFloat(-0.95f, 0.95f);
                mosquitoes[i].y = randFloat(-0.95f, 0.95f);
            }
            mosquitoes[i].z = randFloat(0.05f, 0.1f);
            mosquitoes[i].dx = randFloat(-0.005f, 0.005f);
            mosquitoes[i].dy = randFloat(-0.005f, 0.005f);
            mosquitoes[i].size = randFloat(0.03f, 0.06f);
            mosquitoes[i].alive = true;
            mosquitoes[i].deadTimer = 0;
            mosquitoes[i].attractedToPond = (randFloat(0.0f, 1.0f) < 0.5f);
            mosquitoes[i].pondTime = 0;
            totalAlive++;
            #ifdef _WIN32
            Beep(1050, 60);
            #else
            fprintf(stderr, "Audio: Mosquito spawned!\n");
            #endif
            return;
        }
    }
}

void checkSprayCollisions(int& killedThisFrame) {
    if (!spraying) return;
    int killedThisSpray = 0;
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
        if (!mosquitoes[i].alive) continue;
        float dx = mosquitoes[i].x - sprayX, dy = mosquitoes[i].y - sprayY;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist <= sprayRadius + mosquitoes[i].size * 0.5f) {
            mosquitoes[i].alive = false;
            mosquitoes[i].deadTimer = 60 + (int)(randFloat(0.0f, 1.0f) * 40);
            totalAlive--;
            totalKilled++;
            killedThisSpray++;
            mosquitoes[i].x += randFloat(-0.05f, 0.05f);
            mosquitoes[i].y += randFloat(-0.05f, 0.05f);
            #ifdef _WIN32
            Beep(950, 90);
            #else
            fprintf(stderr, "Audio: Mosquito killed!\n");
            #endif
        }
    }
    for (size_t i = 0; i < larvae.size(); ++i) {
        float dx = larvae[i].x - sprayX, dy = larvae[i].y - sprayY;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist <= sprayRadius) {
            larvae.erase(larvae.begin() + i);
            --i;
            totalKilled++;
            killedThisSpray++;
            #ifdef _WIN32
            Beep(950, 90);
            #else
            fprintf(stderr, "Audio: Larva killed!\n");
            #endif
        }
    }
    if (killedThisSpray > 0) {
        killedThisFrame += killedThisSpray;
        snprintf(popupText, sizeof(popupText), "Spray killed %d mosquitoes/larvae!", killedThisSpray);
        popupTimer = POPUP_DURATION;
    }
}

void updateMosquitoesLogic() {
    int killedThisFrame = 0;
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
        if (mosquitoes[i].alive) {
            if (mosquitoes[i].attractedToPond) {
                float targetX = POND_X, targetY = POND_Y;
                if (waterBowlVisible && randFloat(0.0f, 1.0f) < 0.5f) {
                    targetX = waterBowlX;
                    targetY = waterBowlY;
                }
                float dx = targetX - mosquitoes[i].x, dy = targetY - mosquitoes[i].y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist > 0.02f) {
                    float speed = 0.002f;
                    mosquitoes[i].dx += (dx / dist) * speed;
                    mosquitoes[i].dy += (dy / dist) * speed;
                    float speedLimit = 0.008f;
                    float currentSpeed = sqrtf(mosquitoes[i].dx * mosquitoes[i].dx + mosquitoes[i].dy * mosquitoes[i].dy);
                    if (currentSpeed > speedLimit) {
                        mosquitoes[i].dx = (mosquitoes[i].dx / currentSpeed) * speedLimit;
                        mosquitoes[i].dy = (mosquitoes[i].dy / currentSpeed) * speedLimit;
                    }
                }
            }
            mosquitoes[i].x += mosquitoes[i].dx + (windActive ? windForce : 0.0f);
            mosquitoes[i].y += mosquitoes[i].dy;
            mosquitoes[i].z = 0.05f + 0.05f * sinf((float)glutGet(GLUT_ELAPSED_TIME) * 0.005f);
            if (mosquitoes[i].x < -0.95f || mosquitoes[i].x > 0.95f) mosquitoes[i].dx = -mosquitoes[i].dx;
            if (mosquitoes[i].y < -0.95f || mosquitoes[i].y > 0.95f) mosquitoes[i].dy = -mosquitoes[i].dy;
            if (randFloat(0.0f, 1.0f) * 800 < 10) {
                mosquitoes[i].dx += randFloat(-0.003f, 0.003f);
                mosquitoes[i].dy += randFloat(-0.003f, 0.003f);
            }
            if ((isNearPondArea(mosquitoes[i].x, mosquitoes[i].y) || isNearWaterBowl(mosquitoes[i].x, mosquitoes[i].y)) && larvae.size() < MAX_LARVAE) {
                mosquitoes[i].pondTime++;
                if (mosquitoes[i].pondTime > 150) {
                    Larva larva = {mosquitoes[i].x + randFloat(-0.03f, 0.03f), mosquitoes[i].y + randFloat(-0.03f, 0.03f), 0.01f, 0};
                    larvae.push_back(larva);
                    mosquitoes[i].pondTime = 0;
                    #ifdef _WIN32
                    Beep(1150, 120);
                    #else
                    fprintf(stderr, "Audio: Larva spawned!\n");
                    #endif
                    snprintf(popupText, sizeof(popupText), "Larva spawned in %s!", isNearPondArea(mosquitoes[i].x, mosquitoes[i].y) ? "pond" : "water bowl");
                    popupTimer = POPUP_DURATION;
                }
            } else {
                mosquitoes[i].pondTime = 0;
            }
        } else if (mosquitoes[i].deadTimer > 0) {
            mosquitoes[i].deadTimer--;
        }
    }
    for (size_t i = 0; i < larvae.size(); ++i) {
        larvae[i].timer++;
        larvae[i].size += 0.0001f;
        if (larvae[i].size > 0.015f) larvae[i].size = 0.015f;
        if (larvae[i].timer > 350) {
            spawnOneMosquito(true);
            larvae.erase(larvae.begin() + i);
            --i;
            #ifdef _WIN32
            Beep(1250, 120);
            #else
            fprintf(stderr, "Audio: Larva matured!\n");
            #endif
            snprintf(popupText, sizeof(popupText), "Larva matured into mosquito!");
            popupTimer = POPUP_DURATION;
        }
    }
    if (rainActive) {
        rainTimer--;
        if (rainTimer <= 0) {
            rainActive = false;
            snprintf(popupText, sizeof(popupText), "Rain stopped. Check breeding sites!");
            popupTimer = POPUP_DURATION;
        }
        for (int i = 0; i < NUM_RAINDROPS; ++i) {
            rain[i].y -= rain[i].speed;
            rain[i].x += windForce * 0.5f;
            if (rain[i].y < -1.0f) {
                rain[i].y = 1.0f;
                rain[i].x = randFloat(-1.0f, 1.0f);
            }
        }
    } else if (randFloat(0.0f, 1.0f) * 900 < 6 && !cleanupTimer) {
        rainActive = true;
        rainTimer = RAIN_DURATION;
        for (int i = 0; i < RAIN_SPAWN_COUNT; ++i) spawnOneMosquito(true);
        snprintf(popupText, sizeof(popupText), "Rain event! %d mosquitoes spawned!", RAIN_SPAWN_COUNT);
        popupTimer = POPUP_DURATION;
        #ifdef _WIN32
        Beep(550, 350);
        #else
        fprintf(stderr, "Audio: Rain event!\n");
        #endif
    }
    if (windActive) {
        windTimer--;
        if (windTimer <= 0) windActive = false;
        treeSwayAngle = sinf((float)windTimer * 0.1f) * 10.0f;
    } else if (randFloat(0.0f, 1.0f) * 900 < 5 && !cleanupTimer) {
        windActive = true;
        windTimer = WIND_DURATION;
        windForce = randFloat(-0.006f, 0.006f);
        snprintf(popupText, sizeof(popupText), "Wind event! Mosquitoes shifted %s!", windForce > 0 ? "right" : "left");
        popupTimer = POPUP_DURATION;
        #ifdef _WIN32
        Beep(750, 250);
        #else
        fprintf(stderr, "Audio: Wind event!\n");
        #endif
    }
    if (fogActive) {
        fogTimer--;
        if (fogTimer <= 0) fogActive = false;
    } else if (randFloat(0.0f, 1.0f) * 900 < 4 && !cleanupTimer) {
        fogActive = true;
        fogTimer = FOG_DURATION;
        snprintf(popupText, sizeof(popupText), "Fog event! Visibility reduced!");
        popupTimer = POPUP_DURATION;
        #ifdef _WIN32
        Beep(650, 250);
        #else
        fprintf(stderr, "Audio: Fog event!\n");
        #endif
    }
    if (cleanupTimer > 0) {
        cleanupTimer--;
        if (cleanupTimer <= 0) {
            currentSpawnInterval = SPAWN_INTERVAL_NORMAL;
            snprintf(popupText, sizeof(popupText), "Cleanup ended. Monitor breeding sites!");
            popupTimer = POPUP_DURATION;
        }
    } else if (randFloat(0.0f, 1.0f) * 900 < 3 && !rainActive && !windActive && !fogActive) {
        cleanupTimer = CLEANUP_DURATION;
        waterBowlVisible = false;
        larvae.clear();
        currentSpawnInterval = SPAWN_INTERVAL_NORMAL * 2;
        snprintf(popupText, sizeof(popupText), "Cleanup campaign! Breeding sites cleared!");
        popupTimer = POPUP_DURATION;
        #ifdef _WIN32
        Beep(1550, 350);
        #else
        fprintf(stderr, "Audio: Cleanup event!\n");
        #endif
    }
    if (randFloat(0.0f, 1.0f) * 900 < 3 && !rainActive && !windActive && !fogActive && !cleanupTimer) {
        int swarmCount = 4 + (int)(randFloat(0.0f, 1.0f) * 4);
        for (int i = 0; i < swarmCount; ++i) spawnOneMosquito(true);
        snprintf(popupText, sizeof(popupText), "Mosquito swarm! %d spawned!", swarmCount);
        popupTimer = POPUP_DURATION;
        #ifdef _WIN32
        Beep(1050, 250);
        #else
        fprintf(stderr, "Audio: Swarm event!\n");
        #endif
    }
    bool boost = waterBowlVisible || totalAlive > 8 || rainActive || cleanupTimer > 0;
    int nearPondCount = 0, nearBowlCount = 0;
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
        if (mosquitoes[i].alive) {
            if (isNearPondArea(mosquitoes[i].x, mosquitoes[i].y)) nearPondCount++;
            if (isNearWaterBowl(mosquitoes[i].x, mosquitoes[i].y)) nearBowlCount++;
        }
    }
    if (nearPondCount > 3 || nearBowlCount > 2) boost = true;
    currentSpawnInterval = boost && !cleanupTimer ? SPAWN_INTERVAL_HIGH : SPAWN_INTERVAL_NORMAL;
    spawnCounter++;
    if (spawnCounter >= currentSpawnInterval) {
        spawnOneMosquito(boost);
        if (randFloat(0.0f, 1.0f) * 100 < 50 && !cleanupTimer) spawnOneMosquito(boost);
        spawnCounter = 0;
    }
    difficultyTimer++;
    if (difficultyTimer >= 4800) {
        currentSpawnInterval = std::max(40, currentSpawnInterval - 10);
        difficultyTimer = 0;
        snprintf(popupText, sizeof(popupText), "Difficulty increased! Faster mosquito spawns!");
        popupTimer = POPUP_DURATION;
        #ifdef _WIN32
        Beep(1300, 200);
        #else
        fprintf(stderr, "Audio: Difficulty increased!\n");
        #endif
    }
    if (spraying) {
        sprayTimer--;
        if (sprayTimer <= 0) spraying = false;
    }
    if (popupTimer > 0) popupTimer--;
    cloudOffset += dayTime ? 0.0005f : 0.0002f;
    if (cloudOffset > 2.0f) cloudOffset = -2.0f;
    if (totalAlive > 40) {
        snprintf(popupText, sizeof(popupText), "Game Over! Too many mosquitoes! Restarting...");
        popupTimer = POPUP_DURATION * 2;
        initializeMosquitoes();
        #ifdef _WIN32
        Beep(300, 500);
        #else
        fprintf(stderr, "Audio: Game over!\n");
        #endif
    }
    static int sprayRechargeTimer = 0;
    sprayRechargeTimer++;
    if (sprayRechargeTimer >= 600 && sprayCharges < MAX_SPRAY_CHARGES) {
        sprayCharges++;
        sprayRechargeTimer = 0;
        snprintf(popupText, sizeof(popupText), "Spray charge recharged! Charges: %d", sprayCharges);
        popupTimer = POPUP_DURATION;
        #ifdef _WIN32
        Beep(1200, 200);
        #else
        fprintf(stderr, "Audio: Spray recharged!\n");
        #endif
    }
    updateHistogram(killedThisFrame);
    checkSprayCollisions(killedThisFrame);
}

// --- UI Display ---
void displayUI() {
    // Top-left info panel (smart, compact, status-rich)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Panel background
    float panelL = -0.98f, panelR = -0.48f, panelT = 0.98f, panelB = 0.62f;
    glColor4f(0.06f, 0.06f, 0.07f, 0.78f);
    glBegin(GL_QUADS);
      glVertex2f(panelL, panelT);
      glVertex2f(panelR, panelT);
      glVertex2f(panelR, panelB);
      glVertex2f(panelL, panelB);
    glEnd();

    // Thin border
    glColor4f(0.9f, 0.9f, 0.9f, 0.07f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
      glVertex2f(panelL, panelT);
      glVertex2f(panelR, panelT);
      glVertex2f(panelR, panelB);
      glVertex2f(panelL, panelB);
    glEnd();

    // Content text color (legible)
    glColor3f(1.0f, 1.0f, 1.0f);

    char buf[128];
    // Title
    displayText(panelL + 0.02f, panelT - 0.04f, "Dengue Awareness", GLUT_BITMAP_HELVETICA_18);

    // Key stats
    snprintf(buf, sizeof(buf), "Alive: %d", totalAlive);
    displayText(panelL + 0.02f, panelT - 0.12f, buf);
    snprintf(buf, sizeof(buf), "Killed: %d", totalKilled);
    displayText(panelL + 0.02f, panelT - 0.18f, buf);

    // Recent kills (last history slot)
    int lastIdx = (historyIndex - 1 + HISTOGRAM_SIZE) % HISTOGRAM_SIZE;
    int recentKills = killedHistory[lastIdx];
    snprintf(buf, sizeof(buf), "Recent: %d", recentKills);
    displayText(panelL + 0.02f, panelT - 0.24f, buf);

    // Spawn rate
    const char* spawnText = (currentSpawnInterval <= SPAWN_INTERVAL_HIGH ? "High" : "Normal");
    snprintf(buf, sizeof(buf), "Spawn Rate: %s", spawnText);
    displayText(panelL + 0.02f, panelT - 0.30f, buf);

    // Water bowl status
    snprintf(buf, sizeof(buf), "Water Bowl: %s", waterBowlVisible ? "On" : "Off");
    displayText(panelL + 0.02f, panelT - 0.36f, buf);

    // Spray charges visual bar (segmented)
    float barX = panelL + 0.02f;
    float barY = panelT - 0.44f;
    displayText(barX, barY + 0.02f, "Spray:");
    float segW = 0.026f, segH = 0.04f, gap = 0.008f;
    for (int i = 0; i < MAX_SPRAY_CHARGES; ++i) {
        float sx = barX + 0.06f + i * (segW + gap);
        float sy = barY - 0.02f;
        if (i < sprayCharges) {
            // available charge
            glColor4f(0.12f, 0.72f, 0.95f, 0.95f);
        } else {
            glColor4f(0.25f, 0.25f, 0.25f, 0.85f);
        }
        glBegin(GL_QUADS);
          glVertex2f(sx, sy + segH);
          glVertex2f(sx + segW, sy + segH);
          glVertex2f(sx + segW, sy);
          glVertex2f(sx, sy);
        glEnd();
        // subtle highlight
        glColor4f(1.0f, 1.0f, 1.0f, 0.06f);
        glBegin(GL_QUADS);
          glVertex2f(sx, sy + segH);
          glVertex2f(sx + segW, sy + segH);
          glVertex2f(sx + segW, sy + segH * 0.6f);
          glVertex2f(sx, sy + segH * 0.6f);
        glEnd();
    }

    // Status indicators (rain / wind / fog / cleanup)
    float stX = panelL + 0.02f;
    float stY = panelT - 0.62f;
    float dotR = 0.017f;
    auto drawDot = [&](float cx, float cy, float r, float g, float b, bool on){
        if (on) glColor3f(r,g,b); else glColor3f(0.25f,0.25f,0.25f);
        glBegin(GL_TRIANGLE_FAN);
          glVertex2f(cx, cy);
          for (int a = 0; a <= 16; ++a) {
            float ang = a * 2.0f * 3.1415926f / 16.0f;
            glVertex2f(cx + cosf(ang) * dotR, cy + sinf(ang) * dotR);
          }
        glEnd();
    };

    // Rain
    drawDot(stX + 0.01f, stY + 0.00f, 0.0f, 0.6f, 1.0f, rainActive);
    displayText(stX + 0.05f, stY + 0.0f, "Rain");
    // Wind
    drawDot(stX + 0.01f, stY - 0.07f, 1.0f, 1.0f, 1.0f, windActive);
    displayText(stX + 0.05f, stY - 0.07f, "Wind");
    // Fog
    drawDot(stX + 0.01f, stY - 0.14f, 0.8f, 0.85f, 0.95f, fogActive);
    displayText(stX + 0.05f, stY - 0.14f, "Fog");
    // Cleanup
    bool cleaning = (cleanupTimer > 0);
    drawDot(stX + 0.01f, stY - 0.21f, 0.2f, 0.9f, 0.2f, cleaning);
    displayText(stX + 0.05f, stY - 0.21f, "Cleanup");

    // Danger indicator if mosquitoes high
    float dangerX = panelR - 0.12f, dangerY = panelT - 0.14f;
    if (totalAlive > 35) {
        // big red warning
        glColor3f(1.0f, 0.2f, 0.2f);
        displayText(dangerX, dangerY, "!!! DANGER !!!", GLUT_BITMAP_HELVETICA_18);
    } else if (totalAlive > 20) {
        glColor3f(1.0f, 0.6f, 0.1f);
        displayText(dangerX, dangerY, "High mosquito load", GLUT_BITMAP_HELVETICA_12);
    } else {
        glColor3f(0.6f, 1.0f, 0.6f);
        displayText(dangerX, dangerY, "Mosquitoes nominal", GLUT_BITMAP_HELVETICA_12);
    }

    glDisable(GL_BLEND);
}

void displayInstructions() {
    const char* lines[] = {
        "Controls:",
        "Left-Click: Spray (if charges available)",
        "S: Random Spray",
        "R: Toggle Water Bowl",
        "T: Trigger Rain",
        "D: Toggle Day/Night",
        "Right-Click & Drag: Move Water Bowl",
        "Right-Click: Menu",
        "ESC: Exit"
    };

    float y = 0.75f;

    // Silver text color, no background or border
    glColor3f(0.75f, 0.75f, 0.78f); // silver-like color
    displayText(0.58f, y, lines[0], GLUT_BITMAP_HELVETICA_18);
    y -= 0.05f;

    for (int i = 1; i < (int)(sizeof(lines) / sizeof(lines[0])); ++i) {
        displayText(0.58f, y, lines[i], GLUT_BITMAP_HELVETICA_12);
        y -= 0.04f;
    }
}

void displayPopup() {
    if (popupTimer <= 0) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.9f, 0.9f, 0.2f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(-0.5f, 0.05f);
    glVertex2f(0.5f, 0.05f);
    glVertex2f(0.5f, 0.25f);
    glVertex2f(-0.5f, 0.25f);
    glEnd();
    glColor3f(0.0f, 0.0f, 0.0f);
    displayText(-0.45f, 0.15f, popupText, GLUT_BITMAP_TIMES_ROMAN_24);
    glDisable(GL_BLEND);
}


void display() {

    // Ensure viewport matches current window

    glViewport(0, 0, windowWidth, windowHeight);

    float aspect = (windowHeight > 0) ? (float)windowWidth / (float)windowHeight : 1.0f;



    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



    // 3D perspective camera

    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

    gluPerspective(45.0, (double)aspect, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();

    gluLookAt(0, 0, 4.0, 0, 0, 0, 0, 1, 0);

    glEnable(GL_DEPTH_TEST);



    // --- Rich Sky (day/night) ---

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



    float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;



    if (dayTime) {

        // Day gradient

        glBegin(GL_QUADS);

        glColor3f(0.08f, 0.45f, 0.85f); glVertex3f(-10.0f, 10.0f, -9.5f);

        glColor3f(0.18f, 0.66f, 0.92f); glVertex3f(10.0f, 10.0f, -9.5f);

        glColor3f(0.95f, 0.98f, 0.99f); glVertex3f(10.0f, -1.2f, -9.5f);

        glColor3f(0.88f, 0.95f, 0.98f); glVertex3f(-10.0f, -1.2f, -9.5f);

        glEnd();



        // Sun halo

        glPushMatrix();

        glTranslatef(0.75f, 0.75f, -9.0f);

        for (int i = 5; i >= 1; --i) {

            float a = 0.14f * (i / 5.0f) * (0.9f + 0.1f * sinf(time * 1.3f));

            float r = 0.09f + i * 0.03f;

            glColor4f(1.0f, 0.94f, 0.6f, a);

            drawCircle(0.0f, 0.0f, 0.0f, r, 48);

        }

        glColor4f(1.0f, 0.98f, 0.7f, 1.0f);

        drawCircle(0.0f, 0.0f, 0.0f, 0.06f, 32);

        glPopMatrix();



        // Atmospheric haze

        for (int layer = 0; layer < 3; ++layer) {

            float h = -0.9f + layer * 0.08f;

            float alpha = 0.06f * (1.0f - layer * 0.25f);

            glColor4f(1.0f, 0.86f, 0.6f, alpha);

            glBegin(GL_QUADS);

            glVertex3f(-10.0f, h - 0.05f, -9.4f);

            glVertex3f(10.0f, h - 0.05f, -9.4f);

            glVertex3f(10.0f, h + 0.12f, -9.4f);

            glVertex3f(-10.0f, h + 0.12f, -9.4f);

            glEnd();

        }



        // Mountains

        auto drawMountain = [&](float baseY, float dark, float offset, float scale) {

            glColor3f(0.06f * dark + 0.05f, 0.16f * dark + 0.08f, 0.10f * dark + 0.07f);

            glBegin(GL_TRIANGLE_STRIP);

            for (float x = -1.2f; x <= 1.2f; x += 0.05f) {

                float peak = 0.06f + 0.12f * sinf((x + offset) * 3.4f) * (0.8f + 0.2f * cosf(time * 0.2f + x * 2.0f));

                glVertex3f(x, baseY - 0.3f + peak * scale, -9.2f);

                glVertex3f(x, -1.2f, -9.2f);

            }

            glEnd();

        };

        drawMountain(-0.25f, 0.6f, 0.2f, 1.0f);

        drawMountain(-0.32f, 0.85f, -0.5f, 1.4f);



        // Day clouds

        for (int i = 0; i < 3; ++i) {

            float cx = -0.9f + cloudOffset * 0.6f + i * 0.9f;

            float cy = 0.55f + 0.05f * i;

            drawCloud(cx, cy);

        }

    } else {

        // Night gradient

        glBegin(GL_QUADS);

        glColor3f(0.02f, 0.04f, 0.18f); glVertex3f(-10.0f, 10.0f, -9.5f);

        glColor3f(0.05f, 0.08f, 0.28f); glVertex3f(10.0f, 10.0f, -9.5f);

        glColor3f(0.08f, 0.10f, 0.18f); glVertex3f(10.0f, -1.2f, -9.5f);

        glColor3f(0.03f, 0.06f, 0.12f); glVertex3f(-10.0f, -1.2f, -9.5f);

        glEnd();



        // Moon

        glPushMatrix();

        glTranslatef(0.65f, 0.7f, -9.0f);

        for (int i = 6; i >= 1; --i) {

            float a = 0.08f * (i / 6.0f);

            float r = 0.06f + i * 0.02f;

            glColor4f(0.95f, 0.98f, 1.0f, a * (0.9f + 0.1f * sinf(time * 0.8f)));

            drawCircle(0.0f, 0.0f, 0.0f, r, 40);

        }

        glColor4f(0.98f, 0.98f, 1.0f, 1.0f);

        drawCircle(0.01f, 0.01f, 0.0f, 0.03f, 28);

        glPopMatrix();



        // Stars

        static bool starsInit = false;

        static std::vector<std::pair<float, float>> stars;

        if (!starsInit) {

            starsInit = true;

            std::mt19937 sr(424242);

            std::uniform_real_distribution<float> sx(-1.1f, 1.1f), sy(0.0f, 1.05f);

            for (int i = 0; i < 280; ++i) stars.emplace_back(sx(sr), sy(sr));

        }

        glPointSize(1.8f);

        glBegin(GL_POINTS);

        for (size_t i = 0; i < stars.size(); ++i) {

            float tw = 0.5f + 0.5f * sinf(time * 2.0f + (float)i * 0.13f);

            float bright = 0.6f + 0.4f * tw;

            glColor4f(0.95f * bright, 0.95f * bright, 1.0f * bright, 0.8f);

            glVertex3f(stars[i].first, stars[i].second, -9.2f);

        }

        glEnd();

        glPointSize(1.0f);



        // Milky band

        glColor4f(0.9f, 0.9f, 0.98f, 0.035f);

        glBegin(GL_QUADS);

        glVertex3f(-10.0f, 0.15f, -9.3f);

        glVertex3f(10.0f, 0.15f, -9.3f);

        glVertex3f(10.0f, -0.05f, -9.3f);

        glVertex3f(-10.0f, -0.05f, -9.3f);

        glEnd();



        // Night mountains

        glColor3f(0.02f, 0.02f, 0.04f);

        glBegin(GL_TRIANGLE_STRIP);

        glVertex3f(-1.2f, -0.2f, -9.1f);

        for (float x = -1.2f; x <= 1.2f; x += 0.08f) {

            float peak = 0.08f * (0.5f + 0.5f * sinf(3.0f * x + 0.6f));

            glVertex3f(x, -0.2f + peak, -9.1f);

            glVertex3f(x, -1.2f, -9.1f);

        }

        glEnd();

    }



    glDisable(GL_BLEND);

    glEnable(GL_DEPTH_TEST);

    glPopAttrib();



    // --- Clouds ---

    drawCloud(-0.9f + cloudOffset, 0.8f);

    drawCloud(-0.4f + cloudOffset, 0.85f);

    drawCloud(0.4f + cloudOffset, 0.75f);

    drawCloud(0.8f + cloudOffset, 0.8f);



    // --- Ground ---

    glBegin(GL_QUADS);

    glColor3f(0.3f, 0.6f, 0.3f);

    glVertex3f(-1, -1, -0.1f);

    glVertex3f(1, -1, -0.1f);

    glVertex3f(1, -0.5f, -0.1f);

    glVertex3f(-1, -0.5f, -0.1f);

    glEnd();



    // --- Scene Objects ---

    // Houses rearranged to be spaced out and slightly more right



    // Tree: Large size (2.5) placed far to the left, but within view boundary

    // In display() or wherever you draw trees


    drawPond();

    drawWaterBowl();



    // --- Larvae and Mosquitoes ---

    for (size_t i = 0; i < larvae.size(); ++i)

        if (larvae[i].alive)

            drawLarva(larvae[i].x, larvae[i].y, larvae[i].size);

    for (int i = 0; i < NUM_MOSQUITOES; ++i)

        if (mosquitoes[i].alive)

            drawMosquito(mosquitoes[i].x, mosquitoes[i].y, mosquitoes[i].z,

                         mosquitoes[i].size, 0.5f, 0.3f, 0.1f);



    // --- Spray Effect ---

    if (spraying) {

        glEnable(GL_BLEND);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(0.1f, 0.6f, 1.0f, 0.5f);

        drawCircle(sprayX, sprayY, 0.0f, sprayRadius, 36);

        glDisable(GL_BLEND);

    }



    // --- Environment Effects ---

    drawRain();

    drawWindEffect();

    drawFog();



    // --- 2D UI ---

    glMatrixMode(GL_PROJECTION);

    glLoadIdentity();

    gluOrtho2D(-1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);



    displayUI();

    displayInstructions();

    displayPopup();

    drawHistogram();



    glutSwapBuffers();

    checkGLError("display");

}

// --- Menu Function ---
void menuFunc(int option) {
    switch (option) {
        case MENU_RESTART:
            initializeMosquitoes();
            snprintf(popupText, sizeof(popupText), "Simulation restarted!");
            popupTimer = POPUP_DURATION;
            #ifdef _WIN32
            Beep(1000, 200);
            #endif
            break;

        case MENU_TOGGLE_BOWL:
            waterBowlVisible = !waterBowlVisible;
            snprintf(popupText, sizeof(popupText), waterBowlVisible ?
                     "Water bowl toggled on!" : "Water bowl toggled off!");
            popupTimer = POPUP_DURATION;
            #ifdef _WIN32
            Beep(1050, 200);
            #endif
            break;

        case MENU_TRIGGER_RAIN:
            if (!rainActive) {
                rainActive = true;
                rainTimer = RAIN_DURATION;
                for (int i = 0; i < RAIN_SPAWN_COUNT; ++i)
                    spawnOneMosquito(true);
                snprintf(popupText, sizeof(popupText),
                         "Rain event triggered! %d mosquitoes spawned!", RAIN_SPAWN_COUNT);
                popupTimer = POPUP_DURATION;
                #ifdef _WIN32
                Beep(550, 350);
                #endif
            }
            break;

        case MENU_EXIT:
            exit(0);
    }
    glutPostRedisplay();
}

// --- Keyboard Input ---
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: exit(0); break;
        case 's': case 'S': doSpray(randFloat(-0.95f, 0.95f), randFloat(-0.95f, 0.95f)); break;
        case 'r': case 'R':
            waterBowlVisible = !waterBowlVisible;
            snprintf(popupText, sizeof(popupText), waterBowlVisible ?
                     "Water bowl toggled on!" : "Water bowl toggled off!");
            popupTimer = POPUP_DURATION;
            #ifdef _WIN32
            Beep(1050, 200);
            #endif
            break;
        case 't': case 'T':
            if (!rainActive) {
                rainActive = true;
                rainTimer = RAIN_DURATION;
                for (int i = 0; i < RAIN_SPAWN_COUNT; ++i)
                    spawnOneMosquito(true);
                snprintf(popupText, sizeof(popupText),
                         "Rain event triggered! %d mosquitoes spawned!", RAIN_SPAWN_COUNT);
                popupTimer = POPUP_DURATION;
                #ifdef _WIN32
                Beep(550, 350);
                #endif
            }
            break;
        case 'd': case 'D':
            dayTime = !dayTime;
            snprintf(popupText, sizeof(popupText), dayTime ?
                     "Switched to day!" : "Switched to night!");
            popupTimer = POPUP_DURATION;
            #ifdef _WIN32
            Beep(1000, 200);
            #endif
            break;
    }
    glutPostRedisplay();
}

// --- Mouse Input ---
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && sprayCharges > 0) {
        float wx = screenToWorldX(x, windowWidth);
        float wy = screenToWorldY(y, windowHeight);
        doSpray(wx, wy);
    }

    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN && waterBowlVisible) {
        float wx = screenToWorldX(x, windowWidth);
        float wy = screenToWorldY(y, windowHeight);
        float dx = wx - waterBowlX, dy = wy - waterBowlY;
        if (sqrtf(dx * dx + dy * dy) <= waterBowlRadius * 1.5f) {
            draggingBowl = true;
            lastMouseX = x;
            lastMouseY = y;
        }
    }

    if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
        draggingBowl = false;

    glutPostRedisplay();
}

void motion(int x, int y) {
    if (draggingBowl && waterBowlVisible) {
        waterBowlX = screenToWorldX(x, windowWidth);
        waterBowlY = screenToWorldY(y, windowHeight);
        glutPostRedisplay();
    }
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    windowWidth = w;
    windowHeight = h;
}

void initGL() {
    printf("initGL: Setting up OpenGL\n");
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    initializeMosquitoes();
    initializeRain();
    printf("initGL: Complete\n");
}

void timerFunc(int value) {
    updateMosquitoesLogic();
    glutPostRedisplay();
    glutTimerFunc(16, timerFunc, 0); // ~60 FPS
}

int main(int argc, char** argv) {
    FILE* debugLog = freopen("debug.log", "w", stdout);
    setbuf(debugLog, NULL);
    printf("main: Starting\n");

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_W, WINDOW_H);
    glutCreateWindow("Dengue Awareness Simulation");
    glutFullScreen();

    initGL();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutTimerFunc(16, timerFunc, 0);

    glutCreateMenu(menuFunc);
    glutAddMenuEntry("Restart Simulation", MENU_RESTART);
    glutAddMenuEntry("Toggle Water Bowl", MENU_TOGGLE_BOWL);
    glutAddMenuEntry("Trigger Rain Event", MENU_TRIGGER_RAIN);
    glutAddMenuEntry("Exit", MENU_EXIT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    srand(static_cast<unsigned>(time(nullptr)));  // Only call ONCE
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    glutMainLoop();
    return 0;
}
