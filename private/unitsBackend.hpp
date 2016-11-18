#pragma once
#include "../Ryan_scat/defs.hpp"
#include <string>
#include <memory>
#include "../Ryan_scat/registry.hpp"
#include "../Ryan_Scat/units/units.hpp"
#include "../Ryan_Scat/units/unitsPlugins.hpp"
namespace Ryan_Scat {
	namespace units {
		namespace implementations {
			INTERNAL_TAG void _init();
			INTERNAL_TAG void _registerBackend(const std::string &inUnits, const std::string &outUnits,
				const std::string &family, converter_p);
			INTERNAL_TAG converter_p _queryBackend(const std::string &inUnits, const std::string &outUnits,
				const std::string &family);
			struct INTERNAL_TAG simpleUnits : public Unithandler {
				static bool canConvert(Converter_registry_provider::optsType opts);
				static std::shared_ptr<const Unithandler> constructConverter(
					Converter_registry_provider::optsType opts);
				simpleUnits(const std::string &in, const std::string &out, bool init = true);
				bool validLength(const std::string &_inUnits, const std::string &_outUnits);
				bool validFreq(const std::string &_inUnits, const std::string &_outUnits);
				bool validVol(const std::string &_inUnits, const std::string &_outUnits);
				bool validPres(const std::string &in, const std::string &out);
				bool validMass(const std::string &in, const std::string &out);
				bool validTemp(const std::string &in, const std::string &out);
				bool validDens(const std::string &in, const std::string &out);
				virtual ~simpleUnits();
				double _inOffset, _outOffset, _convFactor;
				std::string _inUnits, _outUnits;
				bool _valid;
				virtual bool isValid() const;
				double convert(double inVal) const;
			};

			struct INTERNAL_TAG spectralUnits : public implementations::Unithandler {
				static bool canConvert(Converter_registry_provider::optsType opts);
				static std::shared_ptr<const implementations::Unithandler> constructConverter(
					Converter_registry_provider::optsType opts);
				spectralUnits(const std::string &in, const std::string &out);
				virtual ~spectralUnits();
				std::string _inUnits, _outUnits;
				bool _valid, _Iin, _Iout;
				std::shared_ptr<const implementations::Unithandler> hIn, hOut;
				virtual bool isValid() const;
				virtual double convert(double input) const;
			};
		}
	}
}
