#pragma once

#include <functional>
#include <string>
#include <array>
#include <cstdint>

using Listener = std::function<void(std::string)>;

class EventQueue {
private:
    static constexpr std::size_t  _maxListeners = 256;
    std::array<std::pair<std::uint8_t, Listener>, _maxListeners> _listeners;
    std::array<std::string, _maxListeners> _queuedEvents;
    std::uint8_t _numberOfEventsInQueue;

public:
    EventQueue();

    std::uint8_t RegisterListener(Listener listener);
    void DeregisterListener(std::uint8_t identifier);

    void EnqueueEvent(std::string event);
    void BroadcastAllEvents();
};
