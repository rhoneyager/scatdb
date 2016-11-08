#pragma once
#include "../scatdb/defs.hpp"

namespace scatdb {
	namespace hash {
		/// Used for hashing
		class UINT128;
		typedef UINT128 HASH_t;

		/// Wrapper function that calculates the hash of an object (key) with length (len).
		HASH_t HASH(const void *key, int len);
	}
}
