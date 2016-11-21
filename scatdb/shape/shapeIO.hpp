#pragma once
#include "../defs.hpp"
#include <memory>
#include <vector>
#include "shapeForwards.hpp"

namespace scatdb {
	namespace shape {
		class DLEXPORT_SDBR shapeIO
		{
		public:
			shapeIO();
			virtual ~shapeIO();
			static std::shared_ptr<shapeIO> generate();
			std::vector<shape_ptr> shapes;
		};
	}
}
