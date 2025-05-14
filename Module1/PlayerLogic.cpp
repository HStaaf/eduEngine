
#include "Source.h"
#include <iostream>

struct CollisionEvent {
    entt::entity self;
    entt::entity other;
};

struct ICollisionObserver {
    virtual void OnCollision(const CollisionEvent& e) = 0;
};


// Step 3: Extend PlayerLogic to observe collisions
class PlayerLogic : public Source, public ICollisionObserver {
private:
    entt::entity entity;
	int collectedFood = 0;

public:
    PlayerLogic(entt::entity e) : entity(e) {}

    void Jump() {
        Notify(entity, Event::EVENT_PLAYER_JUMPED);
    }

    void Walk() {
        Notify(entity, Event::EVENT_PLAYER_WALKED);
    }

    entt::entity getEntity() const {
        return entity;
    }

    void OnCollision(const CollisionEvent& e) override {

        std::cout << "Player collided with entity: " << static_cast<int>(e.other) << "\n";
    }

	void CollectFood() {
		collectedFood++;
		std::cout << "Food collected: " << collectedFood << "\n";
	}

	void FeedHorse() {
        collectedFood--;
		std::cout << "Feeding horse .\n";

	}

    int CheckFood() {
        return collectedFood;
    }
};