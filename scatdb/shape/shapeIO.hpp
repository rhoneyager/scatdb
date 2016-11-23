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
			void readFile(const std::string &filename);
			void readFile(const std::string &filename, std::vector<std::shared_ptr<scatdb::shape::shape> > &modifiableOutput);
			void writeFile(const std::string &filename) const;
		};
	}
}
