#pragma once
#include "Observer.h"
#include "Event.h"
#include <iostream>

class CalorieTracker : public Observer {
private:
    float totalCalories = 0.0f;

public:
    void OnNotify(entt::entity source, Event event) override {
        switch (event) {
        case Event::EVENT_PLAYER_JUMPED:
            totalCalories += 0.2f;
            std::cout << "Jumped! +0.2 kcal (Total: " << totalCalories << " kcal)\n";
            break;
        case Event::EVENT_PLAYER_WALKED:
            totalCalories += 0.05f;
            std::cout << "Walked! +0.05 kcal (Total: " << totalCalories << " kcal)\n";
            break;
        default:
            break;
        }
    }

    float getCalories() const { return totalCalories; }

	void AddCalories(float calories) {
		totalCalories += calories;
		std::cout << "Expended " << calories << " kcal through eventsystem (Total: " << totalCalories << " kcal)\n";
	}
};

