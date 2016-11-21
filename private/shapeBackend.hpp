#pragma once
#include "../scatdb/defs.hpp"
#include "../scatdb/shape/shape.hpp"
#include "../scatdb/hashForwards.hpp"
namespace scatdb {
	namespace shape {
		class HIDDEN_SDBR shapeBackend {
		public:
			shapeBackend();
			~shapeBackend();
			shapeStorage_p data;
			shapePointsOnly_p ptsonly;
			hash::HASH_p curHash;
			shapePointStatsStorage_p stats;
			std::string desc;
			//ingest_ptr ingest;
			tags_t tags;
			shapeHeaderStorage_p header;
			cache_t cache;
			double dSpacing;

			hash::HASH_p genHash() const;
			void invalidate();
			void genStats();
			void genDataFromPtsOnly();
		};
	}
}