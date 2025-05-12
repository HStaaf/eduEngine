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

// Refacored the code that changed states. This was previously hard coded in the 
// playerEntity component which was way to tightly coupled.
inline void UpdateAnimState(AnimeComponent& anim, AnimState newState) {
    if (anim.currentState != newState) {
        anim.previousState = anim.currentState;
        anim.currentState = newState;
        anim.blendTimer = 0.0f;
    }
}

// Refactored by breaking off the code that checks if a blend is finalized into a single function 
inline void FinalizeBlend(AnimeComponent& anim) {
    if (anim.blendTimer >= 1.0f) {
        anim.blendTimer = 0.0f;
        anim.previousState = anim.currentState;
    }
}

// Refactored by moving the code which handles jumping into its own function. 
inline void ApplyJumpPhysics(TransformComponent& tfm, LinearVelocityComponent& vel, AnimeComponent& anim, float deltaTime) {
    if (!anim.isGrounded) {
        anim.jumpTime += deltaTime;
        vel.velocity.y += vel.gravity * deltaTime;  
        tfm.position.y += vel.velocity.y * deltaTime;

        if (tfm.position.y < 0.0f) {
            tfm.position.y = 0.0f;
            vel.velocity.y = 0.0f;
            anim.jumpTime = 0.0f;
            anim.isGrounded = true;
        }
    }
}


// PlayerControllerSystem
inline void PlayerControllerSystem(entt::registry& registry, InputManagerPtr input, std::shared_ptr<PlayerLogic> playerLogic) {
    using Key = eeng::InputManager::Key;

    auto view = registry.view<TransformComponent, LinearVelocityComponent, PlayerControllerComponent, AnimeComponent>();
    for (auto entity : view) {
        auto& tfm = view.get<TransformComponent>(entity);
        auto& velocity = view.get<LinearVelocityComponent>(entity);
        auto& controller = view.get<PlayerControllerComponent>(entity);
		auto& anim = view.get<AnimeComponent>(entity);

        if (!anim.isGrounded) return;

        glm::vec3 moveDir(0.0f);
        if (input->IsKeyPressed(Key::W))     moveDir += controller.fwd;
        if (input->IsKeyPressed(Key::S))     moveDir -= controller.fwd;
        if (input->IsKeyPressed(Key::D))     moveDir += controller.right;
        if (input->IsKeyPressed(Key::A))     moveDir -= controller.right;

        if (glm::length(moveDir) > 0.0f) {

            moveDir = glm::normalize(moveDir);
            velocity.velocity = moveDir * controller.speed;
            tfm.rotation = glm::quatLookAtRH(-moveDir, glm::vec3(0, 1, 0));

            UpdateAnimState(anim, AnimState::Walking);
            playerLogic->Walk();
        }
        else {
            velocity.velocity = glm::vec3(0.0f);
            UpdateAnimState(anim, AnimState::Idle);
        }

        if (input->IsKeyPressed(Key::Space)) {
            anim.isGrounded = false;
            velocity.velocity.y = 5.0f;
            UpdateAnimState(anim, AnimState::Jumping);
            playerLogic->Jump();

        }
    }
}

// MovementSystem 
inline void MovementSystem(entt::registry& registry, float deltaTime) {
    auto view = registry.view<TransformComponent, LinearVelocityComponent, AnimeComponent>();
    for (auto entity : view) {
        auto& tfm   = view.get<TransformComponent>(entity);
        auto& vel   = view.get<LinearVelocityComponent>(entity);
        auto& anim  = view.get<AnimeComponent>(entity);
        tfm.position += vel.velocity * deltaTime;
        ApplyJumpPhysics(tfm, vel, anim, deltaTime);
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
inline void RenderSystem(entt::registry& registry, eeng::ForwardRendererPtr renderer, ShapeRendererPtr shprenderer, bool drawSkeleton, float axisLen) {
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

            if (drawSkeleton) {
                for (int i = 0; i < mesh->boneMatrices.size(); ++i) {
                    auto IBinverse = glm::inverse(mesh->m_bones[i].inversebind_tfm);
                    glm::mat4 global = worldMatrix * mesh->boneMatrices[i] * IBinverse;
                    glm::vec3 pos = glm::vec3(global[3]);

                    glm::vec3 right = glm::vec3(global[0]); // X
                    glm::vec3 up = glm::vec3(global[1]); // Y
                    glm::vec3 fwd = glm::vec3(global[2]); // Z

                    shprenderer->push_states(ShapeRendering::Color4u::Red);
                    shprenderer->push_line(pos, pos + axisLen * right);

                    shprenderer->push_states(ShapeRendering::Color4u::Green);
                    shprenderer->push_line(pos, pos + axisLen * up);

                    shprenderer->push_states(ShapeRendering::Color4u::Blue);
                    shprenderer->push_line(pos, pos + axisLen * fwd);

                    shprenderer->pop_states<ShapeRendering::Color4u>();
                    shprenderer->pop_states<ShapeRendering::Color4u>();
                    shprenderer->pop_states<ShapeRendering::Color4u>();
                }
            }
        }
    }
}

inline const char* ToString(AnimState state) {
    switch (state) {
    case AnimState::Start: return "Start";
    case AnimState::Idle: return "Idle";
    case AnimState::Walking: return "Walking";
    case AnimState::Jumping: return "Jumping";
    default: return "Unknown";
    }
}

inline void AnimateSystem(entt::registry& registry, float deltaTime, 
    float totalElapsedTime, float characterAnimSpeed) {
    
    auto view = registry.view<TransformComponent, AnimeComponent, MeshComponent>();

    for (auto entity : view) {
        auto& tfm = view.get<TransformComponent>(entity);
        auto& animeComp = view.get<AnimeComponent>(entity);
        auto& meshComp = view.get<MeshComponent>(entity);

        auto mesh = meshComp.mesh.lock();

        if (animeComp.currentState != animeComp.previousState) {
            animeComp.blendTimer += deltaTime;
            float blender = glm::clamp(animeComp.blendTimer / animeComp.blendFactor, 0.0f, 1.0f);

            mesh->animateBlend(
                animeComp.previousState,
                animeComp.currentState,
                totalElapsedTime * characterAnimSpeed,
                totalElapsedTime * characterAnimSpeed,
                blender
            );

            FinalizeBlend(animeComp);
        }
        else if(animeComp.currentState == animeComp.previousState){
            mesh->animate(animeComp.currentState, totalElapsedTime * characterAnimSpeed);
        }
        else {
            eeng::Log("something is wrong");
        }
    }
}





