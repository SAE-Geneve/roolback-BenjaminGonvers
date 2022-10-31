#pragma once
#include <filesystem>
#include <SFML/System/Time.hpp>

#include "game_globals.h"
#include "player_character.h"
#include "SFML/Graphics/Texture.hpp"

namespace core
{
	class SpriteManager;
}

namespace game
{
	class GameManager;

	struct AnimationData
	{
		float time = 0.0f;
		int textureIdx = 0;
		game::PlayerState oldPlayerState = PlayerState::INVALID_STATE;
	};

	struct Animation
	{
		std::vector<sf::Texture> textures{};

	};

	/**
	* \brief AnimationManager is a ComponentManager that holds all the animations in one place.
	*/
	class AnimationManager : public core::ComponentManager<AnimationData, static_cast<core::EntityMask>(ComponentType::ANIMATION_DATA)>
	{
	public:

		AnimationManager(core::EntityManager& entityManager, core::SpriteManager& spriteManager, GameManager& gameManager);

		void LoadTexture(std::string_view path, Animation& animation) const;
		void UpdateAnimation(const sf::Time dt, const core::Entity& entity);
		void UpdateAnimationCyclic(const core::Entity& entity, Animation& animation, float speed, AnimationData& animatedData);
		void UpdateAnimationLinear(const core::Entity& entity, Animation& animation, float speed, AnimationData& animatedData);


		Animation idle;
		Animation run;
		Animation jump;
		Animation attack;
		Animation dash;
		Animation stun;

	private:

		core::SpriteManager& spriteManager_;
		GameManager& gameManager_;

		bool isInit = false;
	};
	
}
