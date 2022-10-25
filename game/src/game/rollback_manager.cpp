#include <game/rollback_manager.h>
#include <game/game_manager.h>
#include "utils/assert.h"
#include <utils/log.h>
#include <fmt/format.h>

#ifdef TRACY_ENABLE
#include <Tracy.hpp>
#endif

namespace game
{

RollbackManager::RollbackManager(GameManager& gameManager, core::EntityManager& entityManager) :
    gameManager_(gameManager), entityManager_(entityManager),
    currentTransformManager_(entityManager),
    currentPhysicsManager_(entityManager), currentPlayerManager_(entityManager, currentPhysicsManager_, gameManager_),
    currentAttackManager_(entityManager, gameManager),
    lastValidatePhysicsManager_(entityManager),
    lastValidatePlayerManager_(entityManager, lastValidatePhysicsManager_, gameManager_), lastValidateAttackManager_(entityManager, gameManager)
{
    for (auto& input : inputs_)
    {
        std::fill(input.begin(), input.end(), '\0');
    }
    currentPhysicsManager_.RegisterTriggerListener(*this);
}

void RollbackManager::SimulateToCurrentFrame()
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    const auto currentFrame = gameManager_.GetCurrentFrame();
    const auto lastValidateFrame = gameManager_.GetLastValidateFrame();
    //Destroying all created Entities after the last validated frame
    for (const auto& createdEntity : createdEntities_)
    {
        if (createdEntity.createdFrame > lastValidateFrame)
        {
            entityManager_.DestroyEntity(createdEntity.entity);
        }
    }
    createdEntities_.clear();
    //Remove DESTROY flags
    for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
    {
        if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
        {
            entityManager_.RemoveComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED));
        }
    }

    //Revert the current game state to the last validated game state
    currentAttackManager_.CopyAllComponents(lastValidateAttackManager_.GetAllComponents());
    currentPhysicsManager_.CopyAllComponents(lastValidatePhysicsManager_);
    currentPlayerManager_.CopyAllComponents(lastValidatePlayerManager_.GetAllComponents());

    for (Frame frame = lastValidateFrame + 1; frame <= currentFrame; frame++)
    {
        testedFrame_ = frame;
        //Copy player inputs to player manager
        for (PlayerNumber playerNumber = 0; playerNumber < maxPlayerNmb; playerNumber++)
        {
            const auto playerInput = GetInputAtFrame(playerNumber, frame);
            const auto playerEntity = gameManager_.GetEntityFromPlayerNumber(playerNumber);
            if (playerEntity == core::INVALID_ENTITY)
            {
                core::LogWarning(fmt::format("Invalid Entity in {}:line {}", __FILE__, __LINE__));
                continue;
            }
            auto playerCharacter = currentPlayerManager_.GetComponent(playerEntity);
            playerCharacter.input = playerInput;
            currentPlayerManager_.SetComponent(playerEntity, playerCharacter);
        }
        //Simulate one frame of the game
        currentAttackManager_.FixedUpdate(sf::seconds(fixedPeriod));
        currentPlayerManager_.FixedUpdate(sf::seconds(fixedPeriod));
        currentPhysicsManager_.FixedUpdate(sf::seconds(fixedPeriod));
    }
    //Copy the physics states to the transforms
    for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
    {
        if (!entityManager_.HasComponent(entity,
            static_cast<core::EntityMask>(core::ComponentType::BODY2D) |
            static_cast<core::EntityMask>(core::ComponentType::TRANSFORM)))
            continue;
        const auto& body = currentPhysicsManager_.GetBody(entity);
        currentTransformManager_.SetPosition(entity, body.position);
        currentTransformManager_.SetRotation(entity, body.rotation);
    }
}
void RollbackManager::SetPlayerInput(PlayerNumber playerNumber, PlayerInput playerInput, Frame inputFrame)
{
    //Should only be called on the server
    if (currentFrame_ < inputFrame)
    {
        StartNewFrame(inputFrame);
    }
    inputs_[playerNumber][currentFrame_ - inputFrame] = playerInput;
    if (lastReceivedFrame_[playerNumber] < inputFrame)
    {
        lastReceivedFrame_[playerNumber] = inputFrame;
        //Repeat the same inputs until currentFrame
        for (size_t i = 0; i < currentFrame_ - inputFrame; i++)
        {
            inputs_[playerNumber][i] = playerInput;
        }
    }
}

void RollbackManager::StartNewFrame(Frame newFrame)
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    if (currentFrame_ > newFrame)
        return;
    const auto delta = newFrame - currentFrame_;
    if (delta == 0)
    {
        return;
    }
    for (auto& inputs : inputs_)
    {
        for (auto i = inputs.size() - 1; i >= delta; i--)
        {
            inputs[i] = inputs[i - delta];
        }

        for (Frame i = 0; i < delta; i++)
        {
            inputs[i] = inputs[delta];
        }
    }
    currentFrame_ = newFrame;
}

void RollbackManager::ValidateFrame(Frame newValidateFrame)
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    const auto lastValidateFrame = gameManager_.GetLastValidateFrame();
    //We check that we got all the inputs
    for (PlayerNumber playerNumber = 0; playerNumber < maxPlayerNmb; playerNumber++)
    {
        if (GetLastReceivedFrame(playerNumber) < newValidateFrame)
        {
            gpr_assert(false, "We should not validate a frame if we did not receive all inputs!!!");
            return;
        }
    }
    //Destroying all created Entities after the last validated frame
    for (const auto& createdEntity : createdEntities_)
    {
        if (createdEntity.createdFrame > lastValidateFrame)
        {
            entityManager_.DestroyEntity(createdEntity.entity);
        }
    }

    createdEntities_.clear();
    //Remove DESTROYED flag
    for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
    {
        if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
        {
            entityManager_.RemoveComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED));
        }

    }
    createdEntities_.clear();

    //We use the current game state as the temporary new validate game state
    currentAttackManager_.CopyAllComponents(lastValidateAttackManager_.GetAllComponents());
    currentPhysicsManager_.CopyAllComponents(lastValidatePhysicsManager_);
    currentPlayerManager_.CopyAllComponents(lastValidatePlayerManager_.GetAllComponents());

    //We simulate the frames until the new validated frame
    for (Frame frame = lastValidateFrame_ + 1; frame <= newValidateFrame; frame++)
    {
        testedFrame_ = frame;
        //Copy the players inputs into the player manager
        for (PlayerNumber playerNumber = 0; playerNumber < maxPlayerNmb; playerNumber++)
        {
            const auto playerInput = GetInputAtFrame(playerNumber, frame);
            const auto playerEntity = gameManager_.GetEntityFromPlayerNumber(playerNumber);
            auto playerCharacter = currentPlayerManager_.GetComponent(playerEntity);
            playerCharacter.input = playerInput;
            currentPlayerManager_.SetComponent(playerEntity, playerCharacter);
        }
        //We simulate one frame
        currentAttackManager_.FixedUpdate(sf::seconds(fixedPeriod));
        currentPlayerManager_.FixedUpdate(sf::seconds(fixedPeriod));
        currentPhysicsManager_.FixedUpdate(sf::seconds(fixedPeriod));
    }
    //Definitely remove DESTROY entities
    for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
    {
        if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
        {
            entityManager_.DestroyEntity(entity);
        }
    }
    //Copy back the new validate game state to the last validated game state
    lastValidateAttackManager_.CopyAllComponents(currentAttackManager_.GetAllComponents());
    lastValidatePlayerManager_.CopyAllComponents(currentPlayerManager_.GetAllComponents());
    lastValidatePhysicsManager_.CopyAllComponents(currentPhysicsManager_);
    lastValidateFrame_ = newValidateFrame;
    createdEntities_.clear();
}
void RollbackManager::ConfirmFrame(Frame newValidateFrame, const std::array<PhysicsState, maxPlayerNmb>& serverPhysicsState)
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    ValidateFrame(newValidateFrame);
    for (PlayerNumber playerNumber = 0; playerNumber < maxPlayerNmb; playerNumber++)
    {
        const PhysicsState lastPhysicsState = GetValidatePhysicsState(playerNumber);
        if (serverPhysicsState[playerNumber] != lastPhysicsState)
        {
            gpr_assert(false, fmt::format("Physics State are not equal for player {} (server frame: {}, client frame: {}, server: {}, client: {})", 
                playerNumber+1, 
                newValidateFrame, 
                lastValidateFrame_, 
                serverPhysicsState[playerNumber], 
                lastPhysicsState));
        }
    }
}
PhysicsState RollbackManager::GetValidatePhysicsState(PlayerNumber playerNumber) const
{
    PhysicsState state = 0;
    const core::Entity playerEntity = gameManager_.GetEntityFromPlayerNumber(playerNumber);
    const auto& playerBody = lastValidatePhysicsManager_.GetBody(playerEntity);

    const auto pos = playerBody.position;
    const auto* posPtr = reinterpret_cast<const PhysicsState*>(&pos);
    //Adding position
    for (size_t i = 0; i < sizeof(core::Vec2f) / sizeof(PhysicsState); i++)
    {
        state += posPtr[i];
    }

    //Adding velocity
    const auto velocity = playerBody.velocity;
    const auto* velocityPtr = reinterpret_cast<const PhysicsState*>(&velocity);
    for (size_t i = 0; i < sizeof(core::Vec2f) / sizeof(PhysicsState); i++)
    {
        state += velocityPtr[i];
    }
    //Adding rotation
    const auto angle = playerBody.rotation.value();
    const auto* anglePtr = reinterpret_cast<const PhysicsState*>(&angle);
    for (size_t i = 0; i < sizeof(float) / sizeof(PhysicsState); i++)
    {
        state += anglePtr[i];
    }
    //Adding angular Velocity
    const auto angularVelocity = playerBody.angularVelocity.value();
    const auto* angularVelPtr = reinterpret_cast<const PhysicsState*>(&angularVelocity);
    for (size_t i = 0; i < sizeof(float) / sizeof(PhysicsState); i++)
    {
        state += angularVelPtr[i];
    }
    return state;
}

void RollbackManager::SpawnPlayer(PlayerNumber playerNumber, core::Entity entity, core::Vec2f position)
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    Body playerBody;
    playerBody.position = position;
    Box playerBox;
    playerBox.extends = core::Vec2f::one() * 0.25f;

    PlayerCharacter playerCharacter;
    playerCharacter.playerNumber = playerNumber;

    currentPlayerManager_.AddComponent(entity);
    currentPlayerManager_.SetComponent(entity, playerCharacter);

    currentPhysicsManager_.AddBody(entity);
    currentPhysicsManager_.SetBody(entity, playerBody);
    currentPhysicsManager_.AddBox(entity);
    currentPhysicsManager_.SetBox(entity, playerBox);

    lastValidatePlayerManager_.AddComponent(entity);
    lastValidatePlayerManager_.SetComponent(entity, playerCharacter);

    lastValidatePhysicsManager_.AddBody(entity);
    lastValidatePhysicsManager_.SetBody(entity, playerBody);
    lastValidatePhysicsManager_.AddBox(entity);
    lastValidatePhysicsManager_.SetBox(entity, playerBox);

    currentTransformManager_.AddComponent(entity);
    currentTransformManager_.SetPosition(entity, position);
}

PlayerInput RollbackManager::GetInputAtFrame(PlayerNumber playerNumber, Frame frame) const
{
    gpr_assert(currentFrame_ - frame < inputs_[playerNumber].size(),
        "Trying to get input too far in the past");
    return inputs_[playerNumber][currentFrame_ - frame];
}

void RollbackManager::OnTrigger(core::Entity entity1, core::Entity entity2)
{


    const std::function<void(const PlayerCharacter&, core::Entity, const Attack&, core::Entity)> ManageCollision =
        [this](const auto& player, auto playerEntity, const auto& attack, auto attackEntity)
    {
        if (player.playerNumber != attack.playerNumber)
        {
            gameManager_.DestroyAttackBox(attackEntity);
            //lower health point
            auto playerCharacter = currentPlayerManager_.GetComponent(playerEntity);
            if (playerCharacter.invincibilityTime <= 0.0f)
            {
                core::LogDebug(fmt::format("Player {} is hit by attack", playerCharacter.playerNumber));
                playerCharacter.playerState = game::PlayerState::SPAWN;
                playerCharacter.invincibilityTime = playerInvincibilityPeriod;
            }
            currentPlayerManager_.SetComponent(playerEntity, playerCharacter);
        }
    };

    const auto ManageCollisionPlayer =
        [this](PlayerCharacter& firstPlayer, core::Entity firstPlayerEntity, PlayerCharacter& secondPlayer, core::Entity secondPlayerEntity)
    {
        if (firstPlayer.playerNumber != secondPlayer.playerNumber &&
            !(firstPlayer.playerState == PlayerState::ATTACK || secondPlayer.playerState == PlayerState::ATTACK))
        {
            auto firstPlayerBody = currentPhysicsManager_.GetBody(firstPlayerEntity);
            const auto firstPlayerBox = currentPhysicsManager_.GetBox(firstPlayerEntity);

            auto secondPlayerBody = currentPhysicsManager_.GetBody(secondPlayerEntity);
            const auto secondPlayerBox = currentPhysicsManager_.GetBox(secondPlayerEntity);

            const float firstPlayerMaxY = firstPlayerBody.position.y + firstPlayerBox.extends.y;
            const float firstPlayerMinY = firstPlayerBody.position.y - firstPlayerBox.extends.y;

            const float secondPlayerMaxY = secondPlayerBody.position.y + secondPlayerBox.extends.y;
            const float secondPlayerMinY = secondPlayerBody.position.y - secondPlayerBox.extends.y;

        	const float overlapY = std::min(firstPlayerMaxY - secondPlayerMinY, secondPlayerMaxY - firstPlayerMinY);
            
            const float firstPlayerMaxX = firstPlayerBody.position.x + firstPlayerBox.extends.x;
            const float firstPlayerMinX = firstPlayerBody.position.x - firstPlayerBox.extends.x;

            const float secondPlayerMaxX = secondPlayerBody.position.x + secondPlayerBox.extends.x;
            const float secondPlayerMinX = secondPlayerBody.position.x - secondPlayerBox.extends.x;

            const float overlapX = std::min(firstPlayerMaxX - secondPlayerMinX, secondPlayerMaxX - firstPlayerMinX);

            const float overlap = std::min(overlapY, overlapX);

            if(overlap == overlapY)
            {
	            if (overlap == firstPlayerMaxY - secondPlayerMinY)
	            {
                    firstPlayerBody.position.y -= overlap/2;
                    secondPlayerBody.position.y += overlap/2;
                    
                    if (firstPlayerBody.velocity.y > 0.0f)
                    {
                        firstPlayerBody.velocity.y = 0;
                    }
                    if (secondPlayerBody.velocity.y < 0.0f)
                    {
                        secondPlayerBody.velocity.y = 0;
                    }
                }
                else {
                    firstPlayerBody.position.y += overlap/2;
                    secondPlayerBody.position.y -= overlap/2;

                    if (firstPlayerBody.velocity.y < 0.0f)
                    {
                        firstPlayerBody.velocity.y = 0;
                    }
                    if (secondPlayerBody.velocity.y > 0.0f)
                    {
                        secondPlayerBody.velocity.y = 0;
                    }
                }

                
            }else
            {
	            if (overlap == firstPlayerMaxX - secondPlayerMinX)
	            {
                    firstPlayerBody.position.x -= overlap/2;
                    secondPlayerBody.position.x += overlap/2;
                    
                    if (firstPlayerBody.velocity.x > 0.0f)
                    {
                        firstPlayerBody.velocity.x = 0;
                    }
                    if (secondPlayerBody.velocity.x < 0.0f)
                    {
                        secondPlayerBody.velocity.x = 0;
                    }
                }
                else {
                    firstPlayerBody.position.x += overlap/2;
                    secondPlayerBody.position.x -= overlap/2;

                    if (firstPlayerBody.velocity.x < 0.0f)
                    {
                        firstPlayerBody.velocity.x = 0;
                    }
                    if (secondPlayerBody.velocity.x > 0.0f)
                    {
                        secondPlayerBody.velocity.x = 0;
                    }
                }
            }
            currentPhysicsManager_.SetBody(firstPlayerEntity, firstPlayerBody);
            currentPhysicsManager_.SetBody(secondPlayerEntity, secondPlayerBody);

            if(firstPlayer.playerState == PlayerState::DASH)
            {
                firstPlayer.playerState = PlayerState::STUN;
            }
            if(secondPlayer.playerState == PlayerState::DASH)
            {
                secondPlayer.playerState = PlayerState::STUN;
            }
        }
        else 
        {
         //todo attack variant
        }
    };

    if (entityManager_.HasComponent(entity1, static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)) &&
        entityManager_.HasComponent(entity2, static_cast<core::EntityMask>(ComponentType::PLAYER_ATTACK)))
    {
        const auto& player = currentPlayerManager_.GetComponent(entity1);
        const auto& attack = currentAttackManager_.GetComponent(entity2);
        ManageCollision(player, entity1, attack, entity2);

    }
    if (entityManager_.HasComponent(entity2, static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)) &&
        entityManager_.HasComponent(entity1, static_cast<core::EntityMask>(ComponentType::PLAYER_ATTACK)))
    {
        const auto& player = currentPlayerManager_.GetComponent(entity2);
        const auto& attack = currentAttackManager_.GetComponent(entity1);
        ManageCollision(player, entity2, attack, entity1);
    }

    if (entityManager_.HasComponent(entity1, static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)) &&
        entityManager_.HasComponent(entity2, static_cast<core::EntityMask>(ComponentType::PLAYER_CHARACTER)))
    {
        auto& firstPlayer = currentPlayerManager_.GetComponent(entity2);
        auto& secondPlayer = currentPlayerManager_.GetComponent(entity1);
        ManageCollisionPlayer(firstPlayer, entity1, secondPlayer, entity2);
    }
}

void RollbackManager::SpawnAttack(PlayerNumber playerNumber, core::Entity entity, core::Vec2f position)
{
    createdEntities_.push_back({ entity, testedFrame_ });

    Body attackBody;
    attackBody.position = position;
    Box attackBox;
    attackBox.extends = core::Vec2f::one() * AttackScale * 0.5f;

    currentAttackManager_.AddComponent(entity);
    currentAttackManager_.SetComponent(entity, { attackPeriod, playerNumber });

    currentPhysicsManager_.AddBody(entity);
    currentPhysicsManager_.SetBody(entity, attackBody);
    currentPhysicsManager_.AddBox(entity);
    currentPhysicsManager_.SetBox(entity, attackBox);

    currentTransformManager_.AddComponent(entity);
    currentTransformManager_.SetPosition(entity, position);
    currentTransformManager_.SetScale(entity, core::Vec2f::one() * AttackScale);
}

void RollbackManager::DestroyEntity(core::Entity entity)
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    //we don't need to save a bullet that has been created in the time window
    if (std::find_if(createdEntities_.begin(), createdEntities_.end(), [entity](auto newEntity)
        {
            return newEntity.entity == entity;
        }) != createdEntities_.end())
    {
        entityManager_.DestroyEntity(entity);
        return;
    }
        entityManager_.AddComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED));
}
}
