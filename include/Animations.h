#pragma once

#include "FNV1A.h"  // hash_64_fnv1a_const, hash_64_fnv1a

namespace RE
{
	class BSFixedString;
}


enum class Anim : std::uint64_t
{
#pragma warning (push)
#pragma warning (disable : 4307)  // Warning C4307 '*': integral constant overflow
	kWeaponDraw = hash_64_fnv1a_const("weapondraw"),
	kWeaponSheathe = hash_64_fnv1a_const("weaponsheathe"),
	kTailCombatIdle = hash_64_fnv1a_const("tailcombatidle"),
#pragma warning (pop)
};


Anim HashAnimation(const char* a_str, std::uint32_t a_len);
Anim HashAnimation(RE::BSFixedString& a_str);
