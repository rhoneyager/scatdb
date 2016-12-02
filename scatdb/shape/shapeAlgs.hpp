#pragma once
#include "../defs.hpp"
#include <vector>
#include "shapeForwards.hpp"
namespace scatdb {
	namespace shape {
		namespace algorithms {
			DLEXPORT_SDBR shape_ptr projectShape(shape_ptr, int axis = 1);
			DLEXPORT_SDBR void getProjectedStats(shape_ptr p, int axis, double dSpacingM,
				float& maxProjectedDimension_m, float& projectedArea_m2, float& circAreaFrac_dimensionless);
			DLEXPORT_SDBR void getEnvironmentConds(double alt_m, double temp_k, double &eta, double &P_air, double &rho_air, double &g);
			DLEXPORT_SDBR void getProjectedStats(shape_ptr p, double dSpacing, const std::string &dSpacingUnits,
				float& mean_maxProjectedDimension_m, float& mean_projectedArea_m2, 
				float& mean_circAreaFrac_dimensionless, float &mass_Kg, float &v_mps,
				float &vol_m, float &reff_m);
		}
	}
}
