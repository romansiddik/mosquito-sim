#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <vector>
#include <math.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h> // For Beep sound
#endif

// ---------------- Configuration ----------------
const int NUM_MOSQUITOES = 30;
const int WINDOW_W = 900;
const int WINDOW_H = 650;
// Pond position (BOTTOM-LEFT)
const float pondX = -0.7f;
const float pondY = -0.75f;
const float pondRadiusX = 0.25f;
const float pondRadiusY = 0.12f;
// Water bowl position (BOTTOM-LEFT, BESIDE POND)
float waterBowlX = -0.4f, waterBowlY = -0.8f, waterBowlRadius = 0.05f;
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
const int rainDuration = 600; // 30s at 50ms
const int rainSpawnCount = 5; // Mosquitoes spawned during rain
// Score
int totalAlive = 0;
int totalKilled = 0;

// --- Environment Cycle ---
int environmentState = 0; // 0=Day, 1=Night, 2=Fog
const int ENV_STATES = 3;

// --- Camera ---
float camX = 0.0f, camY = 0.0f, camZ = 2.0f; // Top-down view
float lookX = 0.0f, lookY = 0.0f, lookZ = 0.0f;

// --- 3D House Rotation ---
float g_rotateY = -30.0f; // Initial rotation to show front and side
float g_rotateX = 20.0f;  // Initial tilt

// Larva tracking
struct Larva {
    float x;      // Position x
    float y;      // Position y
    float size;   // Visual/collision size
    int timer;    // Frame counter for logic (e.g., growth)
    bool alive;   // Flag for active state

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
    float x, y, z;
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
    environmentState = 0; // Start at day
    frameCounter = 0;
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
        mosquitoes[i].x = randFloat(-0.9f, 0.9f);
        mosquitoes[i].y = randFloat(-0.9f, 0.9f);
        mosquitoes[i].z = 0.0f; // Z will be set in display
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
void displayText(const char* text, float x, float y, void* font); // Forward declaration

void drawCircle(float cx, float cy, float rx, float ry, int segments = 48) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < segments; ++i) {
        float a = 2.0f * 3.1415926f * i / segments;
        glVertex3f(cx + cosf(a) * rx, cy + sinf(a) * ry, 0.0f); // Draw at z=0
    }
    glEnd();
}
void drawMosquito(float x, float y, float z, float size, float r, float g, float b, bool bloodFed = false) {
    GLUquadricObj* quadric = gluNewQuadric();
    gluQuadricDrawStyle(quadric, GLU_FILL); 

    glPushMatrix();
    glTranslatef(x, y, z);

    // Thorax
    glPushMatrix();
    glColor3f(r * 0.8f, g * 0.8f, b * 0.8f); 
    glScalef(size * 0.15f, size * 0.1f, size * 0.1f); 
    glutSolidSphere(1.0f, 12, 12);
    glPopMatrix();

    // Head
    glPushMatrix();
    glTranslatef(size * 0.15f, 0.0f, 0.0f); 
    glColor3f(r * 0.5f, g * 0.5f, b * 0.5f); 
    glutSolidSphere(size * 0.08f, 10, 10);
    glPopMatrix();

    // Proboscis
    glPushMatrix();
    glTranslatef(size * 0.23f, 0.0f, 0.0f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f); 
    glColor3f(r * 0.3f, g * 0.3f, b * 0.3f); 
    gluCylinder(quadric, size * 0.01f, size * 0.005f, size * 0.1f, 8, 1); 
    glPopMatrix();

    // Antennae
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(size * 0.15f, size * 0.05f * i, size * 0.05f);
        glRotatef(45.0f, 0.0f, 0.0f, 1.0f); 
        glColor3f(r * 0.4f, g * 0.4f, b * 0.4f);
        gluCylinder(quadric, size * 0.005f, size * 0.002f, size * 0.12f, 6, 1);
        glPopMatrix();
    }

    // Abdomen
    glPushMatrix();
    glTranslatef(-size * 0.15f, 0.0f, 0.0f); 
    if (bloodFed) {
        glColor3f(1.0f, 0.0f, 0.0f); 
    } else {
        glColor3f(r, g, b);
    }
    glScalef(size * 0.2f, size * 0.08f, size * 0.08f); 
    glutSolidSphere(1.0f, 12, 12);
    glPopMatrix();

    // Wings
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
        glTranslatef(0.0f, size * 0.05f * i, size * 0.05f);
        glRotatef(30.0f * i, 1.0f, 0.0f, 0.0f); 
        glColor4f(1.0f, 1.0f, 1.0f, 0.6f); 
        glBegin(GL_TRIANGLES);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(size * 0.3f, size * 0.1f, 0.0f);
        glVertex3f(0.0f, size * 0.2f, 0.0f);
        glEnd();
        glPopMatrix();
    }

    // Legs
    float legAngles[3] = {-45.0f, 0.0f, 45.0f}; 
    for (int leg = 0; leg < 3; ++leg) {
        for (int side = -1; side <= 1; side += 2) {
            glPushMatrix();
            glTranslatef(size * (0.05f - leg * 0.1f), size * 0.05f * side, 0.0f);
            glRotatef(legAngles[leg], 0.0f, 0.0f, 1.0f); 
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
    glTranslatef(x, y, 0.0f); 
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); 

    // Head
    glPushMatrix();
    glTranslatef(0.0f, size * 0.1f, 0.0f);
    glColor4f(0.4f, 0.3f, 0.2f, 0.8f); 
    glutSolidSphere(size * 0.05f, 10, 10);
    glPopMatrix();

    // Mouth brushes
    glColor4f(0.3f, 0.3f, 0.3f, 0.6f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = 0; i < 6; ++i) {
        float angle = i * 60.0f * 3.14159f / 180.0f;
        glVertex3f(0.0f, size * 0.1f, 0.0f);
        glVertex3f(size * 0.03f * sin(angle), size * 0.1f + size * 0.05f, size * 0.03f * cos(angle));
    }
    glEnd();

    // Antennae
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
        glTranslatef(size * 0.03f * side, size * 0.1f, 0.0f);
        glRotatef(45.0f * side, 0.0f, 0.0f, 1.0f);
        glColor4f(0.4f, 0.3f, 0.2f, 0.8f);
        gluCylinder(quadric, size * 0.005f, size * 0.002f, size * 0.08f, 6, 1);
        glPopMatrix();
    }

    // Thorax
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f);
    glScalef(size * 0.08f, size * 0.15f, size * 0.08f); 
    glColor4f(0.5f, 0.4f, 0.3f, 0.7f);
    glutSolidSphere(1.0f, 12, 12);
    glPopMatrix();

    // Abdomen
    float segmentLength = size * 0.05f;
    float currentY = -size * 0.1f; 
    for (int seg = 0; seg < 8; ++seg) {
        glPushMatrix();
        glTranslatef(0.0f, currentY, 0.0f);
        glColor4f(0.6f + seg * 0.05f, 0.5f + seg * 0.05f, 0.4f, 0.7f); 
        gluCylinder(quadric, size * (0.04f - seg * 0.002f), size * (0.035f - seg * 0.002f), segmentLength, 8, 1);
        currentY -= segmentLength;
        glPopMatrix();
    }

    // Siphon
    glPushMatrix();
    glTranslatef(0.0f, currentY - size * 0.05f, 0.0f);
    glColor4f(0.3f, 0.2f, 0.1f, 0.8f); 
    gluCylinder(quadric, size * 0.01f, size * 0.005f, size * 0.1f, 6, 1);
    glPopMatrix();

    // Optional lateral hairs
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

void drawPitchedRoofHouse(bool isNight) {
    float w = 4.0; // half-width (x-axis)
    float h = 4.0; // half-height (y-axis)
    float d = 5.0; // half-depth (z-axis)

    // --- 1. Draw Main House Body (White) ---
    glColor3f(0.95f, 0.95f, 0.95f); // Light grey / white
    glBegin(GL_QUADS);
        // Front face
        glNormal3f(0.0, 0.0, 1.0);
        glVertex3f(-w, -h, d);
        glVertex3f(w, -h, d);
        glVertex3f(w, h, d);
        glVertex3f(-w, h, d);

        // Back face
        glNormal3f(0.0, 0.0, -1.0);
        glVertex3f(-w, -h, -d);
        glVertex3f(-w, h, -d);
        glVertex3f(w, h, -d);
        glVertex3f(w, -h, -d);

        // Right face
        glNormal3f(1.0, 0.0, 0.0);
        glVertex3f(w, -h, -d);
        glVertex3f(w, h, -d);
        glVertex3f(w, h, d);
        glVertex3f(w, -h, d);

        // Left face
        glNormal3f(-1.0, 0.0, 0.0);
        glVertex3f(-w, -h, -d);
        glVertex3f(-w, -h, d);
        glVertex3f(-w, h, d);
        glVertex3f(-w, h, -d);

        // Top face
        glNormal3f(0.0, 1.0, 0.0);
        glVertex3f(-w, h, d);
        glVertex3f(w, h, d);
        glVertex3f(w, h, -d);
        glVertex3f(-w, h, -d);

        // Bottom face
        glNormal3f(0.0, -1.0, 0.0);
        glVertex3f(-w, -h, d);
        glVertex3f(-w, -h, -d);
        glVertex3f(w, -h, -d);
        glVertex3f(w, -h, d);
    glEnd();

    // --- 2. Draw Roof (Red) ---
    glColor3f(0.8f, 0.0f, 0.0f); // Red
    float roofHeight = h + 3.0f; // 3 units taller than the house body
    float overhang = 1.0f;       // 1 unit overhang on all sides

    glBegin(GL_TRIANGLES);
        // Front gable
        glNormal3f(0.0, 0.0, 1.0);
        glVertex3f(-w - overhang, h, d + overhang);
        glVertex3f(w + overhang, h, d + overhang);
        glVertex3f(0, roofHeight, 0); // Apex of the roof

        // Back gable
        glNormal3f(0.0, 0.0, -1.0);
        glVertex3f(-w - overhang, h, -d - overhang);
        glVertex3f(0, roofHeight, 0); // Apex
        glVertex3f(w + overhang, h, -d - overhang);
    glEnd();

    glBegin(GL_QUADS);
        // Right roof panel
        glNormal3f(0.707, 0.707, 0.0); 
        glVertex3f(w + overhang, h, d + overhang);
        glVertex3f(w + overhang, h, -d - overhang);
        glVertex3f(0, roofHeight, 0); // Apex (shared vertex)
        glVertex3f(0, roofHeight, 0); // Apex (repeat for quad)

        // Left roof panel
        glNormal3f(-0.707, 0.707, 0.0);
        glVertex3f(-w - overhang, h, d + overhang);
        glVertex3f(0, roofHeight, 0); // Apex
        glVertex3f(0, roofHeight, 0); // Apex
        glVertex3f(-w - overhang, h, -d - overhang);
    glEnd();


    // --- 3. Draw Lit Windows (if night) ---
    if (environmentState == 1) { // Use environmentState instead of isNight
        glDisable(GL_LIGHTING); 
        glColor3f(1.0f, 0.9f, 0.2f); // Yellow light
        
        float winW = w * 0.3f; 
        float winH = h * 0.3f;
        float winX1 = -w * 0.7f;
        float winX2 = w * 0.4f; 
        float winY = -h * 0.2f;
        
        // Draw on Front Face (at z = d + 0.01)
        glBegin(GL_QUADS);
            glVertex3f(winX1, winY, d + 0.01f);
            glVertex3f(winX1 + winW, winY, d + 0.01f);
            glVertex3f(winX1 + winW, winY + winH, d + 0.01f);
            glVertex3f(winX1, winY + winH, d + 0.01f);

            glVertex3f(winX2, winY, d + 0.01f);
            glVertex3f(winX2 + winW, winY, d + 0.01f);
            glVertex3f(winX2 + winW, winY + winH, d + 0.01f);
            glVertex3f(winX2, winY + winH, d + 0.01f);
        glEnd();

        // Draw on Right Face (at x = w + 0.01)
        float winZ1 = -d * 0.5f;
        glBegin(GL_QUADS);
            glVertex3f(w + 0.01f, winY, winZ1);
            glVertex3f(w + 0.01f, winY, winZ1 + winW); 
            glVertex3f(w + 0.01f, winY + winH, winZ1 + winW);
            glVertex3f(w + 0.01f, winY + winH, winZ1);
        glEnd();
        
        // Re-enable lighting
        glEnable(GL_LIGHTING);
    }
}

void drawTree(float x, float y) {

    glColor3f(0.54f, 0.27f, 0.07f);

    glBegin(GL_QUADS);
        glVertex3f(x + 0.01f, y, -0.05f);        // slightly narrower trunk
        glVertex3f(x + 0.04f, y, -0.05f);
        glVertex3f(x + 0.04f, y + 0.32f, -0.05f); // slightly taller
        glVertex3f(x + 0.01f, y + 0.32f, -0.05f);
    glEnd();

    glColor3f(0.0f, 0.6f, 0.0f);

    // Main sphere
    glPushMatrix();
    glTranslatef(x + 0.025f, y + 0.40f, 0.02f);
    glutSolidSphere(0.12, 20, 20);
    glPopMatrix();

    // Left sphere
    glPushMatrix();
    glTranslatef(x + 0.005f, y + 0.37f, 0.02f);
    glutSolidSphere(0.10, 20, 20);
    glPopMatrix();

    // Right sphere
    glPushMatrix();
    glTranslatef(x + 0.045f, y + 0.37f, 0.02f);
    glutSolidSphere(0.10, 20, 20);
    glPopMatrix();

    // Back depth sphere
    glPushMatrix();
    glTranslatef(x + 0.025f, y + 0.37f, -0.05f);
    glutSolidSphere(0.10, 20, 20);
    glPopMatrix();

    // Front sphere
    glPushMatrix();
    glTranslatef(x + 0.025f, y + 0.37f, 0.09f);
    glutSolidSphere(0.10, 20, 20);
    glPopMatrix();
}

void drawSun(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f); 

    float radius = 0.12f;
    int segments = 32;

    // --- 1. Draw Sun Body (Radial Gradient) ---
    glBegin(GL_TRIANGLE_FAN);
        
        // Center Vertex: Bright Yellow/White (FF F5 31)
        glColor3f(1.0f, 0.96f, 0.20f); 
        glVertex3f(0.0f, 0.0f, 0.0f); 

        // Outer Vertices: Deep Orange/Red (FA F8 91 F)
        glColor3f(0.98f, 0.57f, 0.09f); 
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * 3.1415926f * float(i) / float(segments);
            float cosA = cosf(angle);
            float sinA = sinf(angle);
            
            glVertex3f(cosA * radius, sinA * radius, 0.01f); 
        }
    glEnd();

    // --- 2. Enhanced Draw Rays (40 Rays with Fade Effect) ---
    
    // Enable blending to allow the outer rays to fade (become slightly transparent)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

    int numRays = 40; // Draw more rays for a fuller glow
    float innerR = radius;
    float outerR = 0.35f; // Extend rays much further
    float rayWidth = 0.005f; // Thin width for a sharp look

    glBegin(GL_QUADS);
        for(int i = 0; i < numRays; ++i) {
            float angle = 2.0f * 3.1415926f * float(i) / float(numRays);
            
            // Inner color (Starting bright orange-yellow)
            glColor4f(1.0f, 0.65f, 0.25f, 1.0f); 
            
            // Inner quad vertices (Start at the sun's edge)
            glVertex3f(cosf(angle) * innerR, sinf(angle) * innerR, 0.0f);
            glVertex3f(cosf(angle + rayWidth) * innerR, sinf(angle + rayWidth) * innerR, 0.0f);

            // Outer color (Fading to red-orange and transparency)
            glColor4f(0.85f, 0.40f, 0.10f, 0.0f); // Fully transparent (Alpha=0.0)
            
            // Outer quad vertices (Extend far outwards)
            glVertex3f(cosf(angle + rayWidth) * outerR, sinf(angle + rayWidth) * outerR, 0.0f);
            glVertex3f(cosf(angle) * outerR, sinf(angle) * outerR, 0.0f);
        }
    glEnd();
    
    glDisable(GL_BLEND); // Turn blending off after drawing rays

    glPopMatrix();
} 

void drawMoon(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f); 

    srand(12345); // Fixed seed for consistent crater placement

    // --- Enable Blending for Transparency (Essential for Glow) ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- Enable Lighting for More Realistic Shading ---
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat lightPos[] = {1.0f, 1.0f, 1.0f, 0.0f}; // Directional light from upper right
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    GLfloat ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    GLfloat diffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glEnable(GL_COLOR_MATERIAL); // Allow colors to interact with lighting

    // --- 1. Draw the Main Lunar Body (Enhanced Blueish Tone with Better Illumination) ---
    glColor3f(0.53f, 0.81f, 0.92f); // #87ceeb - Sky blue with a cool tint
    float moonRadius = 0.12f; 
    glutSolidSphere(moonRadius, 128, 128); // Even higher resolution for ultra-smooth surface
    
    // --- 2. Draw 'Craters' and Maria (Darker blue-gray with more variety and rims for realism) ---
    glColor3f(0.25f, 0.41f, 0.55f); // #40698c - Darker blue-gray
    int numCraters = 30; // Even more craters for intricate detail
    
    for (int i = 0; i < numCraters; ++i) {
        glPushMatrix();
        float r = ((float)(rand() % 100) / 100.0f) * 0.09f + 0.015f; // Wider spread
        float theta = ((float)(rand() % 100) / 100.0f) * 2.0f * 3.1415926f;
        float craterX = r * cosf(theta);
        float craterY = r * sinf(theta);
        float craterSize = 0.008f + ((float)(rand() % 60) / 100.0f) * 0.025f; // More varied sizes
        glTranslatef(craterX, craterY, -0.02f); // Deeper for pronounced shadows
        glutSolidSphere(craterSize, 16, 16); // Higher res
        
        // Add simple rim for craters (lighter ring)
        glColor3f(0.68f, 0.85f, 0.90f); // Lighter blue for rim
        glTranslatef(0.0f, 0.0f, 0.015f); // Raise rim slightly
        // Simulate rim with a flattened sphere or torus, but for simplicity, use a larger thin sphere
        glScalef(1.2f, 1.2f, 0.1f); // Flatten for ring-like appearance
        glutSolidSphere(craterSize, 16, 16);
        glPopMatrix();
    }

    // --- Add Subtle Highlands (Lighter blueish areas with more count for texture) ---
    glColor3f(0.68f, 0.85f, 0.90f); // #add8e6 - Lighter blue
    int numHighlands = 15; // More highlands for balanced texture
    for (int i = 0; i < numHighlands; ++i) {
        glPushMatrix();
        float r = ((float)(rand() % 100) / 100.0f) * 0.07f + 0.03f;
        float theta = ((float)(rand() % 100) / 100.0f) * 2.0f * 3.1415926f;
        float highX = r * cosf(theta);
        float highY = r * sinf(theta);
        float highSize = 0.01f + ((float)(rand() % 40) / 100.0f) * 0.02f;
        glTranslatef(highX, highY, 0.008f); // Slightly more raised
        glutSolidSphere(highSize, 12, 12);
        glPopMatrix();
    }

    // --- 3. Enhanced GLOW Effect (More Layers with Vibrant Blue Gradient) ---
    glDisable(GL_LIGHTING); // Disable lighting for glow to keep it flat and ethereal
    int numGlowLayers = 12; // More layers for an even softer, radiant glow
    for (int i = 0; i < numGlowLayers; ++i) {
        float glowRadius = moonRadius + (i * 0.012f); // Finer spacing for seamless blend
        
        // Smoother alpha fade: Stronger inner glow, gradual fade
        float alpha = 0.3f - (i * 0.025f);
        if (alpha < 0.0f) alpha = 0.0f;

        // Refined color gradient: Intense blue-white core to soft cyan-blue halo
        float red = 0.55f - (i * 0.04f);
        float green = 0.82f - (i * 0.02f);
        float blue = 0.98f + (i * 0.005f); // Boost blue subtly
        glColor4f(red, green, blue, alpha);

        glutSolidSphere(glowRadius, 48, 48); // Higher res for pristine glow
    }

    glDisable(GL_BLEND); // Disable blending after drawing
    glDisable(GL_COLOR_MATERIAL);
    glPopMatrix();
}
void drawGrass(float x, float y) {
    
    int seed = (int)(x * 1234.0f + y * 5678.0f);
    srand(seed); 
    // Draw a clump of 3-5 blades per coordinate
    int bladeCount = 3 + (rand() % 3); 

    for (int i = 0; i < bladeCount; i++) {
        // --- SCALE & VARIATION ---
        // Increased Scale: Height is now 0.15 to 0.25 (was 0.08)
        float height = 0.15f + (rand() % 15) * 0.01f;
        
        // Width relative to height
        float width = 0.02f + (rand() % 5) * 0.002f; 

        // "Lean" - how much the grass curves to the left or right
        // This makes it look organic, not like spikes.
        float lean = ((rand() % 20) - 10) * 0.01f; 

        // Slight random offset for position so they aren't in a perfect line
        float offsetX = ((rand() % 10) - 5) * 0.005f;

        // --- DRAWING THE BLADE ---
        glBegin(GL_TRIANGLES);

            // Vertex 1: Bottom Left (Darker Green - Shadow)
            glColor3f(0.0f, 0.35f, 0.0f); 
            glVertex3f(x + offsetX - width, y, -0.05f);

            // Vertex 2: The Tip (Lighter Green - Sunlight)
            // Note: We add 'lean' to the X coordinate to curve it
            float greenVar = 0.6f + (rand() % 40) * 0.01f; // Random bright green
            glColor3f(0.1f, greenVar, 0.1f); 
            glVertex3f(x + offsetX + lean, y + height, -0.05f);

            // Vertex 3: Bottom Right (Darker Green - Shadow)
            glColor3f(0.0f, 0.30f, 0.0f); 
            glVertex3f(x + offsetX + width, y, -0.05f);

        glEnd();
        
        // Optional: Draw a thin line down the center to make it look sharp
        glBegin(GL_LINES);
            glColor3f(0.0f, 0.2f, 0.0f); // Very dark spine
            glVertex3f(x + offsetX, y, -0.04f);
            glColor3f(0.2f, 0.8f, 0.2f);
            glVertex3f(x + offsetX + lean, y + height, -0.04f);
        glEnd();
    }
}

void drawPond() {
    glPushMatrix();
    // Keep your original position
    glTranslatef(0.0f, 0.0f, -0.04f);

    // 1. Enable Blending for water transparency effects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const int segments = 72;
    const float PI = 3.1415926f;

    // --- PART A: The Water (Radial Gradient for Depth) ---
    // We use GL_TRIANGLE_FAN to create a gradient from center to edge
    glBegin(GL_TRIANGLE_FAN);
        // Center vertex: Lighter blue and slightly transparent (shallow water)
        glColor4f(0.2f, 0.6f, 1.0f, 0.9f); 
        glVertex3f(pondX, pondY, 0.0f); 

        // Outer vertices: Darker blue (deep water feel)
        glColor4f(0.05f, 0.2f, 0.6f, 0.95f); 
        for (int i = 0; i <= segments; ++i) {
            float theta = 2.0f * PI * float(i) / float(segments);
            float dx = cosf(theta) * pondRadiusX;
            float dy = sinf(theta) * pondRadiusY;
            glVertex3f(pondX + dx, pondY + dy, 0.0f);
        }
    glEnd();

    // --- PART B: The Shoreline (Border) ---
    // A thick brown/dark line makes the water pop against the grass
    glLineWidth(2.0f);
    glColor3f(0.35f, 0.25f, 0.15f); // Dark Sandy/Muddy color
    glBegin(GL_LINE_LOOP);
        for (int i = 0; i < segments; ++i) {
            float theta = 2.0f * PI * float(i) / float(segments);
            glVertex3f(pondX + cosf(theta) * pondRadiusX, 
                       pondY + sinf(theta) * pondRadiusY, 0.01f); // Slightly higher z
        }
    glEnd();

    // --- PART C: Internal Ripples (Detail) ---
    // Subtle lighter rings to look like movement
    glLineWidth(1.0f);
    glColor4f(0.5f, 0.8f, 1.0f, 0.4f); // Very light blue, transparent
    glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 36; ++i) {
            float theta = 2.0f * PI * float(i) / 36;
            // Draw at 80% size (0.8f)
            glVertex3f(pondX + cosf(theta) * pondRadiusX * 0.8f, 
                       pondY + sinf(theta) * pondRadiusY * 0.8f, 0.01f);
        }
    glEnd();

    glDisable(GL_BLEND); // Turn off blend so it doesn't affect other objects
    glPopMatrix();
}

void drawWaterBowl() {
    if (!waterBowlVisible) return;

    glPushMatrix();
    // Keep original position
    glTranslatef(0.0f, 0.0f, -0.04f); 

    // Enable transparency for water/glass effects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const int segments = 32;
    const float PI = 3.1415926f;

    // --- PART 1: The Bowl (Clay Material) ---
    // We use a gradient: Darker at edges, lighter in center to simulate a curved bottom
    glBegin(GL_TRIANGLE_FAN);
        glColor3f(0.55f, 0.27f, 0.07f); // Center: Lighter Clay
        glVertex2f(waterBowlX, waterBowlY); 
        
        glColor3f(0.35f, 0.15f, 0.05f); // Edge: Darker Clay (shadows)
        for (int i = 0; i <= segments; ++i) {
            float theta = 2.0f * PI * float(i) / float(segments);
            glVertex2f(waterBowlX + cosf(theta) * waterBowlRadius, 
                       waterBowlY + sinf(theta) * waterBowlRadius);
        }
    glEnd();

    // --- PART 2: The Water Surface ---
    // Radial gradient: Light blue center, dark blue edges (looks deeper)
    glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.6f, 0.85f, 1.0f, 0.8f); // Center: Bright/Reflective
        glVertex2f(waterBowlX, waterBowlY);

        glColor4f(0.1f, 0.4f, 0.8f, 0.9f);  // Edge: Deep water
        for (int i = 0; i <= segments; ++i) {
            float theta = 2.0f * PI * float(i) / float(segments);
            // Using 0.85f leaves a visible "rim" of the bowl showing
            glVertex2f(waterBowlX + cosf(theta) * waterBowlRadius * 0.85f, 
                       waterBowlY + sinf(theta) * waterBowlRadius * 0.85f);
        }
    glEnd();

    // --- PART 3: The Reflection (The "Shine") ---
    // A small white oval near the top-left makes it look glossy
    glBegin(GL_POLYGON);
        glColor4f(1.0f, 1.0f, 1.0f, 0.4f); // Semi-transparent white
        for (int i = 0; i <= 20; ++i) {
            float theta = 2.0f * PI * float(i) / 20.0f;
            // Positioned slightly up and left (-0.3, +0.3)
            float shineX = waterBowlX - (waterBowlRadius * 0.3f);
            float shineY = waterBowlY + (waterBowlRadius * 0.3f);
            // Small radius (0.15)
            glVertex2f(shineX + cosf(theta) * waterBowlRadius * 0.15f, 
                       shineY + sinf(theta) * waterBowlRadius * 0.10f);
        }
    glEnd();

    glDisable(GL_BLEND);
    glPopMatrix();
}

// --- NEW 3D RAIN FUNCTION ---
void drawRain() {

    if (rainActive || environmentState == 2) {

        float alpha = (environmentState == 2 && !rainActive) ? 0.15f : 0.35f;
        int numDrops = (environmentState == 2 && !rainActive) ? 120 : 300;

        glLineWidth(1.8f);

        const float topY = 1.2f;     // highest Y
        const float bottomY = -1.2f; // lowest Y
        const float dropSpeed = 0.06f;
        const float dropLength = 0.18f;

        for (int i = 0; i < numDrops; ++i) {

            srand(i * 17);

            float x = randFloat(-1.5f, 1.5f);

            // Random initial Y so each drop starts at different place
            float baseY = randFloat(bottomY, topY);

            float speedOffset = randFloat(0.7f, 1.3f);

            // y position moves from topY → bottomY
            float y = topY - fmod(frameCounter * dropSpeed * speedOffset + (topY - baseY),
                                  (topY - bottomY));

            // diagonal (wind)
            float dx = randFloat(0.01f, 0.04f);

            // slight brightness variation
            float brightness = randFloat(0.7f, 1.0f);
            glColor4f(0.75f * brightness, 0.80f * brightness, 1.0f * brightness, alpha);

            glBegin(GL_LINES);
            glVertex3f(x, y, 0.0f);                // top of streak
            glVertex3f(x + dx, y - dropLength, 0.0f); // falling down
            glEnd();
        }

        glLineWidth(1.0f);
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
    const float areaWidth = 0.98f - marginLeft; 
    const float maxBarWidth = 0.12f;           
    float barWidth = fminf(maxBarWidth, areaWidth / (float)bars);
    float gap = barWidth * 0.15f;
    float usableBar = barWidth - gap;

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

        glColor3f(0.16f, 0.6f, 0.16f);
        glBegin(GL_QUADS);
          glVertex2f(x0, y0);
          glVertex2f(x1, y0);
          glVertex2f(x1, y1);
          glVertex2f(x0, y1);
        glEnd();

        char valbuf[16];
        snprintf(valbuf, sizeof(valbuf), "%d", killsPerMinute[i]);
        displayText(valbuf, x0, y1 + 0.01f, GLUT_BITMAP_HELVETICA_12);
    }

    glColor3f(0.0f, 0.0f, 0.0f);
    displayText("Kills per Minute", 0.50f, -0.50f, GLUT_BITMAP_HELVETICA_12);

    glDisable(GL_BLEND);
}

void updateHistogram(int killedThisFrame) {
    // frameCounter++; // <-- REMOVED: It is now in timerFunc
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
                    Larva larva = {mosquitoes[i].x + randFloat(-0.02f, 0.02f), mosquitoes[i].y + randFloat(-0.02f, 0.02f), 0.04f, 0, true}; // Give larva a size
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
    } else if (rand() % 1000 < 2 && !rainActive && environmentState != 2) { // No random rain in fog
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
    bool boost = waterBowlVisible || totalAlive > 5 || rainActive || (environmentState != 0);
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
void displayText(const char* text, float x, float y, void* font) {
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
    displayText(buf, -0.95f, 0.94f, GLUT_BITMAP_HELVETICA_18);
    snprintf(buf, sizeof(buf), "Killed: %d", totalKilled);
    displayText(buf, -0.95f, 0.89f, GLUT_BITMAP_HELVETICA_18);
    snprintf(buf, sizeof(buf), "Spawn Rate: %s", (currentSpawnInterval == spawnIntervalHigh ? "High" : "Normal"));
    displayText(buf, -0.7f, 0.94f, GLUT_BITMAP_HELVETICA_18);
    snprintf(buf, sizeof(buf), "Spray Charges: %d/%d", sprayCharges, maxSprayCharges);
    displayText(buf, -0.7f, 0.89f, GLUT_BITMAP_HELVETICA_18);
}
void displayInstructions() {
    const char* lines[] = {
        "Controls:",
        "Left-Click: Spray (if charges > 0)",
        "D: Toggle Day/Night/Fog",
        "S: Random Spray",
        "R: Toggle Water Bowl",
        "T: Trigger Rain Event",
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

// --- COMPLETE AND CORRECT display() FUNCTION ---
void display() {
    // 1. Set clear color based on environment state
   if (environmentState == 1) { // Night
        // New color: A darker, more realistic navy blue
        glClearColor(0.02f, 0.04f, 0.10f, 1.0f);

        // Add realistic night view: Draw a starry sky with twinkling stars
        glDisable(GL_LIGHTING); // Flat colors for stars
        srand(54321); // Fixed seed for consistent star placement

        glPointSize(1.5f); // Slightly larger points for visibility
        glBegin(GL_POINTS);
        
        int numStars = 500; // Number of stars for a dense but not overwhelming sky
        for (int i = 0; i < numStars; ++i) {
            // Random positions across the view (assuming normalized device coordinates -1 to 1)
            float starX = ((float)(rand() % 2000) / 1000.0f) - 1.0f;
            float starY = ((float)(rand() % 2000) / 1000.0f) - 1.0f;
            float starZ = -1.0f; // Place stars far back in 3D space for depth
            
            // Vary star brightness and slight color tint for realism (white to yellowish)
            float brightness = 0.7f + ((float)(rand() % 30) / 100.0f); // 0.7 to 1.0
            float tint = (rand() % 2 == 0) ? 1.0f : 0.95f; // Subtle yellow tint for some stars
            glColor3f(brightness, brightness * tint, brightness * 0.95f); // Cool white to warm
            
            glVertex3f(starX * 2.0f, starY * 2.0f, starZ); // Scale to cover wider area
        }
        
        // Add a few brighter stars (e.g., like Sirius or Venus) for highlights
        int numBrightStars = 10;
        for (int i = 0; i < numBrightStars; ++i) {
            float starX = ((float)(rand() % 2000) / 1000.0f) - 1.0f;
            float starY = ((float)(rand() % 2000) / 1000.0f) - 1.0f;
            float starZ = -1.0f;
            glPointSize(3.0f); // Larger size for bright stars
            glColor3f(1.0f, 1.0f, 0.9f); // Bright yellowish-white
            glVertex3f(starX * 2.0f, starY * 2.0f, starZ);
        }
        
        glEnd();
        glPointSize(1.0f); // Reset point size
        glEnable(GL_LIGHTING); // Re-enable lighting if needed for other elements
    }

     else if (environmentState == 2) { // Fog
        // Changed color: Use the dark navy blue from night for a foggy night ambiance
        glClearColor(0.02f, 0.04f, 0.10f, 1.0f);

        // Add realistic 3D fog view: Use layered alpha quads with varying alpha and slight color gradient for a more beautiful, dynamic effect
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_LIGHTING); // Flat color for fog layers
        glDepthMask(GL_FALSE); // Disable depth writing to allow proper blending over scene geometry

        int numLayers = 40; // More layers for smoother, more immersive and beautiful transitions
        float heightStep = 0.25f; // Finer steps for a denser, more fluid appearance
        float minHeight = 0.0f; // Start at ground level
        float size = 200.0f; // Even larger size to fully envelop the scene

        for (int i = 0; i < numLayers; ++i) {
            float y = minHeight + i * heightStep;
            
            // Vary alpha for a more natural, uneven fog density (higher at bottom, fading upwards)
            float alpha = 0.05f - (i * 0.001f); // Start denser, fade gradually for beauty
            if (alpha < 0.01f) alpha = 0.01f; // Minimum alpha to maintain subtle visibility
            
            // Slight color gradient: Deeper navy at bottom, lighter bluish tint upwards for atmospheric depth
            float red = 0.02f - (i * 0.0005f);
            float green = 0.04f - (i * 0.0005f);
            float blue = 0.10f + (i * 0.001f); // Shift to a slightly brighter blue upwards
            glColor4f(red, green, blue, alpha);

            glPushMatrix();
            glTranslatef(0.0f, y, 0.0f); // Stack layers horizontally along y-axis (assuming y-up)
            
            // Add slight random offset for a more organic, billowing fog effect
            float offsetX = sinf(y * 0.5f) * 5.0f; // Gentle wave motion
            float offsetZ = cosf(y * 0.3f) * 5.0f;
            glTranslatef(offsetX, 0.0f, offsetZ);
            
            glBegin(GL_QUADS);
            glVertex3f(-size, 0.0f, -size);
            glVertex3f(size, 0.0f, -size);
            glVertex3f(size, 0.0f, size);
            glVertex3f(-size, 0.0f, size);
            glEnd();
            glPopMatrix();
        }

        glDepthMask(GL_TRUE); // Re-enable depth writing
        glEnable(GL_LIGHTING); // Re-enable if needed for the rest of the scene
        glDisable(GL_BLEND);
    } else { // Day
        // New color: A richer, brighter sky blue
        glClearColor(0.53f, 0.78f, 0.97f, 1.0f);
    }

    // 2. Clear color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 3. Set up 3D camera
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camX, camY, camZ,   // Camera position (looking from z=2)
              lookX, lookY, lookZ,   // Look at origin
              0.0f, 1.0f, 0.0f);  // Up vector

    // --- 3D SCENE ---

    // 4. Enable/Disable Fog
    if (environmentState == 2) { // Fog
        // Replace built-in fog with layered alpha quads for more realistic 3D volumetric fog effect
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_LIGHTING); // Flat color for fog layers
        glDepthMask(GL_FALSE); // Disable depth writing to allow proper blending over scene geometry

        // Fog color MUST match the background color, with low alpha for layering
        GLfloat fogColor[4] = {0.75f, 0.75f, 0.75f, 0.04f}; // Adjusted alpha for subtlety (tune based on numLayers)
        glColor4fv(fogColor);

        int numLayers = 25; // More layers for smoother, more realistic fog (but impacts performance)
        float heightStep = 0.4f; // Smaller steps for denser fog appearance
        float minHeight = 0.0f; // Start at ground level
        float size = 100.0f; // Large size to cover the scene; adjust based on scene scale

        for (int i = 0; i < numLayers; ++i) {
            float y = minHeight + i * heightStep;
            glPushMatrix();
            glTranslatef(0.0f, y, 0.0f); // Stack layers horizontally along y-axis (assuming y-up)
            glBegin(GL_QUADS);
            glVertex3f(-size, 0.0f, -size);
            glVertex3f(size, 0.0f, -size);
            glVertex3f(size, 0.0f, size);
            glVertex3f(-size, 0.0f, size);
            glEnd();
            glPopMatrix();
        }

        glDepthMask(GL_TRUE); // Re-enable depth writing
        glEnable(GL_LIGHTING); // Re-enable if needed for the rest of the scene
        glDisable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }


    // 5. Draw ground plane (at z=-0.1, and larger)
    glColor3f(0.3f, 0.6f, 0.2f); // Green ground
    glBegin(GL_QUADS);
    glVertex3f(-1.5f, -1.2f, -0.1f);
    glVertex3f( 1.5f, -1.2f, -0.1f);
    glVertex3f( 1.5f,  1.2f, -0.1f);
    glVertex3f(-1.5f,  1.2f, -0.1f);
    glEnd();

    // 6. Draw Sun/Moon (but not in fog)
    if (environmentState == 1) { // Night
    drawMoon(0.8f, 0.8f);
} else if (environmentState == 0) { // Day
    drawSun(0.8f, 0.8f);
}

    // 7. Draw other scene objects (clouds, grass)
    glColor3f(1,1,1);
    drawCircle(-0.8f, 0.75f, 0.08f, 0.04f, 24);
    drawCircle(-0.55f, 0.8f, 0.07f, 0.035f, 24);
    drawCircle(0.3f, 0.7f, 0.09f, 0.045f, 24);

    for (int i = 0; i < 20; ++i) drawGrass(randFloat(-1.0f, 1.0f), -0.95f);


    // --- 8. Draw the new 3D Houses ---
    
    GLfloat light_pos[] = { 1.0f, 5.0f, 5.0f, 1.0f };
    bool lightWindows = (environmentState == 1); // Windows light up only at night
    
    // --- House 1 ---
    glPushMatrix();
    glTranslatef(-0.5f, -0.3f, -0.099f);
    float scale1 = 0.3f / 8.0f; 
    glScalef(scale1, scale1, scale1);
    glRotatef(g_rotateX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_rotateY, 0.0f, 1.0f, 0.0f);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glEnable(GL_COLOR_MATERIAL);
    drawPitchedRoofHouse(lightWindows);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_COLOR_MATERIAL);
    glPopMatrix();


    // --- House 2 ---
    glPushMatrix();
    glTranslatef(0.0f, -0.35f, -0.099f);
    float scale2 = 0.4f / 8.0f;
    glScalef(scale2, scale2, scale2);
    glRotatef(g_rotateX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_rotateY, 0.0f, 1.0f, 0.0f);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glEnable(GL_COLOR_MATERIAL);
    drawPitchedRoofHouse(lightWindows);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_COLOR_MATERIAL);
    glPopMatrix();


    // --- House 3 ---
    glPushMatrix();
    glTranslatef(0.5f, -0.3f, -0.099f);
    float scale3 = 0.25f / 8.0f;
    glScalef(scale3, scale3, scale3);
    glRotatef(g_rotateX, 1.0f, 0.0f, 0.0f);
    glRotatef(g_rotateY, 0.0f, 1.0f, 0.0f);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glEnable(GL_COLOR_MATERIAL);
    drawPitchedRoofHouse(lightWindows);
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_COLOR_MATERIAL);
    glPopMatrix();


    // 9. Draw Trees (now 3D)
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glEnable(GL_COLOR_MATERIAL);
    
    drawTree(-0.7f, -0.4f);
    drawTree(0.7f, -0.4f);
    drawTree(0.2f, -0.75f);
    
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_COLOR_MATERIAL);


    // These will now draw correctly on top of the z=-0.1 ground
    drawPond();
    drawWaterBowl();

    // --- 10. Draw 3D Dynamic Objects (larvae, mosquitoes, rain, spray) ---
    // ALL TRANSPARENT OBJECTS GO HERE
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Larvae
    for (size_t i = 0; i < larvae.size(); ++i) drawLarva(larvae[i].x, larvae[i].y, larvae[i].size);

    // Mosquitoes
    for (int i = 0; i < NUM_MOSQUITOES; ++i) {
      if (mosquitoes[i].alive) {
          // Shadow
          glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
          drawCircle(mosquitoes[i].x, mosquitoes[i].y, mosquitoes[i].size * 0.2f, mosquitoes[i].size * 0.1f);
          
          // Mosquito
          float zPos = 0.05f + sinf((float)frameCounter * 0.1f + i) * 0.02f;
          drawMosquito(mosquitoes[i].x, mosquitoes[i].y, zPos, mosquitoes[i].size, 0.0f, 0.0f, 0.0f);
      }
    }

    // Spray
    if (spraying) {
        glColor4f(0.08f, 0.5f, 1.0f, 0.45f);
        drawCircle(sprayX, sprayY, sprayRadius, sprayRadius, 36);
    }
    
    // 3D Rain (This is the correct location for it)
    drawRain();

    // Now we are done with transparent objects
    glDisable(GL_BLEND); 


    // --- 2D UI OVERLAY ---
    glDisable(GL_FOG); // Turn off fog for the UI
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); 
    glLoadIdentity();
    gluOrtho2D(-1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST); // UI should not be depth tested

    // All UI functions go here
    displayText("Dengue Awareness Simulation", -0.95f, 0.98f, GLUT_BITMAP_TIMES_ROMAN_24);
    displayUI();
    displayInstructions();
    displayPopup();
    drawHistogram();

    // 13. Restore 3D matrices and state
    glEnable(GL_DEPTH_TEST); // Re-enable depth test
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // 14. Swap buffers
    glutSwapBuffers();
}

// ---------------- Input & Timer ----------------
void timerFunc(int value) {
    frameCounter++; // This makes the rain animate
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
    } else if (key == 'd' || key == 'D') {
        environmentState = (environmentState + 1) % ENV_STATES;
        if (environmentState == 0) {
            snprintf(popupText, sizeof(popupText), "Switched to Day");
        } else if (environmentState == 1) {
            snprintf(popupText, sizeof(popupText), "Switched to Night");
        } else {
            snprintf(popupText, sizeof(popupText), "Switched to Fog");
        }
        popupTimer = popupDuration;
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
        waterBowlY = 1.0f - (2.0f * my / winH); // Corrected from 'H'
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
    glEnable(GL_DEPTH_TEST); 
    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)WINDOW_W / (float)WINDOW_H, 0.1f, 100.0f);

    glMatrixMode(GL_MODELVIEW); 
    glLoadIdentity();

    glShadeModel(GL_SMOOTH);

    initializeMosquitoes();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); 
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