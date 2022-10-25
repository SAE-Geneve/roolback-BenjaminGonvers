#pragma once
#include <SFML/System/Time.hpp>

#include "game_globals.h"
#include "physics_manager.h"

namespace game
{
	/**
	 * \brief it's give the actual state behavior of the player object,
	 */
	enum class PlayerState
    {
        INVALID_STATE,
        IDLE,
        MOVE,
        JUMP,
        ATTACK,
        DASH,
        STUN,
        SPAWN,
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
    float actualDashTime = 0.0f;
    float actualAttackTime = 0.0f;
    float doubleClickTimeRight = timeToDoubleClick + 1.0f;
    float doubleClickTimeLeft = timeToDoubleClick + 1.0f;
    PlayerState playerState = PlayerState::IDLE;
    /*
     *The continuous boolean var is for have the upward front this is use for double clicking
    */
    bool oldRightClick = false;
    bool oldLeftClick = false;
    bool playerFaceRight = false;
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

    /**
     * \brief check if the player have clicked last frame for reset the time of the double click and update it if not
     * \param dt delta time for update the time elapse between two click
     * \param playerCharacter the player we check
     */
    void DoubleClickTimeUpdate(sf::Time dt, PlayerCharacter& playerCharacter);
    /**
     * \brief update the oldClick to be the actual click for the next frame WARNING use that after all action, if not some misbehavior can occur
     * \param playerCharacter the player where the click need to be set
     */
    void UpdateOldClick(PlayerCharacter& playerCharacter);
    /**
     * \brief Check if the player go to dash state, if yes change the state to dash and doing the init of the dash state, if no do nothing
     * \param playerCharacter player we check
     * \param playerBody body of the player we check
     * \return true if a change state occurs, otherwise false
     */
    bool CanGoToDash(PlayerCharacter& playerCharacter, Body& playerBody);
    /**
     * \brief resolve the dash state and if the dash finish tell the player Character to go in idle state
     * \param dt delta time for update the time elapse of the dash state
     * \param playerCharacter player in dash state
     * \Param playerBody body of the player in dash state
     * \return true if a change state occurs, otherwise false
     */
    bool ResolveDash(sf::Time dt, PlayerCharacter& playerCharacter,Body& playerBody);
    /**
     * \brief Check if the player go to jump state, if yes change the state to jump and doing the init of the jump state, if no do nothing
     * \param playerCharacter player we check
     * \return true if a change state occurs, otherwise false
     */
    bool CanGoToJump(PlayerCharacter& playerCharacter);
    /**
     * \brief resolve the jump and if jump finish tell the player Character to go in idle state
     * \param dt dt delta time for update the time elapse of the jump state
     * \param playerCharacter player in jump state
     * \param playerBody body of the player in jump state
     * \return true if a change state occurs, otherwise false
     */
    bool ResolveJump(sf::Time dt, PlayerCharacter& playerCharacter, Body& playerBody);
	/**
     * \brief Check if the player go to move state, if yes change the state to move and doing the init of the move state, if no do nothing
     * \param playerCharacter player we check
     * \param playerBody body of the player we check
     * \return true if a change state occurs, otherwise false
     */
    bool CanGoToMove(PlayerCharacter& playerCharacter, Body& playerBody);
    /**
     * \brief resolve the move and if move finish tell the player Character to go in idle state
     * \param playerCharacter player in move state
     * \param playerBody body of the player in move state
     * \return true if a change state occurs, otherwise false
     */
    bool ResolveMove(PlayerCharacter& playerCharacter, Body& playerBody);
    /**
     * \brief this resolve the move state but with no state change.
     * \param playerCharacter player who can move
     * \param playerBody body of the player who can move
     */
    void Move(PlayerCharacter& playerCharacter, Body& playerBody);
	/**
     * \brief check if the player got to attack state, if yes the state to attack and doing the init of the attack state, if no do nothing
     * \param playerCharacter player who can attack
     * \param playerBody body of the player who can attack
     * \return true if a change state occurs, otherwise false
     */
    bool CanGoToAttack(PlayerCharacter& playerCharacter,const Body& playerBody);
    /**
     * \brief resolve the attack state, and tell if the state naturaly finish, if finish go to idle state.
     * \param dt delta time
     * \param playerCharacter player who is in attack state
     * \param playerBody body of player who is in attack state
     * \return true if a change state occurs, otherwise false
     */
    bool ResolveAttack(const sf::Time dt,PlayerCharacter& playerCharacter,Body& playerBody);
    /**
     * \brief resolve the idle state and lock the Y axis of the player.
     * \param playerBody body of the player in idle state.
     */
    void ResolveIdle(Body& playerBody);
};
}
