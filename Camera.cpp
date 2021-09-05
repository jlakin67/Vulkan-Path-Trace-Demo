#include "Camera.h"
#include <iostream>

void Camera::processMouse(GLFWwindow* window, double deltaX, double deltaY)
{
    //if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
    //    deltaX = 0;
    //    deltaY = 0;
    //}
    //double xpos, ypos;
    //glfwGetCursorPos(window, &xpos, &ypos);
    //if (xpos == lastX) deltaX = 0;
    //if (ypos == lastY) deltaY = 0;
        //std::cout << "delta x: " << deltaX << std::endl;
        //std::cout << "delta y: " << deltaY << std::endl;
        //yaw += SENSITIVITY * ((float)deltaX);
        //pitch -= SENSITIVITY * ((float)deltaY);
        //if (pitch > 89.0f) pitch = 89.0f;
        //if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 right = glm::normalize(glm::cross(glm::normalize(front), glm::vec3(0.0f, 0.0f, 1.0f)));
    glm::vec3 up = glm::cross(right, front);
    front += (float)(-deltaY * SENSITIVITY) * up + (float)(deltaX * SENSITIVITY) * right;
    front = glm::normalize(front);
}

void Camera::processKeyboard(int key, float deltaTime) {
    GLfloat velocity = speed * deltaTime;
    if (key == GLFW_KEY_W) pos += front * velocity;
    else if (key == GLFW_KEY_S) pos -= front * velocity;
    else {
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 0.0f, 1.0f)));
        if (key == GLFW_KEY_D) pos += right * velocity;
        if (key == GLFW_KEY_A) pos -= right * velocity;
    }
}