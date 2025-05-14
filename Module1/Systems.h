#pragma once

#include <entt/entt.hpp>
#include "Components.h"
#include "InputManager.hpp"
#include "../src/ForwardRenderer.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include "ShapeRenderer.hpp"

extern std::vector<Sphere*> allSpheres;

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
inline void PlayerControllerSystem(entt::registry& registry, InputManagerPtr input, std::shared_ptr<PlayerLogic> playerLogic, EventQueue& eventQueue) {
    using Key = eeng::InputManager::Key;

    auto view = registry.view<TransformComponent, LinearVelocityComponent, PlayerControllerComponent, AnimeComponent, PlayerTag>();

	if (!view) return;

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
            eventQueue.EnqueueEvent("PLAYER_WALKED");

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
            eventQueue.EnqueueEvent("PLAYER_JUMPED");

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
    auto view = registry.view<TransformComponent, NPCWaypointComponent, LinearVelocityComponent, AnimeComponent>();
    for (auto entity : view) {
        auto& tfm = view.get<TransformComponent>(entity);
        auto& npc = view.get<NPCWaypointComponent>(entity);
        auto& vel = view.get<LinearVelocityComponent>(entity);
        auto& anim = view.get<AnimeComponent>(entity);

        if (npc.waypoints.empty()) {
            vel.velocity = glm::vec3(0.0f);
            continue;
        }

        glm::vec3 dir = npc.waypoints[npc.currentWaypointIndex] - tfm.position;
        if (glm::length2(dir) < proximityThresholdSq) {
            npc.currentWaypointIndex = (npc.currentWaypointIndex + 1) % npc.waypoints.size();
            vel.velocity = glm::vec3(0.0f);
            UpdateAnimState(anim, AnimState::Idle);
        }
        else {
            vel.velocity = glm::normalize(dir) * npc.speed;
            UpdateAnimState(anim, AnimState::Walking);
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

inline bool SphereSphereIntersection(const glm::vec3& centerA, float radiusA, const glm::vec3& centerB, float radiusB){
    float distanceSq = glm::distance2(centerA, centerB);
    float radiusSum = radiusA + radiusB;
    return distanceSq <= radiusSum * radiusSum;
}

inline void SphereCollisionSystem(entt::registry& registry)
{
    auto view = registry.view<TransformComponent, SphereColliderComponent>();

    for (auto entity : view) {
        view.get<SphereColliderComponent>(entity).sphereCollissionTriggered = false;
    }

    for (auto entityA : view) {
        const auto& transformA = view.get<TransformComponent>(entityA);
        auto& colliderA = view.get<SphereColliderComponent>(entityA);
        glm::vec3 centerA = transformA.position + colliderA.localSphere.center;
        float radiusA = colliderA.localSphere.radius;

        for (auto entityB : view) {
            if (entityA == entityB) continue;

            const auto& transformB = view.get<TransformComponent>(entityB);
            auto& colliderB = view.get<SphereColliderComponent>(entityB);
            glm::vec3 centerB = transformB.position + colliderB.localSphere.center;
            float radiusB = colliderB.localSphere.radius;

            if (SphereSphereIntersection(centerA, radiusA, centerB, radiusB)) {
                colliderA.sphereCollissionTriggered = true;
                colliderB.sphereCollissionTriggered = true;
            }
        }
    }
}

inline void SpherePlaneCollisionSystem(entt::registry& registry)
{
    auto spheres = registry.view<TransformComponent, SphereColliderComponent>();
    auto planes  = registry.view<PlaneColliderComponent>();

    for (auto sphereEntity : spheres) {
        auto& transform = spheres.get<TransformComponent>(sphereEntity);
        auto& collider = spheres.get<SphereColliderComponent>(sphereEntity);
        glm::vec3 sphereCenter = transform.position + collider.localSphere.center;
        float sphereRadius = collider.localSphere.radius;

        collider.planeCollissionTriggered = false;

        for (auto planeEntity : planes) {
            const auto& plane = planes.get<PlaneColliderComponent>(planeEntity);

            float dist = glm::dot(plane.normal, sphereCenter - plane.position);

            if (std::abs(dist) <= sphereRadius) {
                collider.planeCollissionTriggered = true;
            }
        }
    }
}

inline bool TestAABBAABB(const AABBBoundingBox& a, const AABBBoundingBox& b)
{
    float centerDiff = std::abs(a.center[0] - b.center[0]);
    float compoundedWidth = a.halfWidths[0] + b.halfWidths[0];
    if (centerDiff > compoundedWidth)
        return false;

    centerDiff = std::abs(a.center[1] - b.center[1]);
    compoundedWidth = a.halfWidths[1] + b.halfWidths[1];
    if (centerDiff > compoundedWidth)
        return false;

    centerDiff = std::abs(a.center[2] - b.center[2]);
    compoundedWidth = a.halfWidths[2] + b.halfWidths[2];
    if (centerDiff > compoundedWidth)
        return false;

    return true;
}

inline void AABBCollisionSystem(entt::registry& registry) {
    auto view = registry.view<TransformComponent, AABBColliderComponent>();

    // Reset all triggers first
    for (auto entity : view) {
        registry.get<AABBColliderComponent>(entity).collissionTriggered = false;
    }

    for (auto entityA : view) {
        auto& aTransform = registry.get<TransformComponent>(entityA);
        auto& aCollider = registry.get<AABBColliderComponent>(entityA);

        for (auto entityB : view) {
            if (entityA == entityB) continue;

            auto& bTransform = registry.get<TransformComponent>(entityB);
            auto& bCollider = registry.get<AABBColliderComponent>(entityB);

            AABBBoundingBox a = aCollider.aabb;
            AABBBoundingBox b = bCollider.aabb;

            // Optional: apply transform.position to center
            a.center += aTransform.position;
            b.center += bTransform.position;

            if (TestAABBAABB(a, b)) {
                aCollider.collissionTriggered = true;
                bCollider.collissionTriggered = true;
            }
        }
    }
}

inline bool TestAABBPlane(const AABBBoundingBox& aabb, const glm::vec3& planePoint, const glm::vec3& planeNormal)
{
    float r =
        aabb.halfWidths[0] * std::abs(glm::dot(glm::vec3(1, 0, 0), planeNormal)) +
        aabb.halfWidths[1] * std::abs(glm::dot(glm::vec3(0, 1, 0), planeNormal)) +
        aabb.halfWidths[2] * std::abs(glm::dot(glm::vec3(0, 0, 1), planeNormal));

    float s = glm::dot(planeNormal, aabb.center - planePoint);

    return std::abs(s) <= r;
}

inline void AABBPlaneCollisionSystem(entt::registry& registry) {
    auto aabbs = registry.view<TransformComponent, AABBColliderComponent>();
    auto planes = registry.view<PlaneColliderComponent>();

    for (auto entity : aabbs) {
        auto& tfm = registry.get<TransformComponent>(entity);
        auto& collider = registry.get<AABBColliderComponent>(entity);

        AABBBoundingBox aabb = collider.aabb;
        aabb.center += tfm.position;

        for (auto planeEntity : planes) {
            const auto& plane = registry.get<PlaneColliderComponent>(planeEntity);
            if (TestAABBPlane(aabb, plane.position, plane.normal)) {
                collider.collissionTriggered = true;
            }
        }
    }
}





float DistanceBetweenSpheres(Sphere* leftSphere, Sphere* rightSphere) {
    float centerDistance = glm::distance(leftSphere->center, rightSphere->center);
    float surfaceDistance = centerDistance - (leftSphere->radius + rightSphere->radius);
    return std::max(0.0f, surfaceDistance);
}

void FindMinMaxPoints(const glm::vec3 leftCenter, const glm::vec3 rightCenter, const float leftRadius,
    const float rightRadius, glm::vec3& minOut, glm::vec3& maxOut) {
    minOut.x = std::min(leftCenter.x - leftRadius, rightCenter.x - rightRadius);
    maxOut.x = std::max(leftCenter.x + leftRadius, rightCenter.x + rightRadius);

    minOut.y = std::min(leftCenter.y - leftRadius, rightCenter.y - rightRadius);
    maxOut.y = std::max(leftCenter.y + leftRadius, rightCenter.y + rightRadius);

    minOut.z = std::min(leftCenter.z - leftRadius, rightCenter.z - rightRadius);
    maxOut.z = std::max(leftCenter.z + leftRadius, rightCenter.z + rightRadius);
}

struct SphereNode {
    Sphere* collisionRepresentation;
    SphereNode* leftChild;
    SphereNode* rightChild;
};

SphereNode* BuildNodeFromSingleSphere(Sphere* sphere) {
    return new SphereNode{ sphere, nullptr, nullptr };
}

SphereNode* BuildNodeFromSpheres(Sphere* leftSphere, Sphere* rightSphere) {
    glm::vec3 minPoint, maxPoint;
    FindMinMaxPoints(leftSphere->center, rightSphere->center, leftSphere->radius,
        rightSphere->radius, minPoint, maxPoint);

    glm::vec3 midPoint = minPoint + (maxPoint - minPoint) * 0.5f;
    float radius = glm::distance(maxPoint, minPoint) * 0.5f;

    return new SphereNode{ new Sphere{ midPoint, radius }, nullptr, nullptr };
}


std::vector<std::pair<SphereNode*, SphereNode*>> FindPairs(std::vector<SphereNode*> openList,
    float maxDistance) {
    std::vector<std::pair<SphereNode*, SphereNode*>> allPairs;
    std::vector<SphereNode*> availableSpheres = openList;

    while (!availableSpheres.empty()) {
        SphereNode* current = availableSpheres.back();
        availableSpheres.pop_back();

        float closestDistance = maxDistance;
        SphereNode* bestMatch = nullptr;
        int bestIndex = -1;

        for (int j = 0; j < availableSpheres.size(); ++j) {
            float distance = DistanceBetweenSpheres(current->collisionRepresentation,
                availableSpheres[j]->collisionRepresentation);
            if (distance < closestDistance) {
                closestDistance = distance;
                bestMatch = availableSpheres[j];
                bestIndex = j;
            }
        }

        if (bestMatch) {
            availableSpheres.erase(availableSpheres.begin() + bestIndex);
        }

        allPairs.push_back({ current, bestMatch });
    }

    return allPairs;
}

SphereNode* BuildBVHBottomUp(std::vector<Sphere*> spheres, float maxDistanceBetweenLeaves) {
    std::vector<SphereNode*> openList;
    for (Sphere* sphere : spheres) {
        openList.push_back(BuildNodeFromSingleSphere(sphere));
    }

    while (openList.size() != 1) {
        auto pairs = FindPairs(openList, maxDistanceBetweenLeaves);
        openList.clear();

        for (auto pair : pairs) {
            if (pair.second) {
                auto node = BuildNodeFromSpheres(
                    pair.first->collisionRepresentation, pair.second->collisionRepresentation
                );
                node->leftChild = pair.first;
                node->rightChild = pair.second;
                openList.push_back(node);
            }
            else {
                auto node = BuildNodeFromSingleSphere(pair.first->collisionRepresentation);
                node->leftChild = pair.first;
                openList.push_back(node);
            }
        }

        maxDistanceBetweenLeaves = std::numeric_limits<float>::max();
    }

    return openList[0];
}

std::vector<Sphere*> FindPossibleCollisions(SphereNode* treeRoot, Sphere* sphere) {
    std::vector<Sphere*> possibleCollisions;

    if (!sphere || !treeRoot)
        return possibleCollisions;

    if (!SphereSphereIntersection(treeRoot->collisionRepresentation->center, treeRoot->collisionRepresentation->radius, sphere->center, sphere->radius))
        return possibleCollisions;

    if (!treeRoot->leftChild && !treeRoot->rightChild) {
        possibleCollisions.push_back(treeRoot->collisionRepresentation);
        return possibleCollisions;
    }

    auto collisions = FindPossibleCollisions(treeRoot->leftChild, sphere);
    possibleCollisions.insert(possibleCollisions.end(), collisions.begin(), collisions.end());

    collisions = FindPossibleCollisions(treeRoot->rightChild, sphere);
    possibleCollisions.insert(possibleCollisions.end(), collisions.begin(), collisions.end());

    return possibleCollisions;
}


//inline void BVHCollisionSystem(
//    entt::registry& registry,
//    std::unordered_map<entt::entity, int>& collisionCandidateCounts,
//    std::shared_ptr<PlayerLogic> playerLogic) {
//    std::vector<std::unique_ptr<Sphere>> tempSpheres; 
//    allSpheres.clear();
//    collisionCandidateCounts.clear();
//
//    auto aabbView = registry.view<AABBColliderComponent>();
//    for (auto entity : aabbView) {
//        registry.get<AABBColliderComponent>(entity).collissionTriggered = false;
//    }
//
//    auto view = registry.view<TransformComponent, SphereColliderComponent>();
//    for (auto entity : view) {
//        auto& tfm = registry.get<TransformComponent>(entity);
//        auto& col = registry.get<SphereColliderComponent>(entity);
//
//        glm::vec3 worldCenter = tfm.position + col.localSphere.center;
//        float worldRadius = col.localSphere.radius;
//
//        // Create world-space sphere, store it in temp vector for cleanup
//        auto sphere = std::make_unique<Sphere>(Sphere{ worldCenter, worldRadius, entity });
//        allSpheres.push_back(sphere.get());
//        tempSpheres.push_back(std::move(sphere));
//
//        col.sphereCollissionTriggered = false;
//    }
//
//    if (allSpheres.empty()) {
//        return;
//    }
//
//    //std::cout << "[BVH] Total spheres collected: " << allSpheres.size() << std::endl;
//
//    // Build BVH
//    SphereNode* root = BuildBVHBottomUp(allSpheres, 3.0f);
//
//    // Query each sphere against the BVH
//    for (Sphere* s : allSpheres) {
//        std::vector<Sphere*> candidates = FindPossibleCollisions(root, s);
//        collisionCandidateCounts[s->owner] = static_cast<int>(candidates.size()) - 1;
//
//        for (Sphere* other : candidates) {
//            if (s == other) continue;
//
//            // === Broad Phase: Sphere-Sphere test ===
//            if (!SphereSphereIntersection(s->center, s->radius, other->center, other->radius))
//                continue;
//
//            // === Narrow Phase: AABB-AABB test ===
//            if (registry.any_of<AABBColliderComponent>(s->owner) &&
//                registry.any_of<AABBColliderComponent>(other->owner)) {
//                auto& tfmA = registry.get<TransformComponent>(s->owner);
//                auto& tfmB = registry.get<TransformComponent>(other->owner);
//
//                AABBBoundingBox aabbA = registry.get<AABBColliderComponent>(s->owner).aabb;
//                AABBBoundingBox aabbB = registry.get<AABBColliderComponent>(other->owner).aabb;
//
//                aabbA.center += tfmA.position;
//                aabbB.center += tfmB.position;
//
//                if (TestAABBAABB(aabbA, aabbB)) {
//
//
//                    auto& colA = registry.get<SphereColliderComponent>(s->owner);
//                    auto& colB = registry.get<SphereColliderComponent>(other->owner);
//
//                    // === Skip if both are triggers ===
//                    if (colA.isTrigger && colB.isTrigger)
//                        continue;
//
//                    // === Mark both as triggered (for visualization)
//                    registry.get<AABBColliderComponent>(s->owner).collissionTriggered = true;
//                    registry.get<AABBColliderComponent>(other->owner).collissionTriggered = true;
//                    colA.sphereCollissionTriggered = true;
//                    colB.sphereCollissionTriggered = true;
//
//                    // === Observer notification (only from the trigger) ===
//                    if (colA.isTrigger && !colB.isTrigger) {
//                        if (auto logic = registry.try_get<PlayerLogic>(s->owner)) {
//                            logic->OnCollision({ s->owner, other->owner });
//                        }
//                    }
//                    else if (colB.isTrigger && !colA.isTrigger) {
//                        if (auto logic = registry.try_get<PlayerLogic>(other->owner)) {
//                            logic->OnCollision({ other->owner, s->owner });
//                        }
//                    }
//
//                    if (!colA.isTrigger && !colB.isTrigger) {
//                        glm::vec3 posA = tfmA.position + colA.localSphere.center;
//                        glm::vec3 posB = tfmB.position + colB.localSphere.center;
//                        glm::vec3 delta = posB - posA;
//                        delta.y = 0.0f;
//
//                        float dist = glm::length(delta);
//                        float rA = colA.localSphere.radius;
//                        float rB = colB.localSphere.radius;
//                        float minDist = rA + rB;
//
//                        if (dist > 0.0001f && dist < minDist) {
//                            glm::vec3 normal = delta / dist;
//                            float penetration = minDist - dist;
//                            glm::vec3 correction = normal * (penetration * 0.5f);
//
//                            tfmA.position -= correction;
//                            tfmB.position += correction;
//                        }
//
//                    }
//
//
//                }
//
//
//            }
//        }
//    }
//}


inline void BVHCollisionSystem(
    entt::registry& registry,
    std::unordered_map<entt::entity, int>& collisionCandidateCounts,
    std::shared_ptr<PlayerLogic> playerLogic)
{
    std::vector<std::unique_ptr<Sphere>> tempSpheres;
    allSpheres.clear();
    collisionCandidateCounts.clear();

    for (auto entity : registry.view<AABBColliderComponent>()) {
        registry.get<AABBColliderComponent>(entity).collissionTriggered = false;
    }

    auto view = registry.view<TransformComponent, SphereColliderComponent>();
    for (auto entity : view) {
        auto& tfm = registry.get<TransformComponent>(entity);
        auto& col = registry.get<SphereColliderComponent>(entity);

        glm::vec3 worldCenter = tfm.position + col.localSphere.center;
        float worldRadius = col.localSphere.radius;

        auto sphere = std::make_unique<Sphere>(Sphere{ worldCenter, worldRadius, entity });
        allSpheres.push_back(sphere.get());
        tempSpheres.push_back(std::move(sphere));

        col.sphereCollissionTriggered = false;
    }

    if (allSpheres.empty()) return;

    SphereNode* root = BuildBVHBottomUp(allSpheres, 3.0f);

    for (Sphere* s : allSpheres) {
        std::vector<Sphere*> candidates = FindPossibleCollisions(root, s);
        collisionCandidateCounts[s->owner] = static_cast<int>(candidates.size()) - 1;

        for (Sphere* other : candidates) {
            if (s == other) continue;
            if (!SphereSphereIntersection(s->center, s->radius, other->center, other->radius)) continue;

            if (!registry.all_of<AABBColliderComponent>(s->owner) ||
                !registry.all_of<AABBColliderComponent>(other->owner))
                continue;

            auto& tfmA = registry.get<TransformComponent>(s->owner);
            auto& tfmB = registry.get<TransformComponent>(other->owner);
            auto& colA = registry.get<SphereColliderComponent>(s->owner);
            auto& colB = registry.get<SphereColliderComponent>(other->owner);

            auto aabbA = registry.get<AABBColliderComponent>(s->owner).aabb;
            auto aabbB = registry.get<AABBColliderComponent>(other->owner).aabb;

            aabbA.center += tfmA.position;
            aabbB.center += tfmB.position;

            if (TestAABBAABB(aabbA, aabbB)) {

                registry.get<AABBColliderComponent>(s->owner).collissionTriggered = true;
                registry.get<AABBColliderComponent>(other->owner).collissionTriggered = true;
                colA.sphereCollissionTriggered = true;
                colB.sphereCollissionTriggered = true;


                std::cout << "s: " << int(s->owner)
                    << ", other: " << int(other->owner)
                    << ", player: " << int(playerLogic->getEntity()) << "\n";

                bool isSFood = registry.any_of<FoodComponent>(s->owner);
                bool isOtherFood = registry.any_of<FoodComponent>(other->owner);

                std::cout << "isSFood: " << isSFood << ", isOtherFood: " << isOtherFood << "\n";


                if (registry.any_of<FoodComponent>(s->owner) && playerLogic && playerLogic->getEntity() == other->owner) {
                    auto& food = registry.get<FoodComponent>(s->owner);
                    if (!food.isCollected) {
                        playerLogic->CollectFood();
                        food.isCollected = true;
                    }
                }
                else if (registry.any_of<FoodComponent>(other->owner) && playerLogic && playerLogic->getEntity() == s->owner) {
                    auto& food = registry.get<FoodComponent>(other->owner);
                    if (!food.isCollected) {
                        playerLogic->CollectFood();
                        food.isCollected = true;
                    }
                }

                //// Notify only from the trigger
                //if (colA.isTrigger && !colB.isTrigger && playerLogic->getEntity() == s->owner) {
                //    std::cout << "Trigger A fired PlayerLogic\n";
                //    playerLogic->OnCollision({ s->owner, other->owner });
                //}
                //else if (colB.isTrigger && !colA.isTrigger && playerLogic->getEntity() == other->owner) {
                //    std::cout << "Trigger B fired PlayerLogic\n";
                //    playerLogic->OnCollision({ other->owner, s->owner });
                //}

                // Only resolve physics if both are not triggers
                if (!colA.isTrigger && !colB.isTrigger) {
                    std::cout << "Resolving collision physically\n";
                    glm::vec3 posA = tfmA.position + colA.localSphere.center;
                    glm::vec3 posB = tfmB.position + colB.localSphere.center;
                    glm::vec3 delta = posB - posA;
                    delta.y = 0.0f;

                    float dist = glm::length(delta);
                    float minDist = colA.localSphere.radius + colB.localSphere.radius;

                    if (dist > 0.0001f && dist < minDist) {
                        glm::vec3 normal = delta / dist;
                        glm::vec3 correction = normal * (minDist - dist) * 0.5f;

                        tfmA.position -= correction;
                        tfmB.position += correction;
                    }
                }
                else {
                    std::cout << "No physical resolution due to trigger involvement\n";
                }
            }
        }
    }
}





