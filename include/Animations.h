#pragma once

#include "FNV1A.h"  // hash_64_fnv1a_const, hash_64_fnv1a

#include "RE/Skyrim.h"


enum class Anim : std::uint64_t
{
	kWeaponDraw = hash_64_fnv1a_const("weapondraw"),
	kWeaponSheathe = hash_64_fnv1a_const("weaponsheathe"),
	kTailCombatIdle = hash_64_fnv1a_const("tailcombatidle"),
	kGraphDeleting = hash_64_fnv1a_const("graphdeleting")
};


Anim HashAnimation(const char* a_str, std::uint32_t a_len);
Anim HashAnimation(const RE::BSFixedString& a_str);
