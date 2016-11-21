#pragma once
#pragma warning( disable : 4251 ) // dll-interface
#include "../defs.hpp"
#include <memory>
#include <Eigen/Dense>
#include "../hashForwards.hpp"
#include "shapeForwards.hpp"

namespace scatdb {
	namespace shape {
		class DLEXPORT_SDBR shape
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
			void getTags(tags_t&) const;
			void setTags(const tags_t&);
			shapeHeaderStorage_p getHeader() const;
			void setHeader(shapeHeaderStorage_p);
			shapePointStatsStorage_p getStats() const;
			void getStats(shapePointStatsStorage_p) const;
			void getStats(shapePointStatsStorage_t&) const;
			hash::HASH_p hash() const;
			cache_t cache() const;
		};
	}
}
