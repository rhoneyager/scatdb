#include "../Ryan_Scat/zeros.hpp"
#include <cmath>

namespace Ryan_Scat {

	namespace zeros {
		double findzero(double a, double b, const std::function<double(double) > &f )
		{
			using namespace std;
			/* Define the basic testing variables.
			   eval func is the function's evaluator.
			   It typically links to the polynomial eval func.
			   The rest of the vars are used in Brent's method.
			   */
			//evalfunction *f = evaltarget;
			//double (*f)(double) = (double (*)(double)) evalfunc;
			double convint = 1E-7;
			double fcint = 1E-13;
			double fa, fb, fc, fd, fs = 1.0;
			unsigned int i=0;
			fa = f(a);
			fb = f(b);

			// Bad starting points if fa*fb > 0
			if (fa*fb > 0) 
				RSthrow(Ryan_Scat::error::error_types::xBadInput)
					.add<std::string>("Reason",
						"Bad selection of a,b. f(a) and f(b) need to have opposite signs.")
				.add<double>("a",a).add<double>("b",b)
				.add<double>("fa",fa).add<double>("fb",fb);

			if ( abs(fa) < abs(fb) )
			{
				// Swap the two here. Junk only declared in block.
				double junk = b;
				b = a;
				a = junk;
				fa = f(a);
				fb = f(b);
			}

			double c = a; // A testing point
			double s = 0, d = 0;  // Initialize these to stop VS whining
			fc = f(c);
			bool mflag = true; // Flag for method stuff
			while (fb != 0 || fs != 0 || abs(b-a) > convint)
			{
				if (fa != fc && fb != fc)
				{
					// Perform inverse quadratic interpolation
					double i,j,k;
					i = a * fb * fc / ( (fa - fb)*(fa - fc));
					j = b * fa * fc / ( (fb - fa)*(fb - fc));
					k = c * fa * fb / ( (fc - fa)*(fc - fb));
					s = i + j + k;
				} else {
					// Use Secant Rule
					s = b - fb * (b - a) / ( fb - fa );
				}

				if (
						( (3.0*a+b)/4.0 < s && s < b) == false ||
						(mflag && abs(s-b) >= (abs(b-c)/2.0) ) ||
						(!mflag && abs(s-b) >= (abs(c-d)/2.0) ) ||
						(mflag && abs(b-c) < convint) ||
						(!mflag && abs(c-d) < convint)
						)
				{
					// Bisection Method
					mflag = true;
					s = 0.5 * (a + b);
				} else {
					mflag = false;
				}

				fs = f(s);

				d = c;
				c = b;

				fd = f(d);
				fc = f(c);

				if (fa * fs < 0)
				{
					b = s;
					fb = f(b);
				} else {
					a = s;
					fa = f(a);
				}


				if ( abs(fa) < abs(fb) )
				{
					// Do a swap again
					double junk = b;
					b = a;
					a = junk;
					fa = f(a);
					fb = f(b);
				}
				i++;
				//if (i > 100) break;
				if ( abs(b-a) < convint && abs(fb) < fcint && abs(fs) < fcint ) break;
			}
			return s;
		}


	}
}

