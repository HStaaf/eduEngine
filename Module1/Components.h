#ifndef COMPONENTS_H
#define COMPONENTS_H

#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include "../src/RenderableMesh.hpp"
#include "CollisionGeometry.h"


struct AABBColliderComponent {
    AABBBoundingBox aabb;
    bool isTrigger = false;
    bool collissionTriggered = false; 


    AABBColliderComponent() = default;

    AABBColliderComponent(const glm::vec3& center, const glm::vec3& halfWidths, bool isTrigger = false, bool collissionTriggered = false)
        : aabb(center, halfWidths.x, halfWidths.y, halfWidths.z),
        collissionTriggered(collissionTriggered),
        isTrigger(isTrigger) {
    }
};

struct HorseComponent {
    bool isSpinning = false;
    float spinTimeRemaining = 0.0f;
    float spinSpeed = 180.0f;
};

struct FoodComponent {
    bool isCollected = false;

    FoodComponent() = default;
    FoodComponent(bool collected) : isCollected(collected) {}
};

struct PlaneColliderComponent {
    glm::vec3 position;  
    glm::vec3 normal;  

    PlaneColliderComponent() = default;
    PlaneColliderComponent(const glm::vec3& pos, const glm::vec3& norm)
        : position(pos), normal(glm::normalize(norm)) {
    }
};

struct SphereColliderComponent {
    Sphere localSphere; 
    bool isTrigger = false;
    bool sphereCollissionTriggered = false;
	bool planeCollissionTriggered = false;
    SphereColliderComponent() = default;

    SphereColliderComponent(const glm::vec3& offset, float radius, bool trigger = false, bool sphereCollissionTriggered = false, bool planeCollissionTriggered = false)
        : localSphere(offset, radius), isTrigger(trigger), sphereCollissionTriggered(sphereCollissionTriggered), planeCollissionTriggered(planeCollissionTriggered) {
    }
};

struct PlayerTag {};

enum AnimState:uint8_t{ Start = 0, Idle = 1, Walking = 2, Jumping = 3 };

struct AnimeComponent{
    AnimState previousState = AnimState::Start;
    AnimState currentState  = AnimState::Idle;
    float blendFactor = 0.5f;
    float blendTimer = 0.0f;
	float jumpTime = 0.0f;
    bool isGrounded = true;
};

struct TransformComponent {
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

struct LinearVelocityComponent {
    glm::vec3 velocity = glm::vec3(0.0f);
	float gravity = -9.81f;
};

struct MeshComponent {
    std::weak_ptr<eeng::RenderableMesh> mesh;
};

struct PlayerControllerComponent {
    float speed = 5.0f;
    glm::vec3 fwd = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
};

struct NPCWaypointComponent {
    std::vector<glm::vec3> waypoints;
    size_t currentWaypointIndex = 0;
    float speed = 1.0f;
};


#endif 