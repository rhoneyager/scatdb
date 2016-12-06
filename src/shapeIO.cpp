#include <string>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include "../scatdb/error.hpp"
#include "../scatdb/hash.hpp"
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
				auto shp = ::scatdb::plugins::builtin::shape::readTextFile(filename);
				shapes.push_back(shp);
				modifiableOutput.push_back(shp);
			}
		}
		void shapeIO::readFile(const std::string &filename) {
			std::vector<std::shared_ptr<scatdb::shape::shape> > mo;
			readFile(filename, mo);
		}
		void shapeIO::writeFile(const std::string &filename, const std::string &outType) const {
			using namespace boost::filesystem;
			path p(filename);
			path pext = p.extension();

			if (pext.string() == ".hdf5" || outType == "hdf5") {
				// hdf5 files may contain many shapes
				plugins::hdf5::writeShapesHDF5(filename, this->shapes);
			} else {
				auto writeShp = [&outType](const std::string &filename, shape_ptr s) {
					if (outType == "ddscat" || outType == "") plugins::builtin::shape::writeDDSCAT(filename, s);
					else if (outType == "raw") plugins::builtin::shape::writeTextRaw(filename, s);
					else SDBR_throw(error::error_types::xUnimplementedFunction);
				};
				if (shapes.size() > 1) {
					boost::filesystem::create_directory(p);
					for (const auto &s : shapes) {
						writeShp(s->hash()->string(), s);
					}
				}
				else if (shapes.size() == 1) writeShp(filename, shapes[0]);
				
			}
		}

	}
}