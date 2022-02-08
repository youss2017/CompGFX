#pragma once
#include <cstdint>

// alignas(16)
enum class ENTITY_TYPE : uint8_t
{
    GLA_REBEL,
    GLA_RGB_TROOPER,
    GLA_TERRORIST,
    GLA_ANGRY_MOB,
    GLA_SABOTEUR,
    GLA_JARMEN_KELL,
    GLA_HIJACKER,

};

enum class ENTITY_FACTION : uint8_t
{
    GLA_TOXIN,
    GLA_STEALTH,
    GLA_DEMOLATION,
    GLA_NORMAL,

    USA_SUPERWEAPON,
    USA_AIRFORCE,
    USA_LAZER,
    USA_NORMAL,

    CHINA_NUCLEAR,
    CHINA_TANK,
    CHINA_INFANTARY,
    CHINA_NORMAL,
};

// Upgrades for soliders/workers/tanks/planes
enum class EntityFactionUpgrades {
    CAPTURE_UPGRADE,
    BOOBY_TRAPS,
    WORKER_SHOES
};

struct EntityProperties
{
    ENTITY_FACTION m_faction; // to make life easier
    ENTITY_TYPE m_type;
    uint8_t m_health;
    // TODO: include position
    uint8_t m_upgrade_level;
    uint8_t m_veterancy;
    uint8_t m_ctrl_group_id; // CTRL + 1, 2, 3, ...
    float m_max_speed;       // TODO: determine units
    float m_altitude;
};
