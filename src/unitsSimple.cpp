#include <string>
#include "../scatdb/units/units.hpp"
#include "../scatdb/units/unitsPlugins.hpp"
#include "../scatdb/error.hpp"
#include "../private/unitsBackend.hpp"
#include "../private/options.hpp"

namespace scatdb {
	namespace units {
		namespace implementations {
			bool simpleUnits::canConvert(Converter_registry_provider::optsType opts) {
				const std::string in = opts->getVal<std::string>("inUnits");
				const std::string out = opts->getVal<std::string>("outUnits");
				simpleUnits test(in, out);
				if (test._valid) return true;
				return false;
			}
			std::shared_ptr<const implementations::Unithandler> simpleUnits::constructConverter(
				Converter_registry_provider::optsType opts) {
				const std::string in = opts->getVal<std::string>("inUnits");
				const std::string out = opts->getVal<std::string>("outUnits");
				std::shared_ptr<simpleUnits> res(new simpleUnits(in, out, true));
				return res;
			}
			simpleUnits::simpleUnits(const std::string &in, const std::string &out, bool init) :
				Unithandler("simple"),
				_inOffset(0), _outOffset(0), _convFactor(1),
				_inUnits(in), _outUnits(out), _valid(true)
			{
				if (!init) return;
				if (validLength(in, out)) return;
				if (validFreq(in, out)) return;
				if (validVol(in, out)) return;
				if (validPres(in, out)) return;
				if (validMass(in, out)) return;
				if (validTemp(in, out)) return;
				if (validDens(in, out)) return;

				_valid = false;
			}
			bool simpleUnits::validLength(const std::string &_inUnits, const std::string &_outUnits) {
				bool inV = false, outV = false;
				_convFactor = 1.f;
				if (_inUnits == "nm") { _convFactor /= 1e9; inV = true; }
				if (_inUnits == "um" || _inUnits == "microns" || _inUnits == "micrometers")
				{
					_convFactor /= 1e6; inV = true;
				}
				if (_inUnits == "mm") { _convFactor /= 1e3; inV = true; }
				if (_inUnits == "cm") { _convFactor *= 0.01; inV = true; }
				if (_inUnits == "km") { _convFactor *= 1000.; inV = true; }
				if (_inUnits == "m") inV = true;
				if (_outUnits == "nm") { _convFactor *= 1e9; outV = true; }
				if (_outUnits == "um" || _outUnits == "microns" || _outUnits == "micrometers")
				{
					_convFactor *= 1e6; outV = true;
				}
				if (_outUnits == "mm") { _convFactor *= 1e3; outV = true; }
				if (_outUnits == "cm") { _convFactor *= 100.; outV = true; }
				if (_outUnits == "km") { _convFactor /= 1000.; outV = true; }
				if (_outUnits == "m") outV = true;

				if (inV && outV) { return true; }
				return false;
			}
			bool simpleUnits::validFreq(const std::string &_inUnits, const std::string &_outUnits) {
				bool inV = false, outV = false;
				_convFactor = 1.f;
				if (_inUnits == "GHz") { _convFactor *= 1e9; inV = true; }
				if (_inUnits == "MHz") { _convFactor *= 1e6; inV = true; }
				if (_inUnits == "KHz") { _convFactor *= 1e3; inV = true; }
				if (_inUnits == "Hz" || _inUnits == "s^-1" || _inUnits == "1/s") inV = true;
				if (_outUnits == "GHz") { _convFactor /= 1e9; outV = true; }
				if (_outUnits == "MHz") { _convFactor /= 1e6; outV = true; }
				if (_outUnits == "KHz") { _convFactor /= 1e3; outV = true; }
				if (_outUnits == "Hz" || _outUnits == "s^-1" || _outUnits == "1/s") outV = true;

				if (inV && outV) { return true; }
				return false;
			}
			bool simpleUnits::validVol(const std::string &_inUnits, const std::string &_outUnits) {
				bool inV = false, outV = false;
				_convFactor = 1.f;
				std::string in = _inUnits, out = _outUnits;

				// If it doesn't end in ^3, add it. Used to prevent awkward string manipulations.
				if (in.find("^3") == std::string::npos) in.append("^3");
				if (out.find("^3") == std::string::npos) out.append("^3");
				if (in == "nm^3") { _convFactor /= 1e27; inV = true; }
				if (in == "um^3") { _convFactor /= 1e18; inV = true; }
				if (in == "mm^3") { _convFactor /= 1e9; inV = true; }
				if (in == "cm^3") { _convFactor /= 1e6; inV = true; }
				if (in == "km^3") { _convFactor *= 1e6; inV = true; }
				if (in == "m^3") inV = true;
				if (out == "nm^3") { _convFactor *= 1e27; outV = true; }
				if (out == "um^3") { _convFactor *= 1e18; outV = true; }
				if (out == "mm^3") { _convFactor *= 1e9; outV = true; }
				if (out == "cm^3") { _convFactor *= 1e6; outV = true; }
				if (out == "km^3") { _convFactor /= 1e6; outV = true; }
				if (out == "m^3") outV = true;
				if (inV && outV) { return true; }
				return false;
			}
			bool simpleUnits::validPres(const std::string &in, const std::string &out) {
				bool inV = false, outV = false;
				_convFactor = 1.f;
				if (in == "mb" || in == "millibar") { _convFactor *= 100; inV = true; }
				if (in == "hPa") { _convFactor *= 100; inV = true; }
				if (in == "Pa") inV = true;
				if (in == "kPa") { _convFactor *= 1000; inV = true; }
				if (in == "bar") { _convFactor *= 100000; inV = true; }
				if (out == "mb" || out == "millibar" || out == "hPa") { _convFactor /= 100; outV = true; }
				if (out == "bar") { _convFactor /= 100000; outV = true; }
				if (out == "Pa") outV = true;
				if (out == "kPa") { _convFactor /= 1000; outV = true; }
				if (inV && outV) { return true; }
				return false;
			}
			bool simpleUnits::validMass(const std::string &in, const std::string &out) {
				bool inV = false, outV = false;
				_convFactor = 1.f;
				if (in == "ug") { _convFactor /= 1e9; inV = true; }
				if (in == "mg") { _convFactor /= 1e6; inV = true; }
				if (in == "g") { _convFactor /= 1e3; inV = true; }
				if (in == "kg") inV = true;
				if (out == "ug") { _convFactor *= 1e9; outV = true; }
				if (out == "mg") { _convFactor *= 1e6; outV = true; }
				if (out == "g") { _convFactor *= 1e3; outV = true; }
				if (out == "kg") outV = true;
				if (inV && outV) { return true; }
				return false;
			}
			bool simpleUnits::validTemp(const std::string &in, const std::string &out) {
				_convFactor = 1.f;
				bool inV = false, outV = false;
				// K - Kelvin, C - Celsius, F - Fahrenheit, R - Rankine
				if (in == "K" || in == "degK") inV = true;
				if (in == "C" || in == "degC") { inV = true; _inOffset += 273.15; }
				if (in == "F" || in == "degF") { inV = true; _convFactor *= 5. / 9.; _inOffset += 459.67; }
				if (in == "R" || in == "degR") { inV = true; _convFactor *= 5. / 9; }
				if (out == "K" || out == "degK") outV = true;
				if (out == "C" || out == "degC") { outV = true; _outOffset -= 273.15; }
				if (out == "F" || out == "degF") { outV = true; _convFactor *= 9. / 5.; _outOffset -= 459.67; }
				if (out == "R" || out == "degR") { outV = true; _convFactor *= 9. / 5.; }
				if (inV && outV) { return true; }
				return false;
			}
			bool simpleUnits::validDens(const std::string &in, const std::string &out) {
				_convFactor = 1.f;
				// Handling only number density here
				// TODO: do other types of conversions
				// Most further stuff requires knowledge of R, thus knowledge of 
				// relative humidity
				bool inV = false, outV = false;
				if (in == "m^-3") inV = true;
				if (in == "cm^-3") { inV = true; _convFactor *= 1e6; }
				if (out == "cm^-3") { outV = true; _convFactor /= 1e6; }
				if (out == "m^-3") outV = true;
				if (inV && outV) { return true; }
				if (in == "ppmv" && out == "ppmv") { return true; } // ppmv identity
				return false;
			}
			simpleUnits::~simpleUnits() {}
			bool simpleUnits::isValid() const { return _valid; }
			double simpleUnits::convert(double inVal) const
			{
				if (_valid) return ((inVal + _inOffset) * _convFactor) + (_outOffset);
				SDBR_throw(scatdb::error::error_types::xBadInput)
					.add<std::string>("Reason", "Trying to convert with bad converter units.")
					;
				return 0;
			}
			
			bool spectralUnits::canConvert(Converter_registry_provider::optsType opts) {
				const std::string in = opts->getVal<std::string>("inUnits");
				const std::string out = opts->getVal<std::string>("outUnits");
				bool inIsLen = converter::canConvert(in, "m");
				bool inIsWv = converter::canConvert(in, "Hz");
				bool outIsLen = converter::canConvert(out, "m");
				bool outIsWv = converter::canConvert(out, "Hz");
				if ((inIsLen || inIsWv) && (outIsLen || outIsWv)) return true;
				return false;
			}
			std::shared_ptr<const implementations::Unithandler> spectralUnits::constructConverter(
				Converter_registry_provider::optsType opts) {
				const std::string in = opts->getVal<std::string>("inUnits");
				const std::string out = opts->getVal<std::string>("outUnits");

				// Can input be converted to a length, or is it an inverse length?
				bool inIsLen = converter::canConvert(in, "m");
				bool inIsWv = converter::canConvert(in, "Hz");
				bool outIsLen = converter::canConvert(out, "m");
				bool outIsWv = converter::canConvert(out, "Hz");
				if ((inIsLen && outIsLen) || (inIsWv && outIsWv)) {
					return std::shared_ptr<simpleUnits>(new simpleUnits(in, out, true));
				}
				// These are heterogeneous.
				std::shared_ptr<spectralUnits> res(new spectralUnits(in, out));

				if (inIsLen) {
					res->hIn = converter::getConverter(in, "m");
					res->_Iin = true;
				}
				if (inIsWv) res->hIn = converter::getConverter(in, "Hz");
				if (outIsLen) {
					res->hOut = converter::getConverter("m", out);
					res->_Iout = true;
				}
				if (outIsWv) res->hOut = converter::getConverter("Hz", out);
				if (res->hIn && res->hOut) res->_valid = true;
				else res->_valid = false;
				return res;
			}
			spectralUnits::spectralUnits(const std::string &in, const std::string &out) :
				Unithandler("spectral"),
				_inUnits(in), _outUnits(out), _valid(true), _Iin(false),
				_Iout(false)
			{
			}

			spectralUnits::~spectralUnits() {}
			bool spectralUnits::isValid() const { return _valid; }

			double spectralUnits::convert(double input) const {
				if (!_valid) return -1;
				double res = input;
				res = hIn->convert(res);

				const double c = 2.99792458e8; // m/s
											   // If inversion needed...
				if (_Iin) res = c / res;
				// Res is now in Hz. Do output conversion.
				if (_Iout) res = c / res;
				res = hOut->convert(res);

				return res;
			}

		}

	}
}

