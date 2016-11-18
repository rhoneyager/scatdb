#pragma once
#pragma warning( disable : 4251 ) // dll-interface
#include "defs.hpp"
#include <memory>
#include <Eigen/Dense>
#include "hash.hpp"
#include "shapeForwards.hpp"
#include "ingestForwards.hpp"

namespace Ryan_Scat {
	namespace shape {
		class Ryan_Scat_DL shape
			{
		private:
			shape();
			shapeBackend_ptr p;
		public:
			static std::shared_ptr<shape> generate();
			~shape();
			shapeStorage_p getPoints() const;
			void getPoints(shapeStorage_p&) const;
			void getPoints(shapePointsOnly_p&) const;
			void getPoints(shapePointsOnly_t&) const;
			void setPoints(const shapeStorage_p);
			void setPoints(const shapePointsOnly_p);
			void setPoints(const shapePointsOnly_t);
			size_t numPoints() const;
			std::string getDescription() const;
			void setDescription(const std::string&);
			double getPreferredDipoleSpacing() const;
			void setPreferredDipoleSpacing(double);
			ingest_ptr getIngestInformation() const;
			void setIngestInformation(const ingest_ptr);
			void getTags(tags_t&) const;
			void setTags(const tags_t&);
			shapeHeaderStorage_p getHeader() const;
			void setHeader(shapeHeaderStorage_p);
			shapePointStatsStorage_p getStats() const;
			void getStats(shapePointStatsStorage_p) const;
			void getStats(shapePointStatsStorage_t&) const;
			hash::HASH_t hash() const;
			cache_t cache() const;
		};
	}
}
