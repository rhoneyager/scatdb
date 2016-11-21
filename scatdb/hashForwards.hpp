#pragma once
#include "defs.hpp"
#include <memory>

namespace scatdb {
	namespace hash {
		/// Used for hashing
		class UINT128;
		typedef UINT128 HASH_t;
		typedef std::shared_ptr<const HASH_t> HASH_p;
		/// Wrapper function that calculates the hash of an object (key) with length (len).
		DLEXPORT_SDBR HASH_p HASH(const void *key, int len);
	}
}
