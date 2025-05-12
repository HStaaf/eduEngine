
#include "Source.h"

class PlayerLogic : public Source {
private:
    entt::entity entity;

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


};
