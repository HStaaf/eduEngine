#pragma once
#include "Event.h"
#include <entt/entt.hpp>

class Observer {
public:
    virtual ~Observer() = default;

    virtual void OnNotify(entt::entity source, Event event) = 0;
};

#pragma once
