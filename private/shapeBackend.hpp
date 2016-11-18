#pragma once
#include "../Ryan_Scat/defs.hpp"
#include "../Ryan_Scat/shape.hpp"
namespace Ryan_Scat {
	namespace shape {
		class INTERNAL_TAG shapeBackend {
		public:
			shapeBackend();
			~shapeBackend();
			shapeStorage_p data;
			shapePointsOnly_p ptsonly;
			hash::HASH_t curHash;
			shapePointStatsStorage_p stats;
			std::string desc;
			ingest_ptr ingest;
			tags_t tags;
			shapeHeaderStorage_p header;
			cache_t cache;
			double dSpacing;

			hash::HASH_t genHash() const;
			void invalidate();
			void genStats();
			void genDataFromPtsOnly();
		};
	}
}