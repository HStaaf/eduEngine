#ifndef COMPONENTS_H
#define COMPONENTS_H

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include "../src/RenderableMesh.hpp"



struct PlayerTag {};

enum class AnimState{ Idle, Walking, Jumping };

struct AnimeComponent{
    AnimState previousState = AnimState::Idle;
    AnimState currentState  = AnimState::Idle;
    float blendTimer = 0.0f;
};

struct TransformComponent {
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

struct LinearVelocityComponent {
    glm::vec3 velocity = glm::vec3(0.0f);
};

struct MeshComponent {
    std::weak_ptr<eeng::RenderableMesh> mesh;
};

struct PlayerControllerComponent {
    float speed = 5.0f;
    glm::vec3 fwd = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
};

struct NPCWaypointComponent {
    std::vector<glm::vec3> waypoints;
    size_t currentWaypointIndex = 0;
    float speed = 1.0f;
};


#endif // COMPONENTS_H