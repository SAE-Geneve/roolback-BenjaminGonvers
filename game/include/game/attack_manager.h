#pragma once
#include <SFML/System/Time.hpp>

#include "game_globals.h"

namespace game
{
/**
 * \brief Bullet is a struct that contains info about a player bullet (when it will be destroyed and whose player it is).
 */
struct Attack
{
    float remainingTime = 0.0f;
    PlayerNumber playerNumber = INVALID_PLAYER;
};

class GameManager;

/**
 * \brief BulletManager is a ComponentManager that holds all the Bullet in one place.
 * It will automatically destroy the Bullet when remainingTime is over.
 */
class AttackManager : public core::ComponentManager<Attack, static_cast<core::EntityMask>(ComponentType::PLAYER_ATTACK)>
{
public:
    explicit AttackManager(core::EntityManager& entityManager, GameManager& gameManager);
    void FixedUpdate(sf::Time dt);
private:
    GameManager& gameManager_;
};
}
