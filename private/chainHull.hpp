#pragma once
#include "../scatdb/defs.hpp"

namespace scatdb {
	namespace contrib {
		namespace chainHull {
			HIDDEN_SDBR class Point { public: 
				float x, y; 
				Point(float a, float b) : x(a), y(b) {}
				Point() : x(0), y(0) {} 
			};

			HIDDEN_SDBR int chainHull_2D(Point* P, int n, Point* H);
		}
	}
}