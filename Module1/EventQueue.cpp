#include "EventQueue.h"
#include <algorithm>

EventQueue::EventQueue()
    : _numberOfEventsInQueue(0)
{
    int ID = 0;
    for (auto& listener : _listeners) {
        listener.first = ID++;
        listener.second = nullptr;
    }
}

std::uint8_t EventQueue::RegisterListener(Listener listener) {
    for (auto& pair : _listeners)
        if (!pair.second) {
            pair.second = listener;
            return pair.first;
        }
    return 255;
}

void EventQueue::BroadcastAllEvents() {
    for (int i = 0; i != _numberOfEventsInQueue; ++i) {
        for (auto& pair : _listeners) {
            if (pair.second)
                pair.second(_queuedEvents[i]);
        }
    }
    _numberOfEventsInQueue = 0;
}

void EventQueue::DeregisterListener(std::uint8_t identifier) {
    _listeners[identifier].second = nullptr;
}

void EventQueue::EnqueueEvent(std::string event)
{
    if (_numberOfEventsInQueue == 255) return;
    _queuedEvents[_numberOfEventsInQueue++] = event;
}

