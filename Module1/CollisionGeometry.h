#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <glm/glm.hpp>

struct AABBBoundingBox {
    glm::vec3 center;
    float halfWidths[3];

    AABBBoundingBox() = default;

    AABBBoundingBox(const glm::vec3& c, float hwX, float hwY, float hwZ)
        : center(c) {
        halfWidths[0] = hwX;
        halfWidths[1] = hwY;
        halfWidths[2] = hwZ;
    }
};

struct Sphere {
    glm::vec3 center; 
    float radius;
	entt::entity owner = entt::null;
    Sphere() = default;
    Sphere(const glm::vec3& c, float r, entt::entity o = entt::null) : center(c), radius(r), owner(o) {}
};




struct Vector2Int {
    int x = 0;
    int y = 0;

    Vector2Int() = default;
    Vector2Int(int minIndex, int maxIndex) : x(minIndex), y(maxIndex) {}
};

std::vector<Vector2Int> FindMinMaxValues(const glm::vec3 points[], int numPoints) {

    Vector2Int minMaxX(0, 0), minMaxY(0, 0), minMaxZ(0, 0);

    for (int i = 1; i < numPoints; ++i) {
        const glm::vec3& pt = points[i];

        if (pt.x < points[minMaxX.x].x) minMaxX.x = i;
        if (pt.x > points[minMaxX.y].x) minMaxX.y = i;

        if (pt.y < points[minMaxY.x].y) minMaxY.x = i;
        if (pt.y > points[minMaxY.y].y) minMaxY.y = i;

        if (pt.z < points[minMaxZ.x].z) minMaxZ.x = i;
        if (pt.z > points[minMaxZ.y].z) minMaxZ.y = i;
    }

    return { minMaxX, minMaxY, minMaxZ };
}

Vector2Int FindMostDistantPoints(const std::vector<Vector2Int>& minMaxPoints, const glm::vec3 points[]) {
    glm::vec3 xVec = points[minMaxPoints[0].y] - points[minMaxPoints[0].x];
    float xDistance = glm::dot(xVec, xVec); // squared distance

    glm::vec3 yVec = points[minMaxPoints[1].y] - points[minMaxPoints[1].x];
    float yDistance = glm::dot(yVec, yVec);

    glm::vec3 zVec = points[minMaxPoints[2].y] - points[minMaxPoints[2].x];
    float zDistance = glm::dot(zVec, zVec);

    Vector2Int maxDistance = minMaxPoints[0];

    if (yDistance > xDistance && yDistance > zDistance)
        maxDistance = minMaxPoints[1];

    if (zDistance > xDistance && zDistance > yDistance)
        maxDistance = minMaxPoints[2];

    return maxDistance;
}


Sphere BuildSphereFromPoints(const glm::vec3 points[], int numPoints, entt::entity owner = entt::null) {
    auto minMaxVectors = FindMinMaxValues(points, numPoints);
    Vector2Int mostDistantPoints = FindMostDistantPoints(minMaxVectors, points);

    glm::vec3 p1 = points[mostDistantPoints.x];
    glm::vec3 p2 = points[mostDistantPoints.y];

    glm::vec3 center = 0.5f * (p1 + p2);
    float radius = glm::length(p2 - center);

    return Sphere(center, radius, owner);
}


AABBBoundingBox BuildAABBFromSphere(const Sphere& s)
{
    AABBBoundingBox aabb;
    aabb.center = s.center;
    for (int i = 0; i < 3; ++i)
        aabb.halfWidths[i] = s.radius;
    return aabb;
}
