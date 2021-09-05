#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

constexpr float SENSITIVITY = 0.01f;
constexpr float SPEED = 8.0f;
constexpr float ZOOM = 45.0f;

class Camera {
public:
    Camera(glm::vec3 pos_ = glm::vec3(0.0f), float pitch_ = 90.0f, float yaw_ = 0.0f, float sensitivity_ = SENSITIVITY, float speed_ = SPEED) :
        pos{ pos_ }, pitch{ pitch_ }, yaw{ yaw_ }, sensitivity{ sensitivity_ }, speed{ speed_ }
    {
        
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    }
    void processKeyboard(int key, float deltaTime);
    void processMouse(GLFWwindow* window, double deltaX, double deltaY);
    glm::vec3 pos, front;
    float pitch, yaw, sensitivity, speed;
};