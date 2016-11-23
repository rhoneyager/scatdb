#include <string>
#include <iostream>
#include <fstream>
#include "../scatdb/error.hpp"
#include "../scatdb/shape/shape.hpp"
#include "../scatdb/shape/shapeAlgs.hpp"

namespace scatdb {
	namespace shape {

		shape_ptr projectShape(shape_ptr p, int axis) {
			auto res = p->clone();
			shapeStorage_p inpts;
			p->getPoints(inpts);
			std::shared_ptr<shapeStorage_t> projptsA(new shapeStorage_t), projptsB(new shapeStorage_t);
			*projptsA = *inpts;
			projptsA->block(0, axis, projptsA->rows(), 1).setZero();
			// Remove duplicate points
			projptsB->resizeLike(*(projptsA.get()));
			projptsB->setZero();
			int k = 0;
			using namespace backends;
			for (int i = 0; i < projptsA->rows(); ++i) {
				auto &ci = projptsA->block(i, 0, 1, NUM_SHAPECOLS);
				bool good = true;
				for (int j = 0; j < k; ++j) {
					auto &cj = projptsB->block(j, 0, 1, NUM_SHAPECOLS);
					if ((ci(0,X) == cj(0,X)) && (ci(0,Y) == cj(0,Y)) && (ci(0,Z) == cj(0,Z))) {
						good = false; break;
					}
				}
				if (good) {
					auto &ck = projptsB->block(k, 0, 1, NUM_SHAPECOLS);
					ck = ci;
					++k;
				}
			}
			projptsB->resize(k, NUM_SHAPECOLS);
			res->setPoints(projptsB);
			return res;
		}

		void getProjectedStats(shape_ptr p, float& maxDimension, float& area, float& circAreaFrac) {
			shapeStorage_p inpts;
			p->getPoints(inpts);
			std::shared_ptr<shapeStorage_t> projptsA(new shapeStorage_t);
			*projptsA = *inpts;
			using namespace backends;
			for (int i = 0; i < projptsA->rows(); ++i) {
				const auto &ci = projptsA->block(i, 0, 1, NUM_SHAPECOLS);
				for (int j = i+1; j < projptsA->rows(); ++j) {
					const auto &cj = projptsA->block(j, 0, 1, NUM_SHAPECOLS);
					float mdcsq = std::pow(ci(0,X) - cj(0,X), 2.f) + std::pow(ci(0,Y) - cj(0,Y), 2.f) + std::pow(ci(0,Z) - cj(0,Z), 2.f);
					float mdc = std::sqrt(mdcsq);
					if (maxDimension < mdc) maxDimension = mdc;
				}
			}
			area = (float) p->numPoints();
			float circArea = 3.141592654f * maxDimension;
			circAreaFrac = area / circArea;
		}

	}
}