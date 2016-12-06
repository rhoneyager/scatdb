#pragma once
#include "../scatdb/defs.hpp"
//#include <cmath>
#include <cstdint>
#include <string>
#include "hashForwards.hpp"

namespace scatdb {
	namespace hash {
		/// Used for hashing
		class HIDDEN_SDBR UINT128 {
		public:
			UINT128() : lower(0), upper(0) {}
			UINT128(uint64_t lower, uint64_t upper) : lower(lower), upper(upper) {}
			uint64_t lower;
			uint64_t upper;
			inline bool operator<(const UINT128 &rhs) const
			{
				if (upper != rhs.upper) return upper < rhs.upper;
				if (lower != rhs.lower) return lower < rhs.lower;
				return false;
			}
			inline bool operator==(const UINT128 &rhs) const
			{
				if (upper != rhs.upper) return false;
				if (lower != rhs.lower) return false;
				return true;
			}
			inline bool operator!=(const UINT128 &rhs) const
			{
				return !operator==(rhs);
			}
			inline UINT128 operator^(const UINT128 &rhs) const
			{
				UINT128 res = *this;
				res.lower = res.lower ^ rhs.lower;
				res.upper = res.upper ^ rhs.upper;
				return res;
			}

			std::string string() const;
		};

#define HASHSEED_SDBR 2387213
	}
}
