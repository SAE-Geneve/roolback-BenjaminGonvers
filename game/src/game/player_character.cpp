#include <game/player_character.h>
#include <game/game_manager.h>

#ifdef TRACY_ENABLE
#include <Tracy.hpp>
#endif
namespace game
{
PlayerCharacterManager::PlayerCharacterManager(core::EntityManager& entityManager, PhysicsManager& physicsManager, GameManager& gameManager) :
    ComponentManager(entityManager),
    physicsManager_(physicsManager),
    gameManager_(gameManager)

{

}

void PlayerCharacterManager::FixedUpdate(sf::Time dt)
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    for (PlayerNumber playerNumber = 0; playerNumber < maxPlayerNmb; playerNumber++)
    {
        const auto playerEntity = gameManager_.GetEntityFromPlayerNumber(playerNumber);
        if (!entityManager_.HasComponent(playerEntity,
            static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)))
            continue;
        auto playerBody = physicsManager_.GetBody(playerEntity);
        auto playerCharacter = GetComponent(playerEntity);

        DoubleClickTimeUpdate(dt, playerCharacter);

        switch (playerCharacter.playerState)
        {
        case PlayerState::IDLE:
            if(CanGoToDash(playerCharacter,playerBody))
                break;
            if(CanGoToJump(playerCharacter))
                break;
            if(CanGoToMove(playerCharacter,playerBody))
				break;
            break;
        case PlayerState::MOVE:
            if(ResolveMove(playerCharacter,playerBody))
                break;
            if(CanGoToDash(playerCharacter,playerBody))
                break;
            if(CanGoToJump(playerCharacter))
				break;
            break;
        case PlayerState::JUMP:
            if(ResolveJump(dt,playerCharacter,playerBody))
				break;
            if(CanGoToDash(playerCharacter, playerBody))
                break;
            break;
        case PlayerState::ATTACK:
            break;
        case PlayerState::DASH:
            if(ResolveDash(dt, playerCharacter))
                break;
            break;
        case PlayerState::STUN:
            break;
        case PlayerState::INVALID_STATE:
            break;

        }

        updateOldClick(playerCharacter);
        
    	physicsManager_.SetBody(playerEntity, playerBody);
        SetComponent(playerEntity, playerCharacter);



        /*if (playerCharacter.invincibilityTime > 0.0f)
        {
            playerCharacter.invincibilityTime -= dt.asSeconds();
            SetComponent(playerEntity, playerCharacter);
        }
        //Check if playerCharacter cannot shoot, and increase shootingTime
        if (playerCharacter.shootingTime < playerShootingPeriod)
        {
            playerCharacter.shootingTime += dt.asSeconds();
            SetComponent(playerEntity, playerCharacter);
        }*/

        //Shooting mechanism//todo delete this
        /*
        if (playerCharacter.shootingTime >= playerShootingPeriod)
        {
            if (input & PlayerInputEnum::PlayerInput::ATTACK)
            {
                const auto currentPlayerSpeed = playerBody.velocity.GetMagnitude();
                const auto bulletVelocity = dir *
                    ((core::Vec2f::Dot(playerBody.velocity, dir) > 0.0f ? currentPlayerSpeed : 0.0f)
                        + bulletSpeed);
                const auto bulletPosition = playerBody.position + dir * 0.5f + playerBody.velocity * dt.asSeconds();
                gameManager_.SpawnBullet(playerCharacter.playerNumber,
                    bulletPosition,
                    bulletVelocity);
                playerCharacter.shootingTime = 0.0f;
                SetComponent(playerEntity, playerCharacter);
            }
        }
        */
       
    }
}

void PlayerCharacterManager::DoubleClickTimeUpdate(const sf::Time dt,PlayerCharacter &playerCharacter)
{

    if (playerCharacter.oldRightClick && !(playerCharacter.input& PlayerInputEnum::PlayerInput::RIGHT))
    {
        playerCharacter.doubleClickTimeRight = 0.0f;
    }

    if (playerCharacter.oldLeftClick && !(playerCharacter.input & PlayerInputEnum::PlayerInput::LEFT))
    {
        playerCharacter.doubleClickTimeLeft = 0.0f;
    }

    playerCharacter.doubleClickTimeRight += dt.asSeconds();
    playerCharacter.doubleClickTimeLeft += dt.asSeconds();

}

void PlayerCharacterManager::updateOldClick(PlayerCharacter& playerCharacter)
{
    if (playerCharacter.input & PlayerInputEnum::PlayerInput::RIGHT)
    {
        playerCharacter.oldRightClick = true;
    }
    else
    {
        playerCharacter.oldRightClick = false;
    }

    if (playerCharacter.input & PlayerInputEnum::PlayerInput::LEFT)
    {
        playerCharacter.oldLeftClick = true;
    }
    else
    {
        playerCharacter.oldLeftClick = false;
    }

}

bool PlayerCharacterManager::CanGoToDash(PlayerCharacter& playerCharacter,Body& playerBody)
{
    if (playerCharacter.input & PlayerInputEnum::PlayerInput::LEFT && !playerCharacter.oldLeftClick && playerCharacter.doubleClickTimeLeft <= timeToDoubleClick
        || playerCharacter.input & PlayerInputEnum::PlayerInput::RIGHT && !playerCharacter.oldRightClick && playerCharacter.doubleClickTimeRight <= timeToDoubleClick)
    {
        playerCharacter.playerState = PlayerState::DASH;
        playerBody.velocity = core::Vec2f( playerDashSpeed* ((playerCharacter.input & PlayerInputEnum::PlayerInput::LEFT ? -1.0f : 0.0f) + (playerCharacter.input & PlayerInputEnum::PlayerInput::RIGHT ? 1.0f : 0.0f)),0.0f);
        playerCharacter.actualDashTime = 0.0f;
        return true;
    }
    return false;
}

bool PlayerCharacterManager::ResolveDash(const sf::Time dt, PlayerCharacter& playerCharacter)
{
    playerCharacter.actualDashTime += dt.asSeconds();
    if (playerCharacter.actualDashTime >= playerDashTime)
    {
        playerCharacter.playerState = PlayerState::IDLE;
        return true;
    }
    return false;
}

bool PlayerCharacterManager::CanGoToJump(PlayerCharacter& playerCharacter)
{
    if (playerCharacter.input & PlayerInputEnum::PlayerInput::UP)
    {
        playerCharacter.actualJumpTime = 0.0f;
        
        playerCharacter.playerState = PlayerState::JUMP;
        return true;
    }
    return false;
}

bool PlayerCharacterManager::ResolveJump(const sf::Time dt, PlayerCharacter& playerCharacter, Body& playerBody)
{
    playerCharacter.actualJumpTime += dt.asSeconds();

    if (playerCharacter.actualJumpTime <= playerJumpFlyTime)
    {
        playerBody.velocity.y = playerJumpSpeed - gravity.y * dt.asSeconds();
    }

    if (playerCharacter.actualJumpTime >= playerJumpFlyTime && playerBody.position.y <= groundLevel)
    {
        playerCharacter.playerState = PlayerState::IDLE;
        return true;
    }
    return  false;
}

bool PlayerCharacterManager::CanGoToMove(PlayerCharacter& playerCharacter,Body& playerBody)
{
    const auto PlayerMoveHorizontal = (playerCharacter.input & PlayerInputEnum::PlayerInput::LEFT ? -1.0f : 0.0f) +
														(playerCharacter.input & PlayerInputEnum::PlayerInput::RIGHT ? 1.0f : 0.0f) * playerSpeed;

	playerBody.velocity.x = PlayerMoveHorizontal;

	if (playerCharacter.playerState == PlayerState::IDLE && PlayerMoveHorizontal != 0.0f)
	{
            playerCharacter.playerState = PlayerState::MOVE;
            return true;
	}
    return false;
}


bool PlayerCharacterManager::ResolveMove(PlayerCharacter& playerCharacter, Body& playerBody)
{
	const auto PlayerMoveHorizontal = (playerCharacter.input & PlayerInputEnum::PlayerInput::LEFT ? -1.0f : 0.0f) +
														(playerCharacter.input & PlayerInputEnum::PlayerInput::RIGHT ? 1.0f : 0.0f) * playerSpeed;

	playerBody.velocity.x = PlayerMoveHorizontal;

	if (playerCharacter.playerState == PlayerState::MOVE && playerBody.velocity.x == 0.0f)
    {
        playerCharacter.playerState = PlayerState::IDLE;
        return true;
    }
    return false;
}

void PlayerCharacterManager::Move(PlayerCharacter& playerCharacter, Body& playerBody)
{
    const auto PlayerMoveHorizontal = (playerCharacter.input & PlayerInputEnum::PlayerInput::LEFT ? -1.0f : 0.0f) +
        (playerCharacter.input & PlayerInputEnum::PlayerInput::RIGHT ? 1.0f : 0.0f) * playerSpeed;

    playerBody.velocity.x = PlayerMoveHorizontal;
}
/*//todo
bool PlayerCharacterManager::CanGoToAttack(const sf::Time dt, PlayerCharacter& playerCharacter, Body& playerBody)
{
	if(playerCharacter.input & PlayerInputEnum::PlayerInput::ATTACK)
	{
		//todo the check
	}
}
*/
}


