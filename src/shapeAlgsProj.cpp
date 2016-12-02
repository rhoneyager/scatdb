#include <string>
#include <iostream>
#include <fstream>
#include "../scatdb/error.hpp"
#include "../scatdb/hash.hpp"
#include "../scatdb/logging.hpp"
#include "../scatdb/shape/shape.hpp"
#include "../scatdb/shape/shapeAlgs.hpp"
#include "../scatdb/units/units.hpp"
#include "../private/chainHull.hpp"

namespace scatdb {
	namespace shape {
		namespace algorithms {
			namespace backend {
				HIDDEN_SDBR void convertShapeTo2DpointArray(
					shape_ptr p, int ignoredAxis, std::vector<contrib::chainHull::Point> &res)
				{
					res.clear();
					res.reserve(p->numPoints());
					shapePointsOnly_t pts;
					p->getPoints(pts);
					for (int i = 0; i < pts.rows(); ++i) {
						float a, b;
						if (ignoredAxis == 1) { a = pts(i, 1); b = pts(i, 2); }
						else if (ignoredAxis == 2) { a = pts(i, 0); b = pts(i, 2); }
						else if (ignoredAxis == 3) { a = pts(i, 0); b = pts(i, 1); }
						else SDBR_throw(error::error_types::xBadInput)
							.add<std::string>("Reason", "Bad input axis to be ignored. Must be 1, 2 or 3.")
							.add<int>("ignoredAxis", ignoredAxis);
						res.push_back(std::move(contrib::chainHull::Point(a, b)));
					}
					// Sort the resulting points
					std::sort(res.begin(), res.end(),
						[](const contrib::chainHull::Point& a, const contrib::chainHull::Point &b)
					{ if (a.x != b.x) return a.x < b.x; return a.y < b.y; });
				}

				HIDDEN_SDBR void getHull(std::vector<contrib::chainHull::Point> &in, std::vector<contrib::chainHull::Point> &out) {
					out.clear();
					out.resize(in.size());
					int n = contrib::chainHull::chainHull_2D(in.data(), (int)in.size(), out.data());
					out.resize((size_t)n);
				}
			}

			shape_ptr projectShape(shape_ptr p, int axis) {
				auto res = p->clone();

				std::vector<contrib::chainHull::Point> projPts;
				backend::convertShapeTo2DpointArray(p, axis, projPts);
				// ProjPts is already sorted, first by X, then by Y.
				// I can easily drop points if already sorted!
				shapePointsOnly_t finalPts;
				finalPts.resize((int)(projPts.size()), 3);
				finalPts.block(0, 2, finalPts.rows(), 1).setZero();
				int numPts = 0;
				contrib::chainHull::Point *lastPt = nullptr;
				for (size_t i = 0; i < projPts.size(); ++i) {
					if (lastPt) {
						if ((lastPt->x = projPts[i].x) && (lastPt->y == projPts[i].y)) continue;
					}
					finalPts(numPts, 0) = projPts[i].x;
					finalPts(numPts, 1) = projPts[i].y;

					numPts++;
					lastPt = &(projPts[i]);
				}
				finalPts.conservativeResize(numPts, 3);

				res->setPoints(finalPts);

				SDBR_log("shapeAlgorithms", logging::INFO, "Shape projection of " << p->hash()->lower
					<< " projected " << p->numPoints() << " points to " << numPts << " in axis " << axis << ".");
				return res;
			}

			void getProjectedStats(shape_ptr p, int axis, double dSpacingM,
				float& maxProjectedDimension_m, float& projectedArea_m2, float& circAreaFrac_dimensionless) {
				auto sProj = projectShape(p, axis);

				maxProjectedDimension_m = -1;
				projectedArea_m2 = -1;
				circAreaFrac_dimensionless = -1;

				std::vector<contrib::chainHull::Point> ptArrayIn, ptArrayCvx;
				backend::convertShapeTo2DpointArray(sProj, axis, ptArrayIn);
				backend::getHull(ptArrayIn, ptArrayCvx);

				for (size_t i = 0; i < ptArrayCvx.size(); ++i) {
					for (size_t j = i + 1; j < ptArrayCvx.size(); ++j) {
						float mdcsq = std::pow(ptArrayCvx[i].x - ptArrayCvx[j].x, 2.f)
							+ std::pow(ptArrayCvx[i].y - ptArrayCvx[j].y, 2.f);
						float mdc = (float)(std::sqrt(mdcsq) * dSpacingM);
						if (maxProjectedDimension_m < mdc) maxProjectedDimension_m = mdc;
					}
				}

				projectedArea_m2 = (float)sProj->numPoints() * (float) (dSpacingM * dSpacingM);
				float circArea = 3.141592654f * maxProjectedDimension_m * maxProjectedDimension_m / 4.f;
				circAreaFrac_dimensionless = projectedArea_m2 / circArea;
				SDBR_log("shapeAlgorithms", logging::INFO,
					"Shape projection of " << p->hash()->lower
					<< " to axis " << axis << 
					" had " << ptArrayIn.size() << " unique points, and the"
					" 2D convex hull had " << ptArrayCvx.size() << " points. "
					<< "maxProjDim_m = " << maxProjectedDimension_m
					<< ", projectedArea_m2 = " << projectedArea_m2
					<< ", circArea = " << circArea
					<< ", circAreaFrac_dimensionless = " << circAreaFrac_dimensionless
				);

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
				float& mean_maxProjectedDimension_m, float& mean_projectedArea_m2,
				float& mean_circAreaFrac_dimensionless, float &mass_Kg, float &v_mps,
				float &volumeM, float &reffM) {
				Eigen::Array3f mpd, mpa, caf;
				mpd.setZero();
				mpa.setZero();
				caf.setZero();
				// Convert volume into united quantity, and determine mass.
				std::shared_ptr<scatdb::units::converter> cnv = std::shared_ptr<scatdb::units::converter>(new scatdb::units::converter(dSpacingUnits, "m"));
				if (!cnv->isValid()) SDBR_throw(error::error_types::xBadInput)
					.add<std::string>("Reason", "Dipole spacing units must be units of length. Either the units are wrong, or this conversion is unsupported.")
					.add<std::string>("dSpacingUnits", dSpacingUnits);
				double dSpacingM = cnv->convert(dSpacing);
				volumeM = std::pow((float)dSpacingM, 3.f) * (float)p->numPoints();
				reffM = std::pow(3.f*volumeM / (4.f*3.141592654f), 1.f / 3.f);
				float denKgM = 916; // density of ice in kg / m^3
				mass_Kg = denKgM * volumeM;

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
