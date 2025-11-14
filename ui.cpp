#include "ui.h"
#include "config.h"
#include <GLFW/glfw3.h>
#include <cstring>
#include <cstdio>


// ---------------- Display ----------------

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
}
void displayInstructions() {
    // Text rendering removed as GLFW does not have a built-in font renderer.
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
    }
}
