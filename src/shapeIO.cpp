#include <string>
#include <iostream>
#include <fstream>
#include "../scatdb/error.hpp"
#include "../scatdb/logging.hpp"
#include "../scatdb/shape/shape.hpp"
#include "../scatdb/shape/shapeIO.hpp"
#include "../private/shapeIOtext.hpp"

namespace scatdb {
	namespace shape {
		std::shared_ptr<shapeIO> shapeIO::generate() {
			std::shared_ptr<shapeIO> res
			(new shapeIO);
			return res;
		}
		shapeIO::shapeIO() {}
		shapeIO::~shapeIO() {}
		void shapeIO::readFile(const std::string &filename, std::vector<std::shared_ptr<scatdb::shape::shape> > &modifiableOutput) {
			auto shp = ::scatdb::shape::shape::generate();
			// TODO: look at extension!
			SDBR_log("shapeIO", logging::ERROR, "TODO: look at file extension when reading!");
			std::ifstream in(filename.c_str());
			scatdb::plugins::builtin::shape::readDDSCAT(shp, in);
			shapes.push_back(shp);
			modifiableOutput.push_back(shp);
		}
		void shapeIO::readFile(const std::string &filename) {
			auto shp = ::scatdb::shape::shape::generate();
			std::ifstream in(filename.c_str());
			scatdb::plugins::builtin::shape::readDDSCAT(shp, in);
			shapes.push_back(shp);
		}
		void shapeIO::writeFile(const std::string &filename) const {
			// TODO: look at extension!
			SDBR_throw(error::error_types::xUnimplementedFunction);
		}
	}
}