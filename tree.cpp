#include "tree.h"
#include <GLFW/glfw3.h>
#include <cmath>

// Private helper functions for drawing tree components
void drawCylinder(float x, float y, float z, float radius, float height, int segments);
void drawSphere(float x, float y, float z, float radius, int segments);
void drawLeaf(float x, float y, float z, float size);

// Main function to draw a tree of a specific type
void drawTree(TreeType type, float x, float y, float z) {
    switch (type) {
        case PINE:
            // Trunk
            glColor3f(0.4f, 0.25f, 0.15f);
            drawCylinder(x, y, z, 0.25f, 2.5f, 16);
            // Tiered leaves
            glColor3f(0.0f, 0.4f, 0.1f);
            for (int i = 0; i < 4; ++i) {
                drawSphere(x, y + 1.2f + i * 0.5f, z, 1.0f - i * 0.2f, 16 - i * 2);
            }
            break;
        case OAK:
            // Trunk and major branches
            glColor3f(0.6f, 0.4f, 0.2f);
            drawCylinder(x, y, z, 0.3f, 2.0f, 16);
            drawCylinder(x + 0.3f, y + 1.5f, z, 0.15f, 1.0f, 8);
            drawCylinder(x - 0.2f, y + 1.8f, z + 0.1f, 0.1f, 0.8f, 8);
            // Dense, clustered leaves
            glColor3f(0.1f, 0.5f, 0.1f);
            for (int i = 0; i < 15; ++i) {
                float dx = (rand() / (float)RAND_MAX - 0.5f) * 2.5f;
                float dy = (rand() / (float)RAND_MAX) * 1.5f;
                float dz = (rand() / (float)RAND_MAX - 0.5f) * 2.5f;
                drawSphere(x + dx, y + 1.5f + dy, z + dz, 0.5f, 12);
            }
            break;
        case WILLOW:
            // Leaning trunk
            glColor3f(0.5f, 0.45f, 0.3f);
            drawCylinder(x, y, z, 0.2f, 2.8f, 12);
            // Drooping leaves
            glColor3f(0.3f, 0.6f, 0.2f);
            for (int i = 0; i < 20; ++i) {
                float angle = (i / 20.0f) * 2.0f * 3.14159f;
                float dx = cos(angle) * (0.5f + (i / 20.0f) * 1.5f);
                float dz = sin(angle) * (0.5f + (i / 20.0f) * 1.5f);
                float dy = - (rand() / (float)RAND_MAX) * 2.0f;
                drawLeaf(x + dx, y + 2.5f + dy, z + dz, 0.15f);
            }
            break;
    }
}

// --- Drawing Primitives ---

void drawCylinder(float x, float y, float z, float radius, float height, int segments) {
    float halfHeight = height / 2.0f;
    glPushMatrix();
    glTranslatef(x, y, z);
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * 3.1415926f / segments;
        float dx = cos(angle) * radius;
        float dz = sin(angle) * radius;
        glNormal3f(dx, 0, dz);
        glVertex3f(dx, -halfHeight, dz);
        glVertex3f(dx, halfHeight, dz);
    }
    glEnd();
    glPopMatrix();
}

void drawSphere(float x, float y, float z, float radius, int segments) {
    glPushMatrix();
    glTranslatef(x, y, z);
    // Simplified sphere for foliage
    for (int lat = 0; lat < segments; ++lat) {
        float lat1 = 3.14159f * (-0.5f + (float)lat / segments);
        float z0 = sin(lat1) * radius, zr0 = cos(lat1) * radius;
        float lat2 = 3.14159f * (-0.5f + (float)(lat + 1) / segments);
        float z1 = sin(lat2) * radius, zr1 = cos(lat2) * radius;
        glBegin(GL_QUAD_STRIP);
        for (int lon = 0; lon <= segments; ++lon) {
            float lng = 2 * 3.14159f * (float)lon / segments;
            float x_ = cos(lng), y_ = sin(lng);
            glNormal3f(x_ * zr1, y_ * zr1, z1);
            glVertex3f(x_ * zr1, y_ * zr1, z1);
            glNormal3f(x_ * zr0, y_ * zr0, z0);
            glVertex3f(x_ * zr0, y_ * zr0, z0);
        }
        glEnd();
    }
    glPopMatrix();
}

void drawLeaf(float x, float y, float z, float size) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(size, size, size);
    glBegin(GL_TRIANGLES);
    // A simple, slightly randomized triangle to represent a leaf
    glNormal3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.2f, 0.0f);
    glVertex3f(0.2f, 1.0f, 0.0f);
    glEnd();
    glPopMatrix();
}
