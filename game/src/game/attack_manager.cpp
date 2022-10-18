#include "game/attack_manager.h"
#include "game/game_manager.h"

#ifdef TRACY_ENABLE
#include <Tracy.hpp>
#endif
namespace game
{
AttackManager::AttackManager(core::EntityManager& entityManager, GameManager& gameManager) :
    ComponentManager(entityManager), gameManager_(gameManager)
{
}

void AttackManager::FixedUpdate(sf::Time dt)
{

#ifdef TRACY_ENABLE
    ZoneScoped;
#endif
    for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
    {
        if(entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
        {
            continue;
        }
        if (entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::PLAYER_ATTACK)))
        {
        	gameManager_.DestroyAttackBox(entity);
        }
    }
}
}
