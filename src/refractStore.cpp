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
#include "../Ryan_Scat/refract.hpp"
#include "../Ryan_Scat/refractBase.hpp"
#include "../Ryan_Scat/zeros.hpp"
#include "../Ryan_Scat/units/units.hpp"
//#include "../private/linterp.h"
#include "../Ryan_Scat/error.hpp"
#include "../Ryan_Scat/logging.hpp"

namespace Ryan_Scat {
	namespace refract {
	}
}
