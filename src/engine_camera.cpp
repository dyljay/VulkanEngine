#include "engine_camera.hpp"

#include <cassert>
#include <limits>


namespace GameEngine {

EngineCamera::EngineCamera(GLFWwindow* window) {
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfwSetCursorPosCallback(window, EngineCamera::mouseCallback);
}

EngineCamera::~EngineCamera() {} 

void EngineCamera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
  projectionMatrix = glm::mat4{1.0f};
  projectionMatrix[0][0] = 2.f / (right - left);
  projectionMatrix[1][1] = 2.f / (bottom - top);
  projectionMatrix[2][2] = 1.f / (far - near);
  projectionMatrix[3][0] = -(right + left) / (right - left);
  projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
  projectionMatrix[3][2] = -near / (far - near);
}
 
void EngineCamera::setPerspectiveProjection(float fovy, float aspect, float near, float far) {
  assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
  const float tanHalfFovy = tan(fovy / 2.f);
  projectionMatrix = glm::mat4{0.0f};
  projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
  projectionMatrix[1][1] = 1.f / (tanHalfFovy);
  projectionMatrix[2][2] = far / (far - near);
  projectionMatrix[2][3] = 1.f;
  projectionMatrix[3][2] = -(far * near) / (far - near);
}

void EngineCamera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
  const glm::vec3 w{glm::normalize(direction)};
  const glm::vec3 u{glm::normalize(glm::cross(w, up))};
  const glm::vec3 v{glm::cross(w, u)};

  viewMatrix = glm::mat4{1.f};
  viewMatrix[0][0] = u.x;
  viewMatrix[1][0] = u.y;
  viewMatrix[2][0] = u.z;
  viewMatrix[0][1] = v.x;
  viewMatrix[1][1] = v.y;
  viewMatrix[2][1] = v.z;
  viewMatrix[0][2] = w.x;
  viewMatrix[1][2] = w.y;
  viewMatrix[2][2] = w.z;
  viewMatrix[3][0] = -glm::dot(u, position);
  viewMatrix[3][1] = -glm::dot(v, position);
  viewMatrix[3][2] = -glm::dot(w, position);
    
  invViewMatrix = glm::mat4{1.f};
  invViewMatrix[0][0] = u.x;
  invViewMatrix[0][1] = u.y;
  invViewMatrix[0][2] = u.z;
  invViewMatrix[1][0] = v.x;
  invViewMatrix[1][1] = v.y;
  invViewMatrix[1][2] = v.z;
  invViewMatrix[2][0] = w.x;
  invViewMatrix[2][1] = w.y;
  invViewMatrix[2][2] = w.z;
  invViewMatrix[3][0] = position.x;
  invViewMatrix[3][1] = position.y;
  invViewMatrix[3][2] = position.z;
}

void EngineCamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
  setViewDirection(position, target - position, up);
}

void EngineCamera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
  const float c3 = glm::cos(rotation.z);
  const float s3 = glm::sin(rotation.z);
  const float c2 = glm::cos(rotation.x);
  const float s2 = glm::sin(rotation.x);
  const float c1 = glm::cos(rotation.y);
  const float s1 = glm::sin(rotation.y);
  const glm::vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
  const glm::vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
  const glm::vec3 w{(c2 * s1), (-s2), (c1 * c2)};
  viewMatrix = glm::mat4{1.f};
  viewMatrix[0][0] = u.x;
  viewMatrix[1][0] = u.y;
  viewMatrix[2][0] = u.z;
  viewMatrix[0][1] = v.x;
  viewMatrix[1][1] = v.y;
  viewMatrix[2][1] = v.z;
  viewMatrix[0][2] = w.x;
  viewMatrix[1][2] = w.y;
  viewMatrix[2][2] = w.z;
  viewMatrix[3][0] = -glm::dot(u, position);
  viewMatrix[3][1] = -glm::dot(v, position);
  viewMatrix[3][2] = -glm::dot(w, position);
    
  invViewMatrix = glm::mat4{1.f};
  invViewMatrix[0][0] = u.x;
  invViewMatrix[0][1] = u.y;
  invViewMatrix[0][2] = u.z;
  invViewMatrix[1][0] = v.x;
  invViewMatrix[1][1] = v.y;
  invViewMatrix[1][2] = v.z;
  invViewMatrix[2][0] = w.x;
  invViewMatrix[2][1] = w.y;
  invViewMatrix[2][2] = w.z;
  invViewMatrix[3][0] = position.x;
  invViewMatrix[3][1] = position.y;
  invViewMatrix[3][2] = position.z;
}

void EngineCamera::mouseCallback(GLFWwindow *window, double xposIn, double yposIn) {
    auto app = reinterpret_cast<EngineCamera*>(glfwGetWindowUserPointer(window));
    
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    
    if (app->firstMouse)
    {
        app->lastX = xpos;
        app->lastY = ypos;
        app->firstMouse = false;
    }
    
    float xoffset = xpos - app->lastX;
    float yoffset = app->lastY - ypos; // reversed since y-coordinates go from bottom to top
    
    app->lastX = xpos;
    app->lastY = ypos;
    
    app->EngineCamera::ProcessMouseMovement(xoffset, yoffset);
}

void EngineCamera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }
        
        // update Front, Right and Up Vectors using the updated Euler angles
        EngineCamera::updateCameraVectors();
}

void EngineCamera::updateCameraVectors() {
    /*
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    Up    = glm::normalize(glm::cross(Right, Front));
    */
}
}

