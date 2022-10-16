#pragma once
#include <SFML/System/Time.hpp>

#include "game_globals.h"

namespace game
{
	/**
	 * \brief it's give the actual state behavior of the player object,
	 * if INVALID the player doesn't exist physically
	 */
	enum class PlayerState
    {
        INVALID_STATE,
        IDLE,
        MOVE,
        JUMP,
        ATTACK,
        DASH
    };
class PhysicsManager;
/**
 * \brief PlayerCharacter is a struct that holds information about the player character (when they can shoot again, their current input, and their current health).
 */
struct PlayerCharacter
{
    float shootingTime = 0.0f;
    PlayerInput input = 0u;
    PlayerNumber playerNumber = INVALID_PLAYER;
    short health = playerHealth;
    float invincibilityTime = 0.0f;
    float actualJumpTime = 0.0f;
    float doubleClickTime = timeToDoubleClick + 1.0f;
    PlayerState playerState = PlayerState::IDLE;
};
class GameManager;

/**
 * \brief PlayerCharacterManager is a ComponentManager that holds all the PlayerCharacter in the game.
 */
class PlayerCharacterManager : public core::ComponentManager<PlayerCharacter, static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)>
{
public:
    explicit PlayerCharacterManager(core::EntityManager& entityManager, PhysicsManager& physicsManager, GameManager& gameManager);
    void FixedUpdate(sf::Time dt);

private:
    PhysicsManager& physicsManager_;
    GameManager& gameManager_;
};
}
