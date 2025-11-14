#include "drawing.h"
#include "tree.h"
#include <GLFW/glfw3.h>

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw a small grove of different trees
    drawTree(OAK, 0.0f, -0.5f, 0.0f);      // A central oak tree
    drawTree(PINE, -3.0f, -0.5f, -2.0f);   // A pine tree to the left
    drawTree(WILLOW, 2.5f, -0.5f, -1.0f); // A willow tree to the right
}
