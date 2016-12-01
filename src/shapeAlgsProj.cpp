#include <string>
#include <iostream>
#include <fstream>
#include "../scatdb/error.hpp"
#include "../scatdb/logging.hpp"
#include "../scatdb/shape/shape.hpp"
#include "../scatdb/shape/shapeAlgs.hpp"
#include "../scatdb/units/units.hpp"

namespace scatdb {
	namespace shape {
		namespace algorithms {
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
					const auto &ci = projptsA->block(i, 0, 1, NUM_SHAPECOLS);
					bool good = true;
					for (int j = 0; j < k; ++j) {
						const auto &cj = projptsB->block(j, 0, 1, NUM_SHAPECOLS);
						if ((ci(0, X) == cj(0, X)) && (ci(0, Y) == cj(0, Y)) && (ci(0, Z) == cj(0, Z))) {
							good = false; break;
						}
					}
					if (good) {
						projptsB->block(k, 0, 1, NUM_SHAPECOLS) = ci;
						++k;
					}
				}
				projptsB->resize(k, NUM_SHAPECOLS);
				res->setPoints(projptsB);
				return res;
			}

			void getProjectedStats(shape_ptr p, int axis, double dSpacingM,
				float& maxProjectedDimension_m, float& projectedArea_m2, float& circAreaFrac_dimensionless) {
				auto sProj = projectShape(p, axis);

				// Project the points in each axis, and calculate the projected stats for each case. Average them.
				shapeStorage_p inpts;
				sProj->getPoints(inpts);
				using namespace backends;
				for (int i = 0; i < inpts->rows(); ++i) {
					const auto &ci = inpts->block(i, 0, 1, NUM_SHAPECOLS);
					for (int j = i + 1; j < inpts->rows(); ++j) {
						const auto &cj = inpts->block(j, 0, 1, NUM_SHAPECOLS);
						float mdcsq = std::pow(ci(0, X) - cj(0, X), 2.f) + std::pow(ci(0, Y) - cj(0, Y), 2.f) + std::pow(ci(0, Z) - cj(0, Z), 2.f);
						float mdc = (float) (std::sqrt(mdcsq) * dSpacingM);
						if (maxProjectedDimension_m < mdc) maxProjectedDimension_m = mdc;
					}
				}
				projectedArea_m2 = (float)sProj->numPoints() * (float) (dSpacingM * dSpacingM);
				float circArea = 3.141592654f * maxProjectedDimension_m * maxProjectedDimension_m / 4.f;
				circAreaFrac_dimensionless = projectedArea_m2 / circArea;
			}
			void getEnvironmentConds(double alt_m, double temp_k, double &eta, double &P_air, double &rho_air, double &g) {
				SDBR_log("shapeAlgs", logging::NOTIFICATION, "TODO: Finish implementation of getEnvironmentConds. Currently, "
					"using results for US standard atmosphere (70s?) at 4 km altitude, -10.98 Celsius");
				eta = 1.661e-5; // N . s / m^2
				P_air = 6.166e4; // N/m^2
				rho_air = 8.194e-1; // kg / m^3
				g = 9.794; // m/s^2
			}
			void getProjectedStats(shape_ptr p, double dSpacing, const std::string &dSpacingUnits,
				float& mean_maxProjectedDimension_m, float& mean_projectedArea_m2, float& mean_circAreaFrac_dimensionless, float &mass_Kg, float &v_mps) {
				Eigen::Array3f mpd, mpa, caf;
				// Convert volume into united quantity, and determine mass.
				std::shared_ptr<scatdb::units::converter> cnv = std::shared_ptr<scatdb::units::converter>(new scatdb::units::converter(dSpacingUnits, "m"));
				if (!cnv->isValid()) SDBR_throw(error::error_types::xBadInput)
					.add<std::string>("Reason", "Dipole spacing units must be units of length. Either the units are wrong, or this conversion is unsupported.")
					.add<std::string>("dSpacingUnits", dSpacingUnits);
				double dSpacingM = cnv->convert(dSpacing);
				double volumeM = std::pow(dSpacingM, 3.) * (double)p->numPoints();
				double denKgM = 916; // density of ice in kg / m^3
				double massKg = denKgM * volumeM;

				// Iterate over three projective axes
				getProjectedStats(p, 1, dSpacingM, mpd(0), mpa(0), caf(0));
				getProjectedStats(p, 2, dSpacingM, mpd(1), mpa(1), caf(1));
				getProjectedStats(p, 3, dSpacingM, mpd(2), mpa(2), caf(2));

				// Determine the means of the quantities
				mean_maxProjectedDimension_m = mpd.mean();
				mean_projectedArea_m2 = mpa.mean();
				mean_circAreaFrac_dimensionless = caf.mean();

				// Determine fall speed velocity (height assumed at 4 km, us standard 1976 atmosphere)
				double eta, rho_air, delta_0 = 8, c_0 = 0.35, g, P_air, pi = 3.141592654;
				// Look up eta, rho_air, g from table.
				double alt_m = 4000., temp_k = 0;
				getEnvironmentConds(alt_m, temp_k, eta, P_air, rho_air, g);

				double v = sqrt(rho_air * 8 * mass_Kg*g / (c_0*pi*eta*eta*sqrt(mean_circAreaFrac_dimensionless)));
				v *= 4. / (delta_0 * delta_0);
				v += 1.;
				v = sqrt(v);
				v -= 1.;
				v *= v;
				v *= delta_0*delta_0 / 4.;
				v *= eta / (rho_air * mean_maxProjectedDimension_m); // in m/s
				v_mps = (float)v;
			}
		}
	}
}