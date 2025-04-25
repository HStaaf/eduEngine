#pragma once

#include <entt/entt.hpp>
#include "Components.h"
#include "InputManager.hpp"
#include "../src/ForwardRenderer.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

namespace eeng {
    using ForwardRendererPtr = std::shared_ptr<ForwardRenderer>;
}



// PlayerControllerSystem
inline void PlayerControllerSystem(entt::registry& registry, InputManagerPtr input) {
    using Key = eeng::InputManager::Key;

    auto view = registry.view<TransformComponent, LinearVelocityComponent, PlayerControllerComponent, AnimeComponent>();
    for (auto entity : view) {
        auto& tfm = view.get<TransformComponent>(entity);
        auto& velocity = view.get<LinearVelocityComponent>(entity);
        auto& controller = view.get<PlayerControllerComponent>(entity);
		auto& anim = view.get<AnimeComponent>(entity);

        glm::vec3 forward = controller.fwd;
        glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));
        controller.right = right;

        glm::vec3 moveDir(0.0f);
        if (input->IsKeyPressed(Key::W)) moveDir += forward;
        if (input->IsKeyPressed(Key::S)) moveDir -= forward;
        if (input->IsKeyPressed(Key::D)) moveDir += right;
        if (input->IsKeyPressed(Key::A)) moveDir -= right;

        if (glm::length(moveDir) > 0.0f) {

            moveDir = glm::normalize(moveDir);
            velocity.velocity = moveDir * controller.speed;
            tfm.rotation = glm::quatLookAtRH(-moveDir, glm::vec3(0, 1, 0));
			anim.currentState = AnimState::Walking;
        }
        else {
            velocity.velocity = glm::vec3(0.0f);
			anim.currentState = AnimState::Idle;
        }
    }
}

// MovementSystem 
inline void MovementSystem(entt::registry& registry, float deltaTime) {
    auto view = registry.view<TransformComponent, LinearVelocityComponent>();
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& velocity = view.get<LinearVelocityComponent>(entity);
        transform.position += velocity.velocity * deltaTime;
    }
}

// NPCControllerSystem 
inline void NPCControllerSystem(entt::registry& registry) {
    constexpr float proximityThresholdSq = 0.1f;
    auto view = registry.view<TransformComponent, NPCWaypointComponent, LinearVelocityComponent>();
    for (auto entity : view) {
        auto& tfm = view.get<TransformComponent>(entity);
        auto& npc = view.get<NPCWaypointComponent>(entity);
        auto& vel = view.get<LinearVelocityComponent>(entity);

        if (npc.waypoints.empty()) {
            vel.velocity = glm::vec3(0.0f);
            continue;
        }

        glm::vec3 dir = npc.waypoints[npc.currentWaypointIndex] - tfm.position;
        if (glm::length2(dir) < proximityThresholdSq) {
            npc.currentWaypointIndex = (npc.currentWaypointIndex + 1) % npc.waypoints.size();
            vel.velocity = glm::vec3(0.0f);
        }
        else {
            vel.velocity = glm::normalize(dir) * npc.speed;
        }
    }
}

// RenderSystem 
inline void RenderSystem(entt::registry& registry, eeng::ForwardRendererPtr renderer) {
    auto view = registry.view<TransformComponent, MeshComponent>();
    for (auto entity : view) {
        auto& tfm = view.get<TransformComponent>(entity);
        auto& meshComp = view.get<MeshComponent>(entity);

        if (auto mesh = meshComp.mesh.lock()) {
            glm::mat4 worldMatrix =
                glm::translate(tfm.position) *
                glm::mat4_cast(tfm.rotation) *
                glm::scale(tfm.scale);

            renderer->renderMesh(mesh, worldMatrix);
        }
    }
}

inline void AnimateSystem(entt::registry& registry, float time) {
	auto view = registry.view<TransformComponent, AnimeComponent>();
	for (auto entity : view) {
		auto& tfm = view.get<TransformComponent>(entity);
		auto& animeComp = view.get<AnimeComponent>(entity);
		// Update animation state based on time or other conditions
		// For example, you can switch between Idle and Walking states based on time
		if (time > 1.0f) {
			animeComp.currentState = AnimState::Walking;
		}
	}
}