#include "game/animation_manager.h"
#include "game/game_manager.h"
#include <fmt/format.h>

namespace game
{
	AnimationManager::AnimationManager(core::EntityManager& entityManager, core::SpriteManager& spriteManager, GameManager& gameManager ) :
		ComponentManager(entityManager),spriteManager_(spriteManager), gameManager_(gameManager)
{
		LoadTexture("run", run);
		LoadTexture("idle", idle);
		LoadTexture("attack", attack);
		LoadTexture("dash", dash);
		LoadTexture("jump", jump);
		LoadTexture("stun", stun);
	}

	void AnimationManager::LoadTexture(const std::string_view path, Animation& animation) const
	{
		auto format = fmt::format("data/sprites/{}", path);
		auto dirIter = std::filesystem::directory_iterator(fmt::format("data/sprites/{}", path));
		const int textureCount = std::count_if(
			begin(dirIter),
			end(dirIter),
			[](auto& entry) { return entry.is_regular_file() && entry.path().extension() == ".png"; });

		//LOAD SPRITES
		for (size_t i = 0; i < textureCount; i++)
		{
			sf::Texture newTexture;
			const auto fullPath = fmt::format("data/sprites/{}/{}{}.png", path, path, i);
			if (!newTexture.loadFromFile(fullPath))
			{
				core::LogError(fmt::format("Could not load data/sprites/{}/{}{}.png sprite", path, path, i));
			}
			animation.textures.push_back(newTexture);
		}
	}
	void AnimationManager::UpdateAnimation(const sf::Time dt,const core::Entity& entity)
	{
		AnimationData& actualAnimationData = GetComponent(entity);
		
		actualAnimationData.time += dt.asSeconds();

		const auto playerCharacter = gameManager_.GetRollbackManager().GetPlayerCharacterManager().GetComponent(entity);

		if(playerCharacter.playerState != actualAnimationData.oldPlayerState)
		{
			actualAnimationData.textureIdx = 0;
		}

		switch (playerCharacter.playerState)
		{
		case PlayerState::IDLE:
			UpdateAnimationCyclic(entity, idle, 1.0f, actualAnimationData);
			actualAnimationData.oldPlayerState = PlayerState::IDLE;
			break;
		case PlayerState::MOVE:
			UpdateAnimationCyclic(entity, run, 1.0f, actualAnimationData);
			actualAnimationData.oldPlayerState = PlayerState::MOVE;
			break;
		case PlayerState::JUMP:
			UpdateAnimationLinear(entity, jump, 1.0f, actualAnimationData);
			actualAnimationData.oldPlayerState = PlayerState::JUMP;
			break;
		case PlayerState::ATTACK:
			UpdateAnimationLinear(entity, attack, 1.0f, actualAnimationData);
			actualAnimationData.oldPlayerState = PlayerState::ATTACK;
			break;
		case PlayerState::DASH:
			UpdateAnimationCyclic(entity, dash, 1.0f, actualAnimationData);
			actualAnimationData.oldPlayerState = PlayerState::DASH;
			break;
		case PlayerState::STUN:
			UpdateAnimationCyclic(entity, stun, 1.0f, actualAnimationData);
			actualAnimationData.oldPlayerState = PlayerState::STUN;
			break;
		case PlayerState::INVALID_STATE:
			core::LogError("UpdateAnimation trying to play \"INVALID_STATE\"");
			break;

		default:
			core::LogError("UpdateAnimation not working as expected");
			break;
		}

		
		
	}


	void AnimationManager::UpdateAnimationCyclic(const core::Entity& entity, Animation& animation, float speed, AnimationData& animatedData)
	{
		auto& playerSprite = spriteManager_.GetComponent(entity);

		if (animatedData.time >= animationPeriod / speed)
		{
			animatedData.textureIdx++;
			if (animatedData.textureIdx >= animation.textures.size())
			{
				//resets the animation (it loops)
				animatedData.textureIdx = 0;
			}
			animatedData.time = 0;
		}
		//Prevents texture index from being out of range of the textures vector
		if (animatedData.textureIdx >= animation.textures.size())
		{
			animatedData.textureIdx = animation.textures.size() - 1;
		}
		playerSprite.setTexture(animation.textures[animatedData.textureIdx]);
	}
	void AnimationManager::UpdateAnimationLinear(const core::Entity& entity, Animation& animation, float speed, AnimationData& animatedData)
	{
		auto& playerSprite = spriteManager_.GetComponent(entity);

		if (animatedData.time >= animationPeriod / speed)
		{
			animatedData.textureIdx++;
			if (animatedData.textureIdx >= animation.textures.size())
			{
				//blocks the animation on last texture
				animatedData.textureIdx = animation.textures.size() - 1;
			}
			animatedData.time = 0;
		}
		if (animatedData.textureIdx >= animation.textures.size())
		{
			animatedData.textureIdx = animation.textures.size() - 1;
		}
		playerSprite.setTexture(animation.textures[animatedData.textureIdx]);
	}
}