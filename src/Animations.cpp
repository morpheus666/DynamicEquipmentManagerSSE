#include "Animations.h"

#include <cctype>  // tolower
#include <string>  // string

#include "RE/BSFixedString.h"  // BSFixedString


Anim HashAnimation(const char* a_str, std::uint32_t a_len)
{
	return Anim(hash_64_fnv1a(a_str, a_len));
}


Anim HashAnimation(RE::BSFixedString& a_str)
{
	std::string str = "";
	str.reserve(a_str.size());
	for (auto ch : a_str) {
		str += std::tolower(ch);
	}
	return HashAnimation(str.c_str(), str.length());
}
