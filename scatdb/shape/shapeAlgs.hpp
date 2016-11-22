#pragma once
#include "../defs.hpp"
#include <vector>
#include "shapeForwards.hpp"
namespace scatdb {
	namespace shape {
		namespace algorithms {
			DLEXPORT_SDBR shape_ptr projectShape(shape_ptr, int axis = 1);
			DLEXPORT_SDBR void getProjectedStats(shape_ptr, float& maxDimension, float& area, float& circAreaFrac);
		}
	}
}
