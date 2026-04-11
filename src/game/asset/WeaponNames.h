#ifndef __WEAPON_NAMES_H__
#define __WEAPON_NAMES_H__

#include <string>

namespace WeaponNames {

static const char* TABLE[] = {
    "assault_rifle",
    "chainsaw",
    "holy_water_pistol",
    "shooting_sentry_bot",
    "exploding_sentry_bot",
    "red_shooting_sentry_bot",
    "red_exploding_sentry_bot",
    "super_shotgun",
    "chaingun",
    "assault_rifle_with_scope",
    "plasma_gun",
    "rocket_launcher",
    "bfg",
    "soul_cube",
    "item",
    "m_bite",
    "m_claw",
    "m_punch",
    "m_charge",
    "m_flesh_throw",
    "m_fireball",
    "m_plasma",
    "m_floor_strike",
    "m_fire",
    "m_machine_gun",
    "m_chain_gun",
    "m_rockets",
    "m_acid_spit",
    "m_plasma_gun",
    "m_vios_plasma",
    "m_vios_lightning",
    "m_vios_poison",
};
static const int COUNT = 32;

static std::string fromIndex(int idx) {
    if (idx >= 0 && idx < COUNT)
        return TABLE[idx];
    return std::to_string(idx);
}

static int toIndex(const std::string& name) {
    for (int i = 0; i < COUNT; i++) {
        if (name == TABLE[i])
            return i;
    }
    try {
        return std::stoi(name);
    } catch (...) {
        return 0;
    }
}

} // namespace WeaponNames

#endif
