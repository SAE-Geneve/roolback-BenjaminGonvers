/**
 * \file game_globals.h
 */

#pragma once
#include <SFML/Graphics/Color.hpp>
#include <array>

#include "engine/component.h"
#include "engine/entity.h"
#include "graphics/color.h"
#include "maths/angle.h"
#include "maths/vec2.h"


namespace game
{
/**
 * \brief PlayerNumber is a type used to define the number of the player.
 * Starting from 0 to maxPlayerNmb
 */
using PlayerNumber = std::uint8_t;
/**
 * \brief INVALID_PLAYER is an integer constant that defines an invalid player number.
 */
constexpr auto INVALID_PLAYER = std::numeric_limits<PlayerNumber>::max();
/**
 * \brief ClientId is a type used to define the client identification.
 * It is given by the server to clients.
 */
enum class ClientId : std::uint16_t {};
constexpr auto INVALID_CLIENT_ID = ClientId{ 0 };
using Frame = std::uint32_t;
/**
 * \brief maxPlayerNmb is a integer constant that defines the maximum number of player per game
 */
constexpr std::uint32_t maxPlayerNmb = 2;
constexpr short playerHealth = 5;
constexpr float playerSpeed = 2.0f;
constexpr float playerJumpSpeed = 3.0f;
constexpr float playerJumpFlyTime = 0.4f;
constexpr float playerDashTime = 0.2f;
constexpr float playerDashSpeed = 10.0f;
constexpr float playerStunLength = 2.0f;
constexpr core::Degree playerAngularSpeed = core::Degree(90.0f);
constexpr float playerShootingPeriod = 0.3f;
constexpr float attackScale = 0.2f;
constexpr float attackPeriod = 0.75f;
constexpr float playerInvincibilityPeriod = 1.5f;
constexpr float invincibilityFlashPeriod = 0.5f;
constexpr float groundLevel = -2.0f;
constexpr float timeToDoubleClick = 0.25f;
constexpr core::Vec2f gravity{ 0.0f,-9.81f };
constexpr float respawnDistance = 4.0f;
constexpr float animationPeriod = 1.0f;
    

/**
 * \brief windowBufferSize is the size of input stored by a client. 5 seconds of frame at 50 fps
 */
constexpr std::size_t windowBufferSize = 5u * 50u;

/**
 * \brief startDelay is the delay to wait before starting a game in milliseconds
 */
constexpr long long startDelay = 3000;
/**
 * \brief maxInputNmb is the number of inputs stored into an PlayerInputPacket
 */
constexpr std::size_t maxInputNmb = 50;
/**
 * \brief fixedPeriod is the period used in seconds to start a new FixedUpdate method in the game::GameManager
 */
constexpr float fixedPeriod = 0.02f; //50fps


constexpr std::array<core::Color, std::max(4u, maxPlayerNmb)> playerColors
{
    core::Color::red(),
    core::Color::cyan(),
    core::Color::yellow(),
    core::Color::green()
};

constexpr std::array<core::Vec2f, std::max(4u, maxPlayerNmb)> spawnPositions
{
    core::Vec2f(-0.75,-1),
    core::Vec2f(0.75,-1),
    core::Vec2f(0.5f,-1),
    core::Vec2f(-0.5f,-1),
};

constexpr std::array<core::Degree, std::max(4u, maxPlayerNmb)> spawnRotations
{
    core::Degree(0.0f),
    core::Degree(180.0f),
    core::Degree(-90.0f),
    core::Degree(90.0f)
};

enum class ComponentType : core::EntityMask
{
    PLAYER_CHARACTER = static_cast<core::EntityMask>(core::ComponentType::OTHER_TYPE),
    PLAYER_ATTACK = static_cast<core::EntityMask>(core::ComponentType::OTHER_TYPE) << 1u,
    PLATFORM = static_cast<core::EntityMask>(core::ComponentType::OTHER_TYPE) << 2u,
    PLAYER_INPUT = static_cast<core::EntityMask>(core::ComponentType::OTHER_TYPE) << 3u,
    DESTROYED = static_cast<core::EntityMask>(core::ComponentType::OTHER_TYPE) << 4u,
    ANIMATION_DATA = static_cast<core::EntityMask>(core::ComponentType::OTHER_TYPE) << 5u,
};

/**
 * \brief PlayerInput is a type defining the input data from a player.
 */
using PlayerInput = std::uint8_t;

namespace PlayerInputEnum
{
enum PlayerInput : std::uint8_t
{
    NONE = 0u,
    UP = 1u << 0u,
    DOWN = 1u << 1u,
    LEFT = 1u << 2u,
    RIGHT = 1u << 3u,
    ATTACK = 1u << 4u,
};
}
}
