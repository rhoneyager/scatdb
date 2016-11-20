#pragma once
#include "defs.hpp"

#include <complex>
#include <functional>

#include "error.hpp"

namespace scatdb {

	namespace zeros {
		/// Zero-finding implementation - Brent's method
		DLEXPORT_SDBR double findzero(double a, double b, const std::function<double(double) > & evaltarget);

		// f is an arbitrary class with operator(). So, a functional / lambda function does work.
		template<class T, class U>
		U secantMethod(const T &f, 
			U guess_a, U guess_b,
			double eps = 0.000001, size_t maxIter = 50)
		{
			// Secant method is defined by recurrance
			// xn = x_(n-1) - f(x_(n-1)) * (x_(n-1) - x_(n-2)) / (f(x_(n-1)) - f(x_(n-2)))
			using namespace std;
			U zero;
			U xn = guess_a, xn1 = guess_b, xn2;
			U fxn1, fxn2;
			size_t i=0;
			do {
				xn2 = xn1;
				xn1 = xn;

				fxn1 = f(xn1);
				fxn2 = f(xn2);

				xn = xn1 - fxn1 * (xn1 - xn2) / (fxn1 - fxn2);
			} while ( (abs(xn-xn1) > eps) && (i++ < maxIter));

			if (i >= maxIter) SDBR_throw(scatdb::error::error_types::xModelOutOfRange)
				.add<size_t>("maxIter", maxIter)
				.add<double>("eps", eps)
				.template add<U>("guess_a", guess_a)
				.template add<U>("guess_b", guess_b)
				.template add<U>("xn",xn)
				.template add<U>("xn1",xn1)
				.template add<U>("xn2",xn2)
				.template add<U>("fxn1",fxn1)
				.template add<U>("fxn2",fxn2);
			zero = xn;
			return zero;
		}

	}

}

