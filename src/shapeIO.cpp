#include <string>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
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
			using namespace boost::filesystem;
			path p(filename);
			if (!exists(p)) SDBR_throw(error::error_types::xMissingFile)
				.add<std::string>("Reason", "Attempting to read a file that does not exist")
				.add<std::string>("Filename", filename);
			path pext = p.extension();

			if (pext.string() == ".hdf5") {
				// hdf5 files may contain many shapes
				scatdb::plugins::hdf5::readShapesHDF5(filename, shapes);
			} else {
				// Assume a text file
				std::ifstream in(filename.c_str());
				auto shp = ::scatdb::shape::shape::generate();
				scatdb::plugins::builtin::shape::readDDSCAT(shp, in);
				shapes.push_back(shp);
				modifiableOutput.push_back(shp);
			}
		}
		void shapeIO::readFile(const std::string &filename) {
			std::vector<std::shared_ptr<scatdb::shape::shape> > mo;
			readFile(filename, mo);
		}
		void shapeIO::writeFile(const std::string &filename) const {
			using namespace boost::filesystem;
			path p(filename);
			path pext = p.extension();

			if (pext.string() == ".hdf5") {
				// hdf5 files may contain many shapes
				plugins::hdf5::writeShapesHDF5(filename, this->shapes);
			}
			else {
				SDBR_throw(error::error_types::xUnimplementedFunction);
			}
		}
	}
}