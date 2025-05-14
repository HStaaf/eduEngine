
#include <entt/entt.hpp>
#include "glmcommon.hpp"
#include "imgui.h"
#include "Log.hpp"
#include "Game.hpp"
#include "Systems.h"
#include "Components.h"
#include "CalorieTracker.cpp"
#include "Systems.h"

std::vector<Sphere*> allSpheres;

bool Game::init()
{

    forwardRenderer = std::make_shared<eeng::ForwardRenderer>();
    forwardRenderer->init("shaders/phong_vert.glsl", "shaders/phong_frag.glsl");

    shapeRenderer = std::make_shared<ShapeRendering::ShapeRenderer>();
    shapeRenderer->init();

    entity_registry = std::make_shared<entt::registry>();
    //auto ent1 = entity_registry->create();
    //entity_registry->emplace<TransformComponent>(ent1, glm::vec3{ 0.0f }, glm::vec3{ 0.0f }, glm::vec3{ 1.0f });

    #pragma region Do some entt stuff hidden
    // Do some entt stuff
    //struct Tfm
    //{
    //    float x, y, z;
    //};
    //entity_registry->emplace<Tfm>(ent1, Tfm{});
    #pragma endregion

    #pragma region Generating meshes
    // Grass
    grassMesh = std::make_shared<eeng::RenderableMesh>();
    grassMesh->load("assets/grass/grass_trees_merged2.fbx", false);
    
    // Horse
    horseMesh = std::make_shared<eeng::RenderableMesh>();
    horseMesh->load("assets/Animals/Horse.fbx", false);

    // Character
    characterMesh = std::make_shared<eeng::RenderableMesh>();

    playerMesh = std::make_shared<eeng::RenderableMesh>();
    playerMesh->load("assets/Amy/Ch46_nonPBR.fbx");
    playerMesh->load("assets/Amy/idle.fbx", true);
    playerMesh->load("assets/Amy/walking.fbx", true);
    playerMesh->load("assets/Amy/jump.fbx", true);
    playerMesh->removeTranslationKeys("mixamorig:Hips");

    // Load NPC mesh (fully independent)
    npcMesh = std::make_shared<eeng::RenderableMesh>();
    npcMesh->load("assets/Amy/Ch46_nonPBR.fbx");
    npcMesh->load("assets/Amy/idle.fbx", true);
    npcMesh->load("assets/Amy/walking.fbx", true);
    npcMesh->load("assets/Amy/jump.fbx", true);
    npcMesh->removeTranslationKeys("mixamorig:Hips");

#if 0
    // Character
    characterMesh->load("assets/Ultimate Platformer Pack/Character/Character.fbx", false);
#endif
#if 0
    // Enemy
    characterMesh->load("assets/Ultimate Platformer Pack/Enemies/Bee.fbx", false);
#endif
#if 0
    // ExoRed 5.0.1 PACK FBX, 60fps, No keyframe reduction
    characterMesh->load("assets/ExoRed/exo_red.fbx");
    characterMesh->load("assets/ExoRed/idle (2).fbx", true);
    characterMesh->load("assets/ExoRed/walking.fbx", true);
    // Remove root motion
    characterMesh->removeTranslationKeys("mixamorig:Hips");
#endif
#if 1
    // Amy 5.0.1 PACK FBX
    characterMesh->load("assets/Amy/Ch46_nonPBR.fbx");
    characterMesh->load("assets/Amy/idle.fbx", true);
    characterMesh->load("assets/Amy/walking.fbx", true);
    characterMesh->load("assets/Amy/jump.fbx", true);
    // Remove root motion
    characterMesh->removeTranslationKeys("mixamorig:Hips");
#endif
#if 0
    // Eve 5.0.1 PACK FBX
    // Fix for assimp 5.0.1 (https://github.com/assimp/assimp/issues/4486)
    // FBXConverter.cpp, line 648: 
    //      const float zero_epsilon = 1e-6f; => const float zero_epsilon = Math::getEpsilon<float>();
    characterMesh->load("assets/Eve/Eve By J.Gonzales.fbx");
    characterMesh->load("assets/Eve/idle.fbx", true);
    characterMesh->load("assets/Eve/walking.fbx", true);
    // Remove root motion
    characterMesh->removeTranslationKeys("mixamorig:Hips");
#endif
#pragma endregion

    #pragma region generating matrices for grass and horse
    grassWorldMatrix = glm_aux::TRS(
        { 0.0f, 0.0f, 0.0f },
        0.0f, { 0, 1, 0 },
        { 100.0f, 100.0f, 100.0f });

    horseWorldMatrix = glm_aux::TRS(
        { 30.0f, 0.0f, -35.0f },
        35.0f, { 0, 1, 0 },
        { 0.01f, 0.01f, 0.01f });
    #pragma endregion




    playerEntity = entity_registry->create();
    entity_registry->emplace<TransformComponent>(
        playerEntity,
        glm::vec3{ 0.0f, 0.0f, 0.0f },  // position
        glm::vec3{ 0.0f, 0.0f, 0.0f },  // rotation (Euler angles or radians)
        glm::vec3{ 0.03f, 0.03f, 0.03f }   // scale
    );
    entity_registry->emplace<PlayerTag>(playerEntity);
    entity_registry->emplace<MeshComponent>(playerEntity, playerMesh);
    entity_registry->emplace<LinearVelocityComponent>(playerEntity, glm::vec3{ 0.0f });
	entity_registry->emplace<PlayerControllerComponent>(playerEntity, 5.0f);
    entity_registry->emplace<AnimeComponent>(playerEntity, AnimState::Start, AnimState::Idle, 0.5f, 0.0f, 0.0f, true);
    
    playerLogic = std::make_shared<PlayerLogic>(playerEntity);
    calorieTracker = std::make_shared<CalorieTracker>();
    playerLogic->AddObserver(calorieTracker.get());

    glm::vec3 playerBounds[] = {
        glm::vec3(0.0f, 0.0f, 0.0f),  
        glm::vec3(0.0f, 2.2f, 0.0f)
    };

    int numPoints = sizeof(playerBounds) / sizeof(glm::vec3);
    Sphere boundingSpherePlayer = BuildSphereFromPoints(playerBounds, numPoints, playerEntity);


    entity_registry->emplace<SphereColliderComponent>(
        playerEntity,
        boundingSpherePlayer.center + glm::vec3(0.0f, 0.0f, 0.0f),    // Offset from entity position
        boundingSpherePlayer.radius,    // Radius
        false,                     // isTrigger
		false,					  // collissionTriggered
        false
    );

    AABBBoundingBox aabb = BuildAABBFromSphere(boundingSpherePlayer);
    glm::vec3 halfWidthsVec(aabb.halfWidths[0], aabb.halfWidths[1], aabb.halfWidths[2]);
    entity_registry->emplace<AABBColliderComponent>(playerEntity, aabb.center, halfWidthsVec, true, false);

    // === Add one NPC ===
    entt::entity npcEntity = entity_registry->create();
    entity_registry->emplace<TransformComponent>(
        npcEntity,
        glm::vec3{ -10.0f, 0.0f, 0.0f },
        glm::vec3{ 0.0f, 0.0f, 0.0f },
        glm::vec3{ 0.03f, 0.03f, 0.03f });
    entity_registry->emplace<MeshComponent>(npcEntity, npcMesh);
    entity_registry->emplace<AnimeComponent>(npcEntity, AnimState::Start, AnimState::Idle, 0.5f, 0.0f, 0.0f, true);

    entity_registry->emplace<LinearVelocityComponent>(npcEntity, glm::vec3{ 0.0f });

    // Waypoints and movement logic
    NPCWaypointComponent npcPath;
    npcPath.waypoints = {
        glm::vec3{ 10.0f, 0.0f, 0.0f },
        glm::vec3{ 10.0f, 0.0f, -10.0f },
        glm::vec3{ 0.0f, 0.0f, 0.0f },
    };
    npcPath.currentWaypointIndex = 0;
    npcPath.speed = 2.0f;
    entity_registry->emplace<NPCWaypointComponent>(npcEntity, npcPath);
    Sphere boundingSphereNPC = BuildSphereFromPoints(playerBounds, numPoints, npcEntity);
    entity_registry->emplace<SphereColliderComponent>(
        npcEntity,
        boundingSphereNPC.center + glm::vec3(0.0f, 0.0f, 0.0f),
        boundingSphereNPC.radius,
        false,
        false,
        false);

    AABBBoundingBox aabbNPC = BuildAABBFromSphere(boundingSphereNPC);
    glm::vec3 halfWidthsNPC(aabbNPC.halfWidths[0], aabbNPC.halfWidths[1], aabbNPC.halfWidths[2]);
    entity_registry->emplace<AABBColliderComponent>(npcEntity, aabbNPC.center, halfWidthsNPC, true, false);

    // Create ground entity with a plane collider at y = 0
    entt::entity groundEntity = entity_registry->create();
    entity_registry->emplace<TransformComponent>(
        groundEntity,
        glm::vec3{ 0, 0, 0 },   // position
        glm::vec3{ 0, 0, 0 },   // rotation
        glm::vec3{ 1.0f });     // scale
    entity_registry->emplace<PlaneColliderComponent>(
        groundEntity,
        glm::vec3{ 0, 0, 0 },   // a point on the plane
        glm::vec3{ 0, 1, 0 });  // up direction (normal)




    // FOOD Component
    entt::entity foodEntity = entity_registry->create();
    entity_registry->emplace<TransformComponent>(
        foodEntity,
        glm::vec3{ 3.0f, 0.0f, 0.0f },  
        glm::vec3{ 0.0f },
        glm::vec3{ 1.0f }
    );
    entity_registry->emplace<FoodComponent>(foodEntity, false);

    // Build AABB for the food
    glm::vec3 foodCenter = glm::vec3{ 3.0f, 0.5f, 0.0f };
    float radius = 0.3f;

    // Add AABB collider
    AABBBoundingBox foodAABB(foodCenter, radius, radius, radius);
    entity_registry->emplace<AABBColliderComponent>(
        foodEntity,
        foodAABB.center,
        glm::vec3(foodAABB.halfWidths[0], foodAABB.halfWidths[1], foodAABB.halfWidths[2]),
        false,
        false
    );

    // Add sphere collider as trigger so it enters BVH
    entity_registry->emplace<SphereColliderComponent>(
        foodEntity,
        foodCenter, // Matches AABB center
        radius,
        true,       // Trigger! No physical push
        false,
        false
    );


    eventQueue.RegisterListener([this](const std::string& e) {
        if      (e == "PLAYER_JUMPED") calorieTracker->AddCalories(0.2f);
        else if (e == "PLAYER_WALKED") calorieTracker->AddCalories(0.05f);
        });

    return true;
}

void Game::update(
    float time,
    float deltaTime,
    InputManagerPtr input)
{
	elapsedTime += time;
    if (input->IsKeyPressed(eeng::InputManager::Key::Q)) drawSkeleton = !drawSkeleton;

    updateCamera(input);

    //updatePlayer(deltaTime, input);

    PlayerControllerSystem(*entity_registry, input, playerLogic, eventQueue);
    NPCControllerSystem(*entity_registry);
    MovementSystem(*entity_registry, deltaTime);
    AnimateSystem(*entity_registry, deltaTime, time, characterAnimSpeed);
    //SphereCollisionSystem(*entity_registry);
    BVHCollisionSystem(*entity_registry, collisionCandidateCounts, playerLogic);
    SpherePlaneCollisionSystem(*entity_registry);
    AABBCollisionSystem(*entity_registry);
    AABBPlaneCollisionSystem(*entity_registry);
    //eventQueue.BroadcastAllEvents();

    pointlight.pos = glm::vec3(
        glm_aux::R(time * 0.1f, { 0.0f, 1.0f, 0.0f }) *
        glm::vec4(100.0f, 100.0f, 100.0f, 1.0f));

    //characterWorldMatrix1 = glm_aux::TRS(
    //    player.pos,
    //    0.0f, { 0, 1, 0 },
    //    { 0.03f, 0.03f, 0.03f });

    //characterWorldMatrix2 = glm_aux::TRS(
    //    { -10, 0, 0 },
    //    time * glm::radians(150.0f), { 0, 1, 0 },
    //    { 0.03f, 0.03f, 0.03f });

    //characterWorldMatrix3 = glm_aux::TRS(
    //    { 3, 0, 0 },
    //    time * glm::radians(50.0f), { 0, 1, 0 },
    //    { 0.03f, 0.03f, 0.03f });

    // Intersect player view ray with AABBs of other objects 
    glm_aux::intersect_ray_AABB(player.viewRay, character_aabb2.min, character_aabb2.max);
    glm_aux::intersect_ray_AABB(player.viewRay, character_aabb3.min, character_aabb3.max);
    glm_aux::intersect_ray_AABB(player.viewRay, horse_aabb.min, horse_aabb.max);

    // We can also compute a ray from the current mouse position,
    // to use for object picking and such ...
    if (input->GetMouseState().rightButton)
    {
        glm::ivec2 windowPos(camera.mouse_xy_prev.x, matrices.windowSize.y - camera.mouse_xy_prev.y);
        auto ray = glm_aux::world_ray_from_window_coords(windowPos, matrices.V, matrices.P, matrices.VP);
        // Intersect with e.g. AABBs ...

        eeng::Log("Picking ray origin = %s, dir = %s",
            glm_aux::to_string(ray.origin).c_str(),
            glm_aux::to_string(ray.dir).c_str());
    }
}

void Game::render(
    float time,
    int windowWidth,
    int windowHeight)
{
    renderUI();


    matrices.windowSize = glm::ivec2(windowWidth, windowHeight);

    // Projection matrix
    const float aspectRatio = float(windowWidth) / windowHeight;
    matrices.P = glm::perspective(glm::radians(60.0f), aspectRatio, camera.nearPlane, camera.farPlane);

    // View matrix
    matrices.V = glm::lookAt(camera.pos, camera.lookAt, camera.up);

    matrices.VP = glm_aux::create_viewport_matrix(0.0f, 0.0f, windowWidth, windowHeight, 0.0f, 1.0f);

    // Begin rendering pass
    forwardRenderer->beginPass(matrices.P, matrices.V, pointlight.pos, pointlight.color, camera.pos);

    RenderSystem(*entity_registry, forwardRenderer, shapeRenderer, drawSkeleton, axisLen);
    
    // Grass
    forwardRenderer->renderMesh(grassMesh, grassWorldMatrix);
    grass_aabb = grassMesh->m_model_aabb.post_transform(grassWorldMatrix);

    // Horse
    horseMesh->animate(3, time);
    forwardRenderer->renderMesh(horseMesh, horseWorldMatrix);
    horse_aabb = horseMesh->m_model_aabb.post_transform(horseWorldMatrix);


    // End rendering pass
    drawcallCount = forwardRenderer->endPass();

    // === Draw wireframe sphere colliders ===
    {
        auto view = entity_registry->view<TransformComponent, SphereColliderComponent>();
        for (auto entity : view) {

            // Skip collected food
            if (entity_registry->any_of<FoodComponent>(entity)) {
                const auto& food = entity_registry->get<FoodComponent>(entity);
                if (food.isCollected) {
                    std::cout << "Skipping rendering of food AABB for entity: " << int(entity) << "\n";
                    continue;
                }
            }

            const auto& transform = view.get<TransformComponent>(entity);
            const auto& collider = view.get<SphereColliderComponent>(entity);

            glm::vec3 worldCenter = transform.position + collider.localSphere.center;
            float radius = collider.localSphere.radius;

            ShapeRendering::Color4u color = (collider.sphereCollissionTriggered || collider.planeCollissionTriggered)
                ? ShapeRendering::Color4u{ 0xFF0000FF } : ShapeRendering::Color4u{ 0xFF00FF00 }; 

            shapeRenderer->push_states(color);

            // YZ plane (circle facing X)
            shapeRenderer->push_states(glm_aux::TS(worldCenter, radius * glm::vec3(1.0f)));
            shapeRenderer->push_circle_ring<32>();
            shapeRenderer->pop_states<glm::mat4>();

            // XZ plane (circle facing Y)
            shapeRenderer->push_states(glm_aux::TS(worldCenter, radius * glm::vec3(1.0f)) *
                glm::rotate(glm::radians(90.0f), glm::vec3(0, 0, 1)));
            shapeRenderer->push_circle_ring<32>();
            shapeRenderer->pop_states<glm::mat4>();

            // XY plane (circle facing Z)
            shapeRenderer->push_states(glm_aux::TS(worldCenter, radius * glm::vec3(1.0f)) *
                glm::rotate(glm::radians(90.0f), glm::vec3(0, 1, 0)));
            shapeRenderer->push_circle_ring<32>();
            shapeRenderer->pop_states<glm::mat4>();

            shapeRenderer->pop_states<ShapeRendering::Color4u>();
        }
    }

    // === Draw wireframe AABB colliders ===
    {
        auto view = entity_registry->view<TransformComponent, AABBColliderComponent>();
        for (auto entity : view) {

            // Skip collected food
            if (entity_registry->any_of<FoodComponent>(entity)) {
                const auto& food = entity_registry->get<FoodComponent>(entity);
                if (food.isCollected) {
                    std::cout << "Skipping rendering of food AABB for entity: " << int(entity) << "\n";
                    continue;
                }
            }

            const auto& transform = view.get<TransformComponent>(entity);
            const auto& aabbComp = view.get<AABBColliderComponent>(entity);
            const auto& aabb = aabbComp.aabb;

            glm::vec3 centerWorld = transform.position + aabb.center;
            glm::vec3 half = glm::vec3(aabb.halfWidths[0], aabb.halfWidths[1], aabb.halfWidths[2]);
            glm::vec3 min = centerWorld - half;
            glm::vec3 max = centerWorld + half;

            ShapeRendering::Color4u color = aabbComp.collissionTriggered
                ? ShapeRendering::Color4u{ 0xFFFF0000 } // Red if collided
            : ShapeRendering::Color4u{ 0xFF00FF00 }; // Green if not

            shapeRenderer->push_states(color);
            shapeRenderer->push_AABB(min, max);
            shapeRenderer->pop_states<ShapeRendering::Color4u>();
        }
    }

    #pragma region I dont know what this is so I hide it
    // Draw player view ray
    if (player.viewRay)
    {
        shapeRenderer->push_states(ShapeRendering::Color4u{ 0xff00ff00 });
        shapeRenderer->push_line(player.viewRay.origin, player.viewRay.point_of_contact());
    }
    else
    {
        shapeRenderer->push_states(ShapeRendering::Color4u{ 0xffffffff });
        shapeRenderer->push_line(player.viewRay.origin, player.viewRay.origin + player.viewRay.dir * 100.0f);
    }
    shapeRenderer->pop_states<ShapeRendering::Color4u>();

    // Draw object bases
    {
        shapeRenderer->push_basis_basic(characterWorldMatrix1, 1.0f);
        shapeRenderer->push_basis_basic(characterWorldMatrix2, 1.0f);
        shapeRenderer->push_basis_basic(characterWorldMatrix3, 1.0f);
        shapeRenderer->push_basis_basic(grassWorldMatrix, 1.0f);
        shapeRenderer->push_basis_basic(horseWorldMatrix, 1.0f);
    }

    // Draw AABBs
    {
        shapeRenderer->push_states(ShapeRendering::Color4u{ 0xFFE61A80 });
        shapeRenderer->push_AABB(character_aabb1.min, character_aabb1.max);
        shapeRenderer->push_AABB(character_aabb2.min, character_aabb2.max);
        shapeRenderer->push_AABB(character_aabb3.min, character_aabb3.max);
        shapeRenderer->push_AABB(horse_aabb.min, horse_aabb.max);
        shapeRenderer->push_AABB(grass_aabb.min, grass_aabb.max);
        shapeRenderer->pop_states<ShapeRendering::Color4u>();
    }

#if 0
    // Demo draw other shapes
    {
        shapeRenderer->push_states(glm_aux::T(glm::vec3(0.0f, 0.0f, -5.0f)));
        ShapeRendering::DemoDraw(shapeRenderer);
        shapeRenderer->pop_states<glm::mat4>();
    }
#endif
#pragma endregion
    



    // Draw shape batches
    shapeRenderer->render(matrices.P * matrices.V);
    shapeRenderer->post_render();




}

void Game::renderUI()
{
    ImGui::Begin("Game Info");

    ImGui::Text("Drawcall count %i", drawcallCount);

    if (ImGui::ColorEdit3("Light color",
        glm::value_ptr(pointlight.color),
        ImGuiColorEditFlags_NoInputs))
    {
    }

    if (characterMesh)
    {
        // Combo (drop-down) for animation clip
        int curAnimIndex = characterAnimIndex;
        std::string label = (curAnimIndex == -1 ? "Bind pose" : characterMesh->getAnimationName(curAnimIndex));
        if (ImGui::BeginCombo("Character animation##animclip", label.c_str()))
        {
            // Bind pose item
            const bool isSelected = (curAnimIndex == -1);
            if (ImGui::Selectable("Bind pose", isSelected))
                curAnimIndex = -1;
            if (isSelected)
                ImGui::SetItemDefaultFocus();

            // Clip items
            for (int i = 0; i < characterMesh->getNbrAnimations(); i++)
            {
                const bool isSelected = (curAnimIndex == i);
                const auto label = characterMesh->getAnimationName(i) + "##" + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), isSelected))
                    curAnimIndex = i;
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
            characterAnimIndex = curAnimIndex;
        }

        // In-world position label
        const auto VP_P_V = matrices.VP * matrices.P * matrices.V;
        auto world_pos = glm::vec3(horseWorldMatrix[3]);
        glm::ivec2 window_coords;
        if (glm_aux::window_coords_from_world_pos(world_pos, VP_P_V, window_coords))
        {
            ImGui::SetNextWindowPos(
                ImVec2{ float(window_coords.x), float(matrices.windowSize.y - window_coords.y) },
                ImGuiCond_Always,
                ImVec2{ 0.0f, 0.0f });
            ImGui::PushStyleColor(ImGuiCol_WindowBg, 0x80000000);
            ImGui::PushStyleColor(ImGuiCol_Text, 0xffffffff);

            ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoInputs |
                // ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_AlwaysAutoResize;

            if (ImGui::Begin("window_name", nullptr, flags))
            {
                ImGui::Text("In-world GUI element");
                ImGui::Text("Window pos (%i, %i)", window_coords.x, window_coords.x);
                ImGui::Text("World pos (%1.1f, %1.1f, %1.1f)", world_pos.x, world_pos.y, world_pos.z);
                ImGui::End();
            }
            ImGui::PopStyleColor(2);
        }
    }

    ImGui::SliderFloat("Animation speed", &characterAnimSpeed, 0.1f, 5.0f);
    
    if (auto anime = entity_registry->try_get<AnimeComponent>(playerEntity))
    {
        ImGui::Text("FSM State: %s", ToString(anime->currentState));
        ImGui::SliderFloat("Blend Factor", &anime->blendFactor, 0.0f, 1.0f);

        if (calorieTracker) {
            ImGui::Text("Calories burned: %.2f kcal", calorieTracker->getCalories());
        }

        if (collisionCandidateCounts.contains(playerEntity)) {
            ImGui::Text("Player BVH candidates: %d", collisionCandidateCounts[playerEntity]);
        }
    }
    else
    {
        ImGui::Text("No AnimeComponent found!");
    }
    ImGui::Checkbox("Draw Bone Gizmos", &drawSkeleton);

    ImGui::End(); // end info window
}

void Game::destroy()
{

}

void Game::updateCamera(
    InputManagerPtr input)
{
    // Fetch mouse and compute movement since last frame
    auto mouse = input->GetMouseState();
    glm::ivec2 mouse_xy{ mouse.x, mouse.y };
    glm::ivec2 mouse_xy_diff{ 0, 0 };
    if (mouse.leftButton && camera.mouse_xy_prev.x >= 0)
        mouse_xy_diff = camera.mouse_xy_prev - mouse_xy;
    camera.mouse_xy_prev = mouse_xy;

    // Update camera rotation from mouse movement
    camera.yaw += mouse_xy_diff.x * camera.sensitivity;
    camera.pitch += mouse_xy_diff.y * camera.sensitivity;
    camera.pitch = glm::clamp(camera.pitch, -glm::radians(89.0f), 0.0f);

    // Update camera position
    const glm::vec4 rotatedPos = glm_aux::R(camera.yaw, camera.pitch) * glm::vec4(0.0f, 0.0f, camera.distance, 1.0f);
    camera.pos = camera.lookAt + glm::vec3(rotatedPos);
}

void Game::updatePlayer(
    float deltaTime,
    InputManagerPtr input)
{
    // Fetch keys relevant for player movement
    using Key = eeng::InputManager::Key;
    bool W = input->IsKeyPressed(Key::W);
    bool A = input->IsKeyPressed(Key::A);
    bool S = input->IsKeyPressed(Key::S);
    bool D = input->IsKeyPressed(Key::D);

    // Compute vectors in the local space of the player
    player.fwd = glm::vec3(glm_aux::R(camera.yaw, glm_aux::vec3_010) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
    player.right = glm::cross(player.fwd, glm_aux::vec3_010);

    // Compute the total movement as a 3D vector
    auto movement =
        player.fwd * player.velocity * deltaTime * ((W ? 1.0f : 0.0f) + (S ? -1.0f : 0.0f)) +
        player.right * player.velocity * deltaTime * ((A ? -1.0f : 0.0f) + (D ? 1.0f : 0.0f));

    // Update player position and forward view ray
    player.pos += movement;
    player.viewRay = glm_aux::Ray{ player.pos + glm::vec3(0.0f, 2.0f, 0.0f), player.fwd };

    // Update camera to track the player
    camera.lookAt += movement;
    camera.pos += movement;

}

