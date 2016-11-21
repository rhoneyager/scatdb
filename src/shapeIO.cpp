#include "../scatdb/error.hpp"
#include "../scatdb/shape/shape.hpp"
#include "../scatdb/shape/shapeIO.hpp"

namespace scatdb {
	namespace shape {
		std::shared_ptr<shapeIO> shapeIO::generate() {
			std::shared_ptr<shapeIO> res
			(new shapeIO);
			return res;
		}
		shapeIO::shapeIO() {}
		shapeIO::~shapeIO() {}
	}
}