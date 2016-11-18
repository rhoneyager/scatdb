#pragma once
#pragma warning( disable : 4251 ) // DLL interface of private object

#include "../defs.hpp"
#include <string>
#include <memory>
namespace Ryan_Scat {
	/** \brief Provides convenient runtime conversion functions for converting
	 * different units. 
	 *
	 * This includes interconversions between prefixes and conversions
	 * to different unit types. Complex unit systems may also be introduced, allowing for
	 * calculation of conversion factors in equations.
	 **/
	namespace units {
		namespace implementations { struct Unithandler; }
		typedef std::shared_ptr<const implementations::Unithandler> Unithandler_p;
		class converter;
		typedef std::shared_ptr<const converter> converter_p;
		/** \brief Base conversion class
		*
		* Class is virtual. May be overridden with classes that do formulaic operations,
		* such as converters to density in ppmv.
		*
		* Now, with the appropriate DLL loaded, the udunits system will be used for most conversions.
		* The derived classes still have a bit of code for when udunits is not installed.
		**/
		class Ryan_Scat_DL converter
		{
		public:
			virtual ~converter();
			virtual double convert(double inVal) const;
			static bool canConvert(const std::string &inUnits, const std::string &outUnits);
			static Unithandler_p getConverter(
				const std::string &inUnits, const std::string &outUnits);
			converter(const std::string &inUnits, const std::string &outUnits);
			static std::shared_ptr<const converter> generate(
				const std::string &inUnits, const std::string& outUnits);
			bool isValid() const;
		protected:
			converter();
			Unithandler_p h;
		};

		/// \brief Perform interconversions between frequency, wavelength and wavenumber
		/// (GHz, Hz, m, cm, um, cm^-1, m^-1)
		class Ryan_Scat_DL conv_spec : public converter
		{
		public:
			conv_spec(const std::string &inUnits, const std::string &outUnits);
			static std::shared_ptr<const converter> generate(
				const std::string &inUnits, const std::string& outUnits);
		};
	}
 }



