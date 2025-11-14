#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "drawing.h"
#include "config.h"

// A simple key callback to exit on pressing ESC
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void initGL() {
    // Set a sky-blue background
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

    // Enable 3D-specific features
    glEnable(GL_DEPTH_TEST); // Ensures objects in front obscure objects behind
    glEnable(GL_LIGHTING);   // Enables lighting calculations
    glEnable(GL_LIGHT0);     // Enables the first light source
    glEnable(GL_COLOR_MATERIAL); // Allows glColor to set material properties
    glEnable(GL_NORMALIZE);  // Keeps surface normals unit length

    // Define properties for the light source
    GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat light_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 0.0f }; // Directional light from above-right

    // Apply the light properties
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    // Tell OpenGL how to handle material colors
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

int main(void) {
    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(WINDOW_W, WINDOW_H, "3D Tree Model", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    initGL();

    // Set the input callback
    glfwSetKeyCallback(window, key_callback);

    while (!glfwWindowShouldClose(window))
    {
        // Set up the projection matrix (the lens of the camera)
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_W / (float)WINDOW_H, 0.1f, 100.0f);
        glLoadMatrixf(&projection[0][0]);

        // Set up the modelview matrix (the position and orientation of the camera)
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        // Position the camera at (2, 2, 5) looking at the origin (0, 0, 0)
        glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glLoadMatrixf(&view[0][0]);

        // Render the scene
        display();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
