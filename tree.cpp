#include "tree.h"
#include "drawing.h"
#include <GLFW/glfw3.h>
#include <cmath>

// These are now part of the tree implementation
void drawCylinder(float x, float y, float z, float radius, float height, int segments);
void drawSphere(float x, float y, float z, float radius, int segments);

void drawTree(float x, float y, float z) {
    // Draw the trunk
    glColor3f(0.5f, 0.35f, 0.05f); // Brown
    drawCylinder(x, y, z, 0.2f, 2.0f, 16);

    // Draw the leaves (a few spheres)
    glColor3f(0.0f, 0.6f, 0.2f); // Dark Green
    drawSphere(x, y + 1.5f, z, 0.8f, 16);
    drawSphere(x + 0.5f, y + 1.2f, z + 0.5f, 0.6f, 16);
    drawSphere(x - 0.5f, y + 1.2f, z - 0.5f, 0.6f, 16);
}

void drawCylinder(float x, float y, float z, float radius, float height, int segments) {
    float halfHeight = height / 2.0f;

    // Draw the cylinder body
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * 3.1415926f / segments;
        float dx = cos(angle) * radius;
        float dz = sin(angle) * radius;
        glNormal3f(dx, 0, dz); // Normal for lighting
        glVertex3f(x + dx, y - halfHeight, z + dz);
        glVertex3f(x + dx, y + halfHeight, z + dz);
    }
    glEnd();

    // Draw the top cap
    glBegin(GL_POLYGON);
    glNormal3f(0, 1, 0);
    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * 3.1415926f / segments;
        float dx = cos(angle) * radius;
        float dz = sin(angle) * radius;
        glVertex3f(x + dx, y + halfHeight, z + dz);
    }
    glEnd();

    // Draw the bottom cap
    glBegin(GL_POLYGON);
    glNormal3f(0, -1, 0);
    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * 3.1415926f / segments;
        float dx = cos(angle) * radius;
        float dz = sin(angle) * radius;
        glVertex3f(x + dx, y - halfHeight, z + dz);
    }
    glEnd();
}

void drawSphere(float x, float y, float z, float radius, int segments) {
    for (int lat = 0; lat < segments; ++lat) {
        float latAngle1 = 3.1415926f * (-0.5f + (float)lat / segments);
        float z0 = sin(latAngle1) * radius;
        float zr0 = cos(latAngle1) * radius;

        float latAngle2 = 3.1415926f * (-0.5f + (float)(lat + 1) / segments);
        float z1 = sin(latAngle2) * radius;
        float zr1 = cos(latAngle2) * radius;

        glBegin(GL_QUAD_STRIP);
        for (int lon = 0; lon <= segments; ++lon) {
            float lonAngle = 2 * 3.1415926f * (float)lon / segments;
            float x_ = cos(lonAngle);
            float y_ = sin(lonAngle);

            // Normals for lighting
            glNormal3f(x_ * zr1, y_ * zr1, z1);
            glVertex3f(x + x_ * zr1, y + y_ * zr1, z + z1);

            glNormal3f(x_ * zr0, y_ * zr0, z0);
            glVertex3f(x + x_ * zr0, y + y_ * zr0, z + z0);
        }
        glEnd();
    }
}
