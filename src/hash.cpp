#include "../scatdb/defs.h"
#include "../private/hash.hpp"
#include "../private/MurmurHash3.h"
#include <sstream>
#include <string>

namespace scatdb {
	namespace hash {
		
		std::string UINT128::string() const {
			std::ostringstream o;
			o << lower;
			std::string res = o.str();
			return res;
		}
		HASH_t HASH(const void *key, int len)
		{
			HASH_t res;
			MurmurHash3_x64_128(key, len, HASHSEED_SDBR, &res);
			return res;
		}

	}

}

std::ostream & operator<<(std::ostream &stream, const scatdb::hash::UINT128 &ob)
{
	stream << ob.string();
	return stream;
}


