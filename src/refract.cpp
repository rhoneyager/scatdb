// Code segment directly based on Liu's code. Rewritten in C++ so that it may be 
// compiled in an MSVC environment
#define _SCL_SECURE_NO_WARNINGS // Issue with linterp
#pragma warning( disable : 4244 ) // warnings C4244 and C4267: size_t to int and int <-> _int64
#pragma warning( disable : 4267 )
#include <cmath>
#include <complex>
#include <fstream>
#include <valarray>
#include <mutex>
#include <boost/shared_ptr.hpp>
#include <boost/tokenizer.hpp>
#include "../scatdb/refract/refract.hpp"
#include "../scatdb/refract/refractBase.hpp"
#include "../scatdb/zeros.hpp"
#include "../scatdb/units/units.hpp"
//#include "../private/linterp.h"
#include "../scatdb/error.hpp"
#include "../scatdb/logging.hpp"

namespace scatdb {
	namespace refract {
		namespace implementations {
			std::mutex m_refracts;
			all_providers_mp allProvidersSet;
			std::map<std::string, all_providers_mp> providersSet;
			std::map<std::string, provider_mp> providersByName;
			void _init() {
				std::lock_guard<std::mutex> lock(m_refracts);
				static bool inited = false;
				if (inited) return;
				allProvidersSet = all_providers_mp(new provider_collection_type);

				std::shared_ptr<std::map<std::string, provider_p> > data(
					new std::map<std::string, provider_p>);

				auto pmWaterLiebe = provider_s::generate(
					"mWaterLiebe", "water", 
					"Liebe, H.J., Hufford, G.A. & Manabe, T. Int J Infrared Milli Waves (1991) 12: 659. doi:10.1007/BF01008897",
					"",
					provider_s::spt::FREQTEMP, (void*)mWaterLiebe)
					->addReq("spec", "GHz", 0, 1000)->addReq("temp", "K", 273.15, 373.15)->registerFunc();
				auto pmWaterFreshMeissnerWentz = provider_s::generate(
					"mWaterFreshMeissnerWentz", "water", 
					"T. Meissner and F. J. Wentz, \"The complex dielectric constant of pure and sea water from microwave satellite observations\", IEEE Trans. Geosci. Remote Sensing, vol. 42, no.9, pp. 1836-1849, September 2004.",
					"For pure water (no salt)",
					provider_s::spt::FREQTEMP, (void*)mWaterFreshMeissnerWentz)
					->addReq("spec", "GHz", 0, 500)->addReq("temp", "K", 253.15, 313.15)->registerFunc();
				auto pmIceMatzler = provider_s::generate(
					"mIceMatzler", "ice", 
					"Thermal Microwave Radiation: Applications for Remote Sensing, "
					"Chapter 5, Microwave dielectric properties of ice, "
					"By Christian Mätzler(2006)",
					"",
					provider_s::spt::FREQTEMP, (void*)mIceMatzler)
					->addReq("spec", "GHz", 0, 1000)->addReq("temp", "K", 0, 273.15)->registerFunc();
				auto pmIceWarren = provider_s::generate(
					"mIceWarren", "ice", 
					"Stephen G. Warren, \"Optical constants of ice from the ultraviolet to the microwave, \" Appl. Opt. 23, 1206-1225 (1984)",
					"",
					provider_s::spt::FREQTEMP, (void*)mIceWarren)
					->addReq("spec", "GHz", 0.167, 8600)->addReq("temp", "degC", -60, -1)->registerFunc();
				auto pmWaterHanel = provider_s::generate(
					"mWaterHanel", "water", 
					"Tables from Thomas Hanel. Not sure which paper.", "",
					provider_s::spt::FREQ, (void*)mWaterHanel)
					->addReq("spec", "um", 0.2, 30000)->registerFunc(100);
				auto pmIceHanel = provider_s::generate(
					"mIceHanel", "water", 
					"Tables from Thomas Hanel. Not sure which paper.", "",
					provider_s::spt::FREQ, (void*)mIceHanel)
					->addReq("spec", "um", 0.2, 30000)->registerFunc(100);

				auto pmNaClHanel = provider_s::generate(
					"mNaClHanel", "NaCl", 
					"Tables from Thomas Hanel. Not sure which paper.", "",
					provider_s::spt::FREQ, (void*)mNaClHanel)
					->addReq("spec", "um", 0.2, 30000)->registerFunc(100);
				auto pmSeaSaltHanel = provider_s::generate(
					"mSeaSaltHanel", "SeaSalt", 
					"Tables from Thomas Hanel. Not sure which paper.", "",
					provider_s::spt::FREQ, (void*)mSeaSaltHanel)
					->addReq("spec", "um", 0.2, 30000)->registerFunc(100);
				auto pmDustHanel = provider_s::generate(
					"mDustHanel", "Dust", 
					"Tables from Thomas Hanel. Not sure which paper.", "",
					provider_s::spt::FREQ, (void*)mDustHanel)
					->addReq("spec", "um", 0.2, 300)->registerFunc(100);
				auto pmSandOHanel = provider_s::generate(
					"mSandOHanel", "Sand_O", 
					"Tables from Thomas Hanel. Not sure which paper.", "",
					provider_s::spt::FREQ, (void*)mSandOHanel)
					->addReq("spec", "um", 0.2, 300)->registerFunc(100);
				auto pmSandEHanel = provider_s::generate(
					"mSandEHanel", "Sand_E", 
					"Tables from Thomas Hanel. Not sure which paper.", "",
					provider_s::spt::FREQ, (void*)mSandEHanel)
					->addReq("spec", "um", 0.2, 300)->registerFunc(100);

				inited = true;
			}
		}

		provider_mp provider_s::generate(
			const std::string &name, const std::string &subst,
			const std::string &source, const std::string &notes,
			enum class provider_s::spt sv, void* ptr) {
			provider_mp res(new provider_s);
			res->name = name;
			res->substance = subst;
			res->source = source;
			res->notes = notes;
			res->speciality_function_type = sv;
			res->specialty_pointer = ptr;
			return res;
		}
		provider_mp provider_s::addReq(const std::string &name, const std::string &units,
			double low, double high) {
			auto res = this->shared_from_this();
			auto newreq = requirement_s::generate(name, units, low, high);
			reqs[name] = newreq;
			return res;
		}
		provider_mp provider_s::registerFunc(int priority) {
			provider_mp res = this->shared_from_this();
			all_providers_mp block;
			if (implementations::providersSet.count(substance))
				block = implementations::providersSet.at(substance);
			else {
				block = all_providers_mp(new provider_collection_type);
				implementations::providersSet[substance] = block;
			}
			block->insert(std::pair<int,provider_mp>(priority,res));
			implementations::allProvidersSet->insert(std::pair<int, provider_mp>(priority, res));
			implementations::providersByName[res->name] = res;
			return res;
		}
		provider_s::provider_s() {}
		provider_s::~provider_s() {}
		requirement_p requirement_s::generate(
			const std::string &name, const std::string& units,
			double low, double high) {
			std::shared_ptr<requirement_s> res(new requirement_s);
			res->parameterName = name;
			res->parameterUnits = units;
			res->hasValidRange = true;
			res->validRange = std::pair<double, double>(low, high);
			return res;
		}

		all_providers_p listAllProviders() {
			return implementations::allProvidersSet;
		}
		all_providers_p listAllProviders(const std::string &subst) {
			all_providers_p emptyres;
			if (!implementations::providersSet.count(subst)) return emptyres;
			all_providers_p pss = implementations::providersSet.at(subst);
			return pss;
		}
		void enumProvider(provider_p p, std::ostream &out) {
			out << "Provider:\t" << p->name << "\n"
				<< "\tSubstance:\t" << p->substance << "\n"
				<< "\tSource:\t" << p->source << "\n"
				<< "\tNotes:\t" << p->notes << "\n";
			if (p->reqs.size()) {
				out << "\tRequirements:\n";
				for (const auto &r : p->reqs) {
					out << "\t\tParameter:\t" << r.first
						<< "\t\t\tRange:\t" << r.second->validRange.first << " - "
						<< r.second->validRange.second << " "
						<< r.second->parameterUnits << "\n";
				}
			}
		}
		void enumProviders(all_providers_p p, std::ostream &out) {
			for (const auto &i : *(p.get())) {
				enumProvider(i.second, out);
			}
		}

		provider_p findProvider(const std::string &subst,
			bool haveFreq, bool haveTemp, const std::string & start) {
			provider_p emptyres;
			if (implementations::allProvidersSet->size() == 0) implementations::_init();

			if (implementations::providersByName.count(subst)) {
				provider_p cres = implementations::providersByName.at(subst);
				if ((cres->reqs.count("spec") && !haveFreq) || (cres->reqs.count("temp") && !haveTemp)) {
					SDBR_throw(scatdb::error::error_types::xBadFunctionMap)
						.add<std::string>("Reason", "Attempting to find the provider for a manually-"
							"specified refractive index formula, but the retrieved formula's "
							"requirements do not match what whas passed.")
						.add<std::string>("Provider", subst);
				}
				return cres;
			}

			if (!implementations::providersSet.count(subst)) return emptyres;
			all_providers_mp pss = implementations::providersSet.at(subst);
			bool startSearch = false;
			if (start == "") startSearch = true;
			for (const auto &p : *(pss.get())) {
				if (!startSearch) {
					if (p.second->name == start) startSearch = true;
					continue;
				} else {
					if (p.second->reqs.count("spec") && !haveFreq) continue;
					if (p.second->reqs.count("temp") && !haveTemp) continue;
				}
				return p.second;
			}
			return emptyres;
		}
		all_providers_p findProviders(const std::string &subst,
			bool haveFreq, bool haveTemp) {
			all_providers_mp res(new provider_collection_type);

			if (implementations::allProvidersSet->size() == 0) implementations::_init();
			if (!implementations::providersSet.count(subst)) return res;
			all_providers_p pss = implementations::providersSet.at(subst);
			for (const auto &p : *(pss.get())) {
				if (p.second->reqs.count("spec") && !haveFreq) continue;
				if (p.second->reqs.count("temp") && !haveTemp) continue;
				res->insert(p);
			}
			return res;
		}
		void prepRefract(provider_p prov, const std::string &inFreqUnits,
			refractFunction_freqonly_t& res) {
			/** Translation function exists to ensure that the units are passed as expected. **/
			auto compatFunc = [](
				units::converter_p converterFreq,
				void* innerFunc,
				double inSpec,
				std::complex<double> &m) -> void {
				// It's an ugly cast...
				void(*transFunc)(double, std::complex<double>&) = (void(*)(double,std::complex<double>&))innerFunc;
				if (!converterFreq)
					transFunc(inSpec, m);
				else {
					transFunc(converterFreq->convert(inSpec), m);
				}
			};
			units::converter_p converter;
			if (prov->speciality_function_type != provider_s::spt::FREQ)
				SDBR_throw(scatdb::error::error_types::xBadInput)
				.add<std::string>("Reason", "You called the wrong prepRefract.");
			if (!prov->reqs.count("spec"))
				SDBR_throw(scatdb::error::error_types::xBadFunctionMap)
				.add<std::string>("Reason", "Attempting to prepare a refractive index formula that does not "
					"require a frequency / wavelength / wavenumber, but one was provided.");
			std::string reqUnits = prov->reqs.at("spec")->parameterUnits;
			if (reqUnits != inFreqUnits)
				converter = units::conv_spec::generate(inFreqUnits, reqUnits);

			res = std::bind(compatFunc,
				converter,
				prov->specialty_pointer,
				std::placeholders::_1,
				std::placeholders::_2);
		}
		void prepRefract(provider_p prov,
			const std::string &inFreqUnits, const std::string &inTempUnits,
			refractFunction_freq_temp_t& res) {
			/** Translation function exists to ensure that the units are passed as expected. **/
			auto compatFunc = [](
				units::converter_p converterFreq,
				units::converter_p converterTemp,
				void* innerFunc,
				double inSpec,
				double inTemp,
				std::complex<double> &m) -> void {
				// It's an ugly cast...
				void(*transFunc)(double, double, std::complex<double>&) = (void(*)(double, double, std::complex<double>&))innerFunc;
				if (!converterFreq && !converterTemp)
					transFunc(inSpec, inTemp, m);
				else if (converterFreq && !converterTemp)
					transFunc(converterFreq->convert(inSpec), inTemp, m);
				else if (!converterFreq && converterTemp)
					transFunc(inSpec, converterTemp->convert(inTemp), m);
				else transFunc(converterFreq->convert(inSpec), converterTemp->convert(inTemp), m);
			};
			units::converter_p converterFreq, converterTemp;
			if (prov->speciality_function_type != provider_s::spt::FREQTEMP)
				SDBR_throw(scatdb::error::error_types::xBadInput)
				.add<std::string>("Reason", "You called the wrong prepRefract.");
			if (!prov->reqs.count("spec"))
				SDBR_throw(scatdb::error::error_types::xBadFunctionMap)
				.add<std::string>("Reason", "Attempting to prepare a refractive index formula that does not "
					"require a frequency / wavelength / wavenumber, but one was provided.");
			std::string reqFreqUnits = prov->reqs.at("spec")->parameterUnits;
			if (reqFreqUnits != inFreqUnits)
				converterFreq = units::conv_spec::generate(inFreqUnits, reqFreqUnits);

			if (!prov->reqs.count("temp"))
				SDBR_throw(scatdb::error::error_types::xBadFunctionMap)
				.add<std::string>("Reason", "Attempting to prepare a refractive index formula that does not "
					"require a temperature, but one was provided.");
			std::string reqTempUnits = prov->reqs.at("temp")->parameterUnits;
			if (reqTempUnits != inTempUnits)
				converterTemp = units::converter::generate(inTempUnits, reqTempUnits);

			res = std::bind(compatFunc,
				converterFreq,
				converterTemp,
				prov->specialty_pointer,
				std::placeholders::_1,
				std::placeholders::_2,
				std::placeholders::_3);
		}


		void bruggeman(std::complex<double> Ma, std::complex<double> Mb,
			double fa, std::complex<double> &Mres)
		{
			using namespace std;
			complex<double> eA, eB, eRes;
			mToE(Ma, eA);
			mToE(Mb, eB);

			// Liu's formula is derived only for mAmbient = 1. I want a more general case to make 
			// dielectric chaining easier.
			auto formula = [&](std::complex<double> x) -> std::complex<double>
			{
				// f (( eA - eE ) / (eA + 2eE) ) + (1-f) (( eB - eE ) / (eB + 2eE) ) = 0
				using namespace std;
				complex<double> res, pa, pb;
				pa = complex<double>(fa, 0) * (eA - x) / (eA + (complex<double>(2., 0)*x));
				pb = complex<double>(1. - fa, 0) * (eB - x) / (eB + (complex<double>(2., 0)*x));
				res = pa + pb;
				return res;
			};

			complex<double> gA(1.0, 1.0), gB(1.55, 1.45);
			eRes = zeros::secantMethod(formula, gA, gB);

			eToM(eRes, Mres);
			SDBR_log("refract", scatdb::logging::DEBUG_2,
				"bruggeman code called\n\tMa " << Ma << "\n\tMb " << Mb << "\n\tfa " << fa
				<< "\n\tMres " << Mres);
		}

		void debyeDry(std::complex<double> Ma, std::complex<double> Mb,
			double fa, std::complex<double> &Mres)
		{
			using namespace std;
			std::complex<double> eA, eB, eRes;
			mToE(Ma, eA);
			mToE(Mb, eB);

			complex<double> pa = fa * (eA - complex<double>(1., 0)) / (eA + complex<double>(2., 0));
			complex<double> pb = (1. - fa) * (eB - complex<double>(1., 0)) / (eB + complex<double>(2., 0));

			complex<double> fact = pa + pb;
			eRes = (complex<double>(2., 0) * fact + complex<double>(1., 0))
				/ (complex<double>(1., 0) - fact);
			eToM(eRes, Mres);
			SDBR_log("refract", scatdb::logging::DEBUG_2,
				"debyeDry code called\n\tMa " << Ma << "\n\tMb " << Mb << "\n\tfa " << fa
				<< "\n\tMres " << Mres);
		}

		void maxwellGarnettSpheres(std::complex<double> Ma, std::complex<double> Mb,
			double fa, std::complex<double> &Mres)
		{
			using namespace std;
			std::complex<double> eA, eB, eRes;
			mToE(Ma, eA);
			mToE(Mb, eB);

			// Formula is:
			// (e_eff - eb)/(e_eff+2eb) = fa * (ea - eb) / (ea + 2 eb)
			complex<double> a = complex<double>(fa, 0) * (eA - eB) / (eA + (complex<double>(2, 0) * eB));
			eRes = eB * (complex<double>(2, 0) * a + complex<double>(1, 0))
				/ (complex<double>(1, 0) - a);
			eToM(eRes, Mres);
			SDBR_log("refract", scatdb::logging::DEBUG_2,
				"maxwellGarnettSpheres code called\n\tMa " << Ma << "\n\tMb " << Mb << "\n\tfa " << fa
				<< "\n\tMres " << Mres);
		}

		void maxwellGarnettEllipsoids(std::complex<double> Ma, std::complex<double> Mb,
			double fa, std::complex<double> &Mres)
		{
			using namespace std;
			std::complex<double> eA, eB, eRes;
			mToE(Ma, eA);
			mToE(Mb, eB);

			complex<double> betaA = complex<double>(2., 0)*eA / (eB - eA);
			complex<double> betaB = ((eB / (eB - eA)) * log(eB / eA)) - complex<double>(1., 0);
			complex<double> beta = betaA * betaB;

			complex<double> cf(fa, 0), cfc(1. - fa, 0);

			eRes = ((cfc*beta) + (cf*eA)) / (cf + (cfc*beta));
			eToM(eRes, Mres);
			SDBR_log("refract", scatdb::logging::DEBUG_2, 
				"maxwellGarnettEllipsoids code called\n\tMa " << Ma << "\n\tMb " << Mb << "\n\tfa " << fa
				<< "\n\tMres " << Mres);
		}

		void sihvola(std::complex<double> Ma, std::complex<double> Mb,
			double fa, double nu, std::complex<double> &Mres)
		{
			using namespace std;
			std::complex<double> eA, eB, eRes;
			mToE(Ma, eA);
			mToE(Mb, eB);

			/** Formula is:
			* (e_eff - eb) / (e_eff + 2eb + v(e_eff - eb)) - fa (ea-eb)/(ea+2eb+v(e_eff-eb) = 0
			* This formula has no analytic solution. It is also complex-valued. Its derivative is hard to calculate.
			* Because of this, I will be using the complex secant method to find the zeros.
			* This is also why the other mixing formulas do not call this code.
			**/

			auto formulaSihvola = [&](std::complex<double> x) -> std::complex<double>
			{
				using namespace std;
				complex<double> res, pa, pb;
				pa = (x - eB) / (x + (complex<double>(2.0, 0)*eB) + (complex<double>(nu, 0)*(x - eB)));
				pb = complex<double>(fa, 0) *
					(eA - eB) / (eA + (complex<double>(2.0, 0)*eB) + (complex<double>(nu, 0)*(x - eB)));
				res = pa - pb;
				return res;
			};

			complex<double> gA(1.0, 1.0), gB(1.55, 1.45);
			eRes = zeros::secantMethod(formulaSihvola, gA, gB);
			eToM(eRes, Mres);
			SDBR_log("refract", scatdb::logging::DEBUG_2, "sihvola code called\n\tMa " << Ma << "\n\tMb " << Mb << "\n\tfa " << fa
				<< "\n\tnu " << nu << "\n\tMres " << Mres);
		}

		std::complex<double> mToE(std::complex<double> m)
		{
			return m * m;
		}
		void mToE(std::complex<double> m, std::complex<double> &e)
		{
			e = mToE(m);
		}

		std::complex<double> eToM(std::complex<double> e)
		{
			return sqrt(e);
		}
		void eToM(std::complex<double> e, std::complex<double> &m)
		{
			m = eToM(e);
		}

		double guessTemp(double freq, const std::complex<double>& m,
			std::function<void(double freq, double temp, std::complex<double>& mres)> meth,
			double TA, double TB)
		{
			using namespace std;
			// Attempt to guess the formula using the secant method.
			auto formulaTemp = [&](double T) -> double
			{
				// 0 = mRes(f,T) - m_known
				using namespace std;
				complex<double> mRes;
				meth(freq, T, mRes);

				double mNorm = mRes.real() - m.real(); //norm(mRes- ms.at(0));

				return mNorm;
			};

			double temp = 0;
			//complex<double> gA, gB;
			//refract::mIce(freq, TA, gA);
			//refract::mIce(freq, TB, gB);
			temp = zeros::secantMethod(formulaTemp, TA, TB, 0.00001);
			return temp;
		}

		/*
		void MultiInclusions(
			const std::vector<basicDielectricTransform> &funcs,
			const std::vector<double> &fs,
			const std::vector<std::complex<double> > &ms,
			std::complex<double> &Mres)
		{
			//if (fs.size() != ms.size() + 1) RDthrow Ryan_Debug::error::xBadInput("Array sizes are not the same");
			//if (fs.size() != funcs.size()) RDthrow Ryan_Debug::error::xBadInput("Array sizes are not the same");

			using namespace std;

			double fTot = 0.0;
			double fEnv = 1.0;

			Mres = ms.at(0);
			fTot = fs.at(0);

			// Ordering is most to least enclosed
			for (size_t i = 1; i < funcs.size(); ++i)
			{
				// ms[0] is the initial refractive index (innermost material), and fs[0] is its volume fraction
				complex<double> mA = Mres;
				fTot += fs.at(i);
				if (!fTot) continue;
				funcs.at(i)(mA, ms.at(i), fs.at(i - 1) / fTot, Mres);
			}
		}
		*/

		/*
		void scatdb::refract::maxwellGarnett(std::complex<double> Mice, std::complex<double> Mwater,
		std::complex<double> Mair, double fIce, double fWater, std::complex<double> &Mres)
		{
		using namespace std;
		std::complex<double> Miw;

		// Ice is the inclusion in water, which is the inclusion in air
		double frac = 0;
		if (fWater+fIce == 0) frac = 0;
		else frac = fIce / (fWater + fIce);
		maxwellGarnettSpheres(Mice, Mwater, frac, Miw);
		maxwellGarnettSpheres(Miw, Mair, fIce + fWater, Mres);
		}
		*/
	}
}
