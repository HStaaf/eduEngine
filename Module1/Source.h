#pragma once

#include "Observer.h"
#include "Event.h"
#include <entt/entt.hpp>

class Source {
private:
    static constexpr std::size_t  _maxObservers = 256;
    Observer* _observers[_maxObservers];
    int _numberOfObservers = 0;

protected:
    void Notify(entt::entity source, Event event);

public:
    void AddObserver(Observer* observer);
    void RemoveObserver(Observer* observer);
};

