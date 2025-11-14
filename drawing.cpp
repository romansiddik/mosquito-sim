#include "drawing.h"
#include "tree.h"
#include <GLFW/glfw3.h>

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawTree(0.0f, -0.5f, 0.0f);
}
