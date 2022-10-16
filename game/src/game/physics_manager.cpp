#include "game/physics_manager.h"
#include "engine/transform.h"

#include <SFML/Graphics/RectangleShape.hpp>

#ifdef TRACY_ENABLE
#include <Tracy.hpp>
#endif

namespace game
{

PhysicsManager::PhysicsManager(core::EntityManager& entityManager) :
    entityManager_(entityManager), bodyManager_(entityManager), boxManager_(entityManager)
{

}

/**
 * \brief detect if a collision occurs between two box
 * \param pos1 position of the body of the first object
 * \param extend1 extend of the box of the first object
 * \param pos2 position of the body of the second object
 * \param extend2 extend of the body of the second object 
 * \return true if collision occurs, if not return false
 */
constexpr bool Box2Box(const core::Vec2f pos1,const core::Vec2f extend1,
						const core::Vec2f pos2, const core::Vec2f extend2)
{
    return pos1.x - extend1.x <= pos2.x + extend2.x &&
        pos1.y - extend1.y <= pos2.y + extend2.y &&
        pos1.x + extend1.x >= pos2.x - extend2.x &&
        pos1.y + extend1.y >= pos2.y - extend2.y;
}
void PhysicsManager::FixedUpdate(sf::Time dt)
{
#ifdef TRACY_ENABLE
    ZoneScoped;
#endif

    UpdatePositionFromVelocity(dt);
	ResolveCollision();
	ResolveGravity(dt);
    ResolveGround();

}

void PhysicsManager::SetBody(core::Entity entity, const Body& body)
{
    bodyManager_.SetComponent(entity, body);
}

const Body& PhysicsManager::GetBody(core::Entity entity) const
{
    return bodyManager_.GetComponent(entity);
}

void PhysicsManager::AddBody(core::Entity entity)
{
    bodyManager_.AddComponent(entity);
}

void PhysicsManager::AddBox(core::Entity entity)
{
    boxManager_.AddComponent(entity);
}

void PhysicsManager::SetBox(core::Entity entity, const Box& box)
{
    boxManager_.SetComponent(entity, box);
}

const Box& PhysicsManager::GetBox(core::Entity entity) const
{
    return boxManager_.GetComponent(entity);
}

void PhysicsManager::RegisterTriggerListener(OnTriggerInterface& onTriggerInterface)
{
    onTriggerAction_.RegisterCallback(
        [&onTriggerInterface](core::Entity entity1, core::Entity entity2) { onTriggerInterface.OnTrigger(entity1, entity2); });
}

void PhysicsManager::CopyAllComponents(const PhysicsManager& physicsManager)
{
    bodyManager_.CopyAllComponents(physicsManager.bodyManager_.GetAllComponents());
    boxManager_.CopyAllComponents(physicsManager.boxManager_.GetAllComponents());
}

void PhysicsManager::Draw(sf::RenderTarget& renderTarget)
{
    for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
    {
        if (!entityManager_.HasComponent(entity,
            static_cast<core::EntityMask>(core::ComponentType::BODY2D) |
            static_cast<core::EntityMask>(core::ComponentType::BOX_COLLIDER2D)) ||
            entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
            continue;
        const auto& [extends, isTrigger] = boxManager_.GetComponent(entity);
        const auto& body = bodyManager_.GetComponent(entity);
        sf::RectangleShape rectShape;
        rectShape.setFillColor(core::Color::transparent());
        rectShape.setOutlineColor(core::Color::green());
        rectShape.setOutlineThickness(2.0f);
        const auto position = body.position;
        rectShape.setOrigin({ extends.x * core::pixelPerMeter, extends.y * core::pixelPerMeter });
        rectShape.setPosition(
            position.x * core::pixelPerMeter + center_.x,
            windowSize_.y - (position.y * core::pixelPerMeter + center_.y));
        rectShape.setSize({ extends.x * 2.0f * core::pixelPerMeter, extends.y * 2.0f * core::pixelPerMeter });
        renderTarget.draw(rectShape);
    }
}

	void PhysicsManager::UpdatePositionFromVelocity(sf::Time dt)
    {

        for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
        {
            if (!entityManager_.HasComponent(entity, static_cast<core::EntityMask>(core::ComponentType::BODY2D)))
                continue;
            auto body = bodyManager_.GetComponent(entity);

            if (body.bodyType == BodyType::DYNAMIC || body.bodyType == BodyType::KINEMATIC) {
                body.position += body.velocity * dt.asSeconds();
                body.rotation += body.angularVelocity * dt.asSeconds();
            }
            if (body.bodyType == BodyType::STATIC)
            {
                body.velocity = core::Vec2f::zero();
                body.angularVelocity = core::Degree(0.0f);
            }

            bodyManager_.SetComponent(entity, body);
        }
    }


	void PhysicsManager::ResolveCollision()
	{
        for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
        {
            if (!entityManager_.HasComponent(entity,
                static_cast<core::EntityMask>(core::ComponentType::BODY2D) |
                static_cast<core::EntityMask>(core::ComponentType::BOX_COLLIDER2D)) ||
                entityManager_.HasComponent(entity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
                continue;
            for (core::Entity otherEntity = entity + 1; otherEntity < entityManager_.GetEntitiesSize(); otherEntity++)
            {
                if (!entityManager_.HasComponent(otherEntity,
                    static_cast<core::EntityMask>(core::ComponentType::BODY2D) | static_cast<core::EntityMask>(core::ComponentType::BOX_COLLIDER2D)) ||
                    entityManager_.HasComponent(otherEntity, static_cast<core::EntityMask>(ComponentType::DESTROYED)))
                    continue;
                const Body& body1 = bodyManager_.GetComponent(entity);
                const Box& box1 = boxManager_.GetComponent(entity);

                const Body& body2 = bodyManager_.GetComponent(otherEntity);
                const Box& box2 = boxManager_.GetComponent(otherEntity);

                if (Box2Box(body1.position, box1.extends,
                    body2.position, box2.extends))
                {
                    onTriggerAction_.Execute(entity, otherEntity);
                }

            }
        }
	}

	void PhysicsManager::ResolveGravity(sf::Time dt)
	{
        for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
        {
            if (!entityManager_.HasComponent(entity, static_cast<core::EntityMask>(core::ComponentType::BODY2D)))
                continue;

            auto body = bodyManager_.GetComponent(entity);
            if(body.affectedByGravity_)
            {
                body.velocity += gravity_ * dt.asSeconds();
                bodyManager_.SetComponent(entity, body);
            }
        }
	}

    void PhysicsManager::ResolveGround()
	{
        for (core::Entity entity = 0; entity < entityManager_.GetEntitiesSize(); entity++)
        {
            if (!entityManager_.HasComponent(entity, static_cast<core::EntityMask>(core::ComponentType::BODY2D)))
                continue;

            auto body = bodyManager_.GetComponent(entity);
            if (body.affectedByGravity_ && body.position.y <= groundLevel)
            {
                body.position.y = groundLevel;
                body.velocity.y = 0.0f;
                bodyManager_.SetComponent(entity, body);
            }
        }

	}
}
