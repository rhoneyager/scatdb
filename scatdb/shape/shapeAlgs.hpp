#pragma once
#include "defs.hpp"
#include <vector>
#include "common.hpp"
#include "shapeBackends.hpp"
namespace Ryan_Scat {
	namespace shape {
		namespace algorithms {
			Ryan_Scat_DL shape_ptr rotate(shape_ptr src, double beta, double theta, double phi);
			Ryan_Scat_DL shape_ptr enhance(shape_ptr src, unsigned int multFactor);
			// TODO: Decimation and convolution will need special treatment in the future.
			Ryan_Scat_DL genericAlgRes_t slice(
				shape_ptr src, backends::shapeColumns_e sliceCol,
				const std::vector<float> &bounds);
		}
	}
}
