#pragma once
#include "defs.hpp"
#pragma warning( disable : 4003 ) // Bug in boost with VS2016.3
#include <complex>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include <boost/parameter/keyword.hpp>
#include <boost/parameter/name.hpp>
#include <boost/parameter/preprocessor.hpp>
#include <boost/lexical_cast.hpp>
#include "units/units.hpp"
#include "error.hpp"
#include "logging.hpp"
#include "refractBase.hpp"
#include "optionsForwards.hpp"

namespace Ryan_Scat {
	namespace refract {
		struct requirement_s;
		typedef std::shared_ptr<const requirement_s> requirement_p;
		struct requirement_s {
			std::string parameterName;
			std::string parameterUnits;
			bool hasValidRange;
			std::pair<double, double> validRange;
			static requirement_p generate(
				const std::string &name, const std::string& units,
				double low, double high);
		};
		struct provider_s;
		typedef std::shared_ptr<provider_s> provider_mp;
		typedef std::shared_ptr<const provider_s> provider_p;
		struct provider_s : public std::enable_shared_from_this<provider_s> {
		private:
			provider_s();
		public:
			virtual ~provider_s();
			std::string name;
			std::string substance;
			std::map<std::string, requirement_p> reqs;
			provider_mp addReq(const std::string &name, const std::string &units,
				double low, double high);
			provider_mp registerFunc();
			enum class spt {
				NONE,
				FREQ,
				FREQTEMP,
				OTHER
			} speciality_function_type;
			void* specialty_pointer;
			static provider_mp generate(const std::string &name, const std::string &subst,
				enum class provider_s::spt sv, void* ptr);
		};
		typedef std::shared_ptr<const std::vector<provider_p> > all_providers_p;
		typedef std::shared_ptr<std::vector<provider_p> > all_providers_mp;

		typedef std::function<void(double, std::complex<double>&)> refractFunction_freqonly_t;
		typedef std::function<void(double, double, std::complex<double>&)> refractFunction_freq_temp_t;

		Ryan_Scat_DL provider_p findProvider(const std::string &subst,
			bool haveFreq = true, bool haveTemp = true);

		Ryan_Scat_DL void prepRefract(provider_p, const std::string &inFreqUnits,
			refractFunction_freqonly_t&);
		Ryan_Scat_DL void prepRefract(provider_p,
			const std::string &inFreqUnits, const std::string &inTempUnits,
			refractFunction_freq_temp_t&);
		

	}
}
