#pragma once
#include "defs.hpp"
#include <complex>
#include <functional>

namespace Ryan_Scat {
	namespace refract {

		/// The raw dielectric providers implementations
		namespace implementations {
			//Ryan_Scat_DL void mWater(double f, double t, std::complex<double> &m, const char* provider = nullptr);
			//Ryan_Scat_DL void mIce(double f, double t, std::complex<double> &m, const char* provider = nullptr);
			//Ryan_Scat_DL void mOther(double f, double t, std::complex<double> &m, const char* provider = nullptr);
			/// Water complex refractive index for microwave for 0 to 1000 GHz
			/// Liebe, Hufford and Manabe (1991)
			Ryan_Scat_DL void mWaterLiebe(double f, double t, std::complex<double> &m);
			/// Water complex refractive index for microwave for 0 to 500 GHz, temps from -20 to 40 C.
			/// This one is for pure water (salinity = 0). There is also a model with salinity (TBI).
			/// Meissner and Wentz (2004)
			Ryan_Scat_DL void mWaterFreshMeissnerWentz(double f, double t, std::complex<double> &m);
			/// Ice complex refractive index
			/// Christian Matzler (2006)
			Ryan_Scat_DL void mIceMatzler(double f, double t, std::complex<double> &m);
			/// Ice complex refractive index for microwave/uv
			Ryan_Scat_DL void mIceWarren(double f, double t, std::complex<double> &m);
			/// Water complex refractive index for ir/vis
			Ryan_Scat_DL void mWaterHanel(double lambda, std::complex<double> &m);
			/// Ice complex refractive index for ir/vis
			Ryan_Scat_DL void mIceHanel(double lambda, std::complex<double> &m);
			/// Sodium chloride refractive index for ir/vis
			Ryan_Scat_DL void mNaClHanel(double lambda, std::complex<double> &m);
			/// Sea salt refractive index for ir/vis
			Ryan_Scat_DL void mSeaSaltHanel(double lambda, std::complex<double> &m);
			/// Dust-like particle refractive index for ir/vis
			Ryan_Scat_DL void mDustHanel(double lambda, std::complex<double> &m);
			/// Sand O-ray refractvie index for ir/vis (birefringent)
			Ryan_Scat_DL void mSandOHanel(double lambda, std::complex<double> &m);
			/// Sand E-ray refractive index for ir/vis (birefringent)
			Ryan_Scat_DL void mSandEHanel(double lambda, std::complex<double> &m);
		}

		/// m to e converters
		Ryan_Scat_DL std::complex<double> mToE(std::complex<double> m);
		Ryan_Scat_DL void mToE(std::complex<double> m, std::complex<double> &e);
		Ryan_Scat_DL std::complex<double> eToM(std::complex<double> e);
		Ryan_Scat_DL void eToM(std::complex<double> e, std::complex<double> &m);

		// Temperature-guessing
		double Ryan_Scat_DL guessTemp(double freq, const std::complex<double> &mToEval,
			std::function<void(double freq, double temp, std::complex<double>& mres)> meth
			= Ryan_Scat::refract::implementations::mIceMatzler,
			double tempGuessA_K = 263, double tempGuessB_K = 233);

	}
}
