#pragma once
#include "../defs.hpp"
#include <Eigen/Dense>
#include <memory>
#include <map>
#include <string>
namespace scatdb {
	namespace shape {
		class shape;
		class shapeIO;
		typedef std::shared_ptr<const shape> shape_ptr;
		class shapeBackend;
		typedef std::shared_ptr<shapeBackend> shapeBackend_ptr;
		namespace backends {
			enum shapeColumns_e {
				ID, X, Y, Z, IX, IY, IZ, NUM_SHAPECOLS
			};
			enum shapeHeaderColumns_e {
				A1, A2, D, X0, NUM_SHAPEHEADERCOLS
			};
			enum shapePointStatsColumns_e {
				PTMINS, PTMAXS, NUM_SHAPEPOINTSTATSCOLS
			};
		}
		typedef Eigen::Array<float, Eigen::Dynamic, backends::NUM_SHAPECOLS> shapeStorage_t;
		typedef Eigen::Array<float, Eigen::Dynamic, 3> shapePointsOnly_t;
		typedef std::shared_ptr<const shapeStorage_t> shapeStorage_p;
		typedef std::shared_ptr<const shapePointsOnly_t> shapePointsOnly_p;
		typedef std::shared_ptr<const Eigen::ArrayXf> genericAlgRes_t;
		typedef std::map<std::string, std::string> tags_t;

		typedef Eigen::Array<float, 3, backends::NUM_SHAPEHEADERCOLS> shapeHeaderStorage_t;
		typedef std::shared_ptr<const shapeHeaderStorage_t> shapeHeaderStorage_p;

		typedef Eigen::Array<float, 3, backends::NUM_SHAPEPOINTSTATSCOLS> shapePointStatsStorage_t;
		typedef std::shared_ptr<const shapePointStatsStorage_t> shapePointStatsStorage_p;
		//typedef std::map<std::string, genericAlgRes_t> cache_t;
		typedef std::shared_ptr<std::map<std::string, genericAlgRes_t> > cache_t;
	}
}
