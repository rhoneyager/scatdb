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

			void read_hdf5_shaperawdata(std::shared_ptr<H5::Group> base,
				std::shared_ptr<scatdb::shape::shape > &shp)
			{
				using namespace std;
				using namespace H5;

				// Read in attributes:

				// Description
				string sDesc;
				readAttr<std::string, Group>(base, "Description", sDesc);
				shp->setDescription(sDesc);

				double dSpacing = 0;
				if (attrExists(base, "Dipole_Spacing"))
					readAttr<double, Group>(base, "Dipole_Spacing", dSpacing);
				shp->setPreferredDipoleSpacing(dSpacing);

				shared_ptr<scatdb::shape::shapeHeaderStorage_t> hdr(new scatdb::shape::shapeHeaderStorage_t);
				readDatasetEigen(base, "Header", *hdr);
				shp->setHeader(hdr);

				shared_ptr<scatdb::shape::shapeStorage_t> pts(new shape::shapeStorage_t);
				readDatasetEigen(base, "Points", *pts);
				shp->setPoints(pts);

				// Tags
				// Get size of the tags object
				uint64_t numTags = 0;
				readAttr<uint64_t, Group>(base, "Num_Tags", numTags);
				if (numTags)
				{
					const size_t nTagCols = 2;
					typedef std::array<const char*, nTagCols> strdata;
					
					std::vector<strdata> sdata(numTags);

					hsize_t dim[1] = { static_cast<hsize_t>(numTags) };
					DataSpace space(1, dim);
					// May have to cast array to a private structure
					H5::StrType strtype(0, H5T_VARIABLE);
					CompType stringTableType(sizeof(strdata));
					stringTableType.insertMember("Key", ARRAYOFFSET(strdata, 0), strtype);
					stringTableType.insertMember("Value", ARRAYOFFSET(strdata, 1), strtype);

					std::shared_ptr<DataSet> sdataset(new DataSet(base->openDataSet("Tags")));
					sdataset->read(sdata.data(), stringTableType);

					//size_t i = 0;
					shape::tags_t tt;
					for (const auto &t : sdata)
					{
						tt.insert(std::pair<std::string, std::string>(
							std::string(t.at(0)), std::string(t.at(1))));
					}
					shp->setTags(tt);
				}
			}

			using std::shared_ptr;
			using namespace scatdb::plugins::hdf5;

			void readShapesHDF5(const std::string &filename,
				std::vector<std::shared_ptr<const ::scatdb::shape::shape> > &shps)
			{
				shared_ptr<H5::H5File> file(new H5::H5File(filename, H5F_ACC_RDONLY));

				using namespace H5;
				Exception::dontPrint();
				using namespace std;

				shared_ptr<Group> grpShapes = openGroup(file, "Shapes");
				if (!grpShapes) SDBR_throw(error::error_types::xMissingKey)
					.add<string>("Reason", "Reading shapes from HDF5 file that is missing the path /Shapes")
					.add<string>("Filename", filename);

				hsize_t sz = grpShapes->getNumObjs();
				shps.reserve(shps.size() + (size_t)sz);
				for (hsize_t i = 0; i < sz; ++i) {
					string hname = grpShapes->getObjnameByIdx(i);
					H5G_obj_t t = grpShapes->getObjTypeByIdx(i);
					if (t != H5G_obj_t::H5G_GROUP) continue;
					shared_ptr<Group> grpShape = openGroup(grpShapes, hname.c_str());
					if (grpShape) {
						auto shp = shape::shape::generate();
						read_hdf5_shaperawdata(grpShape, shp);
						shps.push_back(shp);
					}
				}
			}

		}
	}
}
