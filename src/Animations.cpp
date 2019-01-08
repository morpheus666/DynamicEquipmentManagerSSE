#include "Animations.h"

#include "RE/BSFixedString.h"  // BSFixedString


Anim HashAnimation(const char* a_str, std::uint32_t a_len)
{
	return Anim(hash_64_fnv1a(a_str, a_len));
}


Anim HashAnimation(RE::BSFixedString& a_str)
{
	return HashAnimation(a_str.c_str(), a_str.length());
}
