#pragma once
#include "../defs.hpp"
#include <string>
#include <memory>
#include "../registry.hpp"
namespace Ryan_Scat {
	namespace units {
		class converter;
		namespace implementations {
			struct Unithandler;
			class converter_provider_registry {};
			class Ryan_Scat_DL Converter_registry_provider
			{
			public:
				Converter_registry_provider();
				~Converter_registry_provider();
				typedef const std::shared_ptr<const Ryan_Scat::registry::options> optsType;
				typedef std::function<bool(optsType)> canConvertType;
				canConvertType canConvert;
				typedef std::function<std::shared_ptr<const Unithandler>(optsType)> constructType;
				constructType constructConverter;
				const char* name;
			};
		}
	}
	namespace registry {
		extern template class usesDLLregistry <
			::Ryan_Scat::units::implementations::converter_provider_registry,
			::Ryan_Scat::units::implementations::Converter_registry_provider >;
	}
	/** \brief Provides convenient runtime conversion functions for converting
	* different units.
	*
	* This includes interconversions between prefixes and conversions
	* to different unit types. Complex unit systems may also be introduced, allowing for
	* calculation of conversion factors in equations. The static units are based on the
	* boost units library, and the other stuff is taken from my head.
	*
	* Used in atmos class for profile reading and interconversion. Used in lbl to ensure
	* correct dimensionality of functions.
	**/
	namespace units {
		namespace implementations {
			/// Opaque object provided to perform unit manipulations.
			struct Ryan_Scat_DL Unithandler : public Ryan_Scat::registry::handler_external
			{
			protected:
				Unithandler(const char* id);
			public:
				virtual ~Unithandler() {}
				virtual double convert(double input) const = 0;
				virtual bool isValid() const = 0;
				//template <typename T>
				//T convert(T input) const;
			};
		}
		namespace plugins {
			class Ryan_Scat_DL unitsPlugins :
				virtual public std::enable_shared_from_this<unitsPlugins>,
				virtual public registry::usesDLLregistry<
				::Ryan_Scat::units::implementations::converter_provider_registry,
				::Ryan_Scat::units::implementations::Converter_registry_provider >
			{
				unitsPlugins();
				virtual ~unitsPlugins();
			public:
				//static std::shared_ptr<unitsPlugins> generate();
			};
		}
	}
}



