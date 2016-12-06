#pragma once
#include "../defs.hpp"
#include <complex>
#include <functional>

namespace scatdb {
	namespace refract {

		/// The raw dielectric providers implementations
		namespace implementations {
			//DLEXPORT_SDBR void mWater(double f, double t, std::complex<double> &m, const char* provider = nullptr);
			//DLEXPORT_SDBR void mIce(double f, double t, std::complex<double> &m, const char* provider = nullptr);
			//DLEXPORT_SDBR void mOther(double f, double t, std::complex<double> &m, const char* provider = nullptr);
			/// Water complex refractive index for microwave for 0 to 1000 GHz
			/// Liebe, Hufford and Manabe (1991)
			DLEXPORT_SDBR void mWaterLiebe(double f, double t, std::complex<double> &m);
			/// Water complex refractive index for microwave for 0 to 500 GHz, temps from -20 to 40 C.
			/// This one is for pure water (salinity = 0). There is also a model with salinity (TBI).
			/// Meissner and Wentz (2004)
			DLEXPORT_SDBR void mWaterFreshMeissnerWentz(double f, double t, std::complex<double> &m);
			/// Ice complex refractive index
			/// Christian Matzler (2006)
			DLEXPORT_SDBR void mIceMatzler(double f, double t, std::complex<double> &m);
			/// Ice complex refractive index for microwave/uv
			DLEXPORT_SDBR void mIceWarren(double f, double t, std::complex<double> &m);
			/// Water complex refractive index for ir/vis
			DLEXPORT_SDBR void mWaterHanel(double lambda, std::complex<double> &m);
			/// Ice complex refractive index for ir/vis
			DLEXPORT_SDBR void mIceHanel(double lambda, std::complex<double> &m);
			/// Sodium chloride refractive index for ir/vis
			DLEXPORT_SDBR void mNaClHanel(double lambda, std::complex<double> &m);
			/// Sea salt refractive index for ir/vis
			DLEXPORT_SDBR void mSeaSaltHanel(double lambda, std::complex<double> &m);
			/// Dust-like particle refractive index for ir/vis
			DLEXPORT_SDBR void mDustHanel(double lambda, std::complex<double> &m);
			/// Sand O-ray refractvie index for ir/vis (birefringent)
			DLEXPORT_SDBR void mSandOHanel(double lambda, std::complex<double> &m);
			/// Sand E-ray refractive index for ir/vis (birefringent)
			DLEXPORT_SDBR void mSandEHanel(double lambda, std::complex<double> &m);
		}

		/// m to e converters
		DLEXPORT_SDBR std::complex<double> mToE(std::complex<double> m);
		DLEXPORT_SDBR void mToE(std::complex<double> m, std::complex<double> &e);
		DLEXPORT_SDBR std::complex<double> eToM(std::complex<double> e);
		DLEXPORT_SDBR void eToM(std::complex<double> e, std::complex<double> &m);

		// Temperature-guessing
		double DLEXPORT_SDBR guessTemp(double freq, const std::complex<double> &mToEval,
			std::function<void(double freq, double temp, std::complex<double>& mres)> meth
			= scatdb::refract::implementations::mIceMatzler,
			double tempGuessA_K = 263, double tempGuessB_K = 233);

	}
}
