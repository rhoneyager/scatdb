/// \brief Provides hdf5 file IO
#define _SCL_SECURE_NO_WARNINGS
#pragma warning( disable : 4251 ) // warning C4251: dll-interface

#include <array>
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <tuple>

#include <boost/filesystem.hpp>

#include "../scatdb/shape/shapeIO.hpp"
#include "../scatdb/shape/shape.hpp"
#include "../scatdb/error.hpp"
#include "../scatdb/hash.hpp"
#include "../scatdb/export-hdf5.hpp"
#include <hdf5.h>
#include <H5Cpp.h>

namespace scatdb {
	namespace plugins {
		namespace hdf5 {
			std::shared_ptr<H5::Group> write_hdf5_shaperawdata(std::shared_ptr<H5::Group> shpraw,
				const std::shared_ptr<const scatdb::shape::shape > shp)
			{
				using namespace std;
				using namespace H5;
				try {
					auto hash = shp->hash();
					addAttr<uint64_t, H5::Group>(shpraw, "Hash", hash->lower);

					string sDesc = shp->getDescription();
					addAttr(shpraw, "Description", sDesc);

					auto hdr = shp->getHeader();
					addDatasetEigen(shpraw, "Header", *(hdr.get()));

					auto pts = shp->getPoints();
					addDatasetEigen(shpraw, "Points", *(pts.get()));

					double dSpacing = shp->getPreferredDipoleSpacing();
					addAttr(shpraw, "Dipole_Spacing", dSpacing);
					
					auto stats = shp->getStats();
					addDatasetEigen(shpraw, "Stats", *(stats.get()));

					scatdb::shape::tags_t tags;
					shp->getTags(tags);
					addAttr<uint64_t, H5::Group>(shpraw, "Num_Tags", (uint64_t) tags.size());

					// Tags
					if (tags.size())
					{
						const size_t nTagCols = 2;
						typedef std::array<const char*, nTagCols> strdata;
						std::vector<strdata> sdata(tags.size());
						size_t i = 0;
						for (const auto &t : tags)
						{
							sdata.at(i).at(0) = t.first.c_str();
							sdata.at(i).at(1) = t.second.c_str();
							++i;
						}

						hsize_t dim[1] = { static_cast<hsize_t>(tags.size()) };
						DataSpace space(1, dim);
						// May have to cast array to a private structure
						H5::StrType strtype(0, H5T_VARIABLE);
						CompType stringTableType(sizeof(strdata));
						stringTableType.insertMember("Key", ARRAYOFFSET(strdata, 0), strtype);
						stringTableType.insertMember("Value", ARRAYOFFSET(strdata, 1), strtype);
						std::shared_ptr<DataSet> sdataset(new DataSet(shpraw->createDataSet(
							"Tags", stringTableType, space)));
						sdataset->write(sdata.data(), stringTableType);
					}
				}
				catch (H5::Exception &e)
				{
					std::cerr << e.getCDetailMsg() << std::endl;
					throw e;
				}
				return shpraw;
			}

			void writeShapesHDF5(const std::string &filename,
				const std::vector<std::shared_ptr<const ::scatdb::shape::shape> > & shps)
			{
				using std::shared_ptr;
				using namespace H5;
				Exception::dontPrint();
				shared_ptr<H5::H5File> file(new H5::H5File(filename, H5F_ACC_TRUNC));
				
				shared_ptr<Group> grpShapes = openOrCreateGroup(file, "Shapes");
				for (const auto & shp : shps) {
					shared_ptr<Group> grpShape = openOrCreateGroup(grpShapes, shp->hash()->string().c_str());
					shared_ptr<Group> newbase = write_hdf5_shaperawdata(grpShape, shp);
				}
			}
		}
	}
}
