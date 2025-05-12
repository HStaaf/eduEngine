#include "Source.h"


void Source::Notify(entt::entity source, Event event) {
	for (int i = 0; i < _numberOfObservers; ++i) {
		_observers[i]->OnNotify(source, event);
	}
}
// This function adds an observer to the list of observers. It checks if the number of
// observers is less than the maximum allowed, and if so, adds the observer to the list.
void Source::AddObserver(Observer* observer) {
	if (_numberOfObservers < _maxObservers) {
		_observers[_numberOfObservers++] = observer;
	}
}

// This function removes an observer from the list of observers. It also reorders the 
// remaining observers to fill the gap left by the removed observer.
void Source::RemoveObserver(Observer* observer) {
    for (int i = 0; i < _numberOfObservers; ++i) {
        if (_observers[i] == observer) {
            for (int j = i; j < _numberOfObservers - 1; ++j) {
                _observers[j] = _observers[j + 1];
            }
            _observers[_numberOfObservers - 1] = nullptr;
            --_numberOfObservers;
            break;
        }
    }
}
