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
#include "../scatdb/export-hdf5.hpp"
#include <hdf5.h>
#include <H5Cpp.h>

namespace scatdb {
	namespace plugins {
		namespace hdf5 {
			std::shared_ptr<H5::Group> write_hdf5_shaperawdata(std::shared_ptr<H5::Group> shpraw,
				const std::shared_ptr<const scatdb::shape::shape > shp)
			{
				using std::shared_ptr;
				using namespace H5;
				try {
					int fillvalue = -1;   /* Fill value for the dataset */
					DSetCreatPropList plist_3, plist_1;
					plist_3.setFillValue(PredType::NATIVE_INT, &fillvalue);
					plist_1.setFillValue(PredType::NATIVE_INT, &fillvalue);
					// Enable compression
					hsize_t chunk_3[2] = { (hsize_t)shp->numPoints(), 3 };
					hsize_t chunk_1[2] = { (hsize_t)shp->numPoints(), 1 };
					plist_3.setChunk(2, chunk_3);
					plist_1.setChunk(1, chunk_1);
					if (scatdb::plugins::hdf5::useZLIB()) {
						plist_3.setDeflate(6);
						plist_1.setDeflate(6);
					}

					// Create a dataspace describing the dataset size
					hsize_t fDimBasic[] = { shp->numPoints(), 3 };
					DataSpace fspacePts(2, fDimBasic);
					// Write the entire dataset (no hyperslabs necessary)
					shared_ptr<DataSet> latticePts(new DataSet(shpraw->createDataSet("latticePts", PredType::NATIVE_INT,
						fspacePts, plist_3)));
					Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor> latticePtsInt = shp->latticePts.cast<int>();
					latticePts->write(latticePtsInt.data(), PredType::NATIVE_INT);

					shared_ptr<DataSet> latticePtsRi(new DataSet(shpraw->createDataSet("latticePtsRi", PredType::NATIVE_INT,
						fspacePts, plist_3)));
					Eigen::Matrix<int, Eigen::Dynamic, 3, Eigen::RowMajor> latticePtsRiInt = shp->latticePtsRi.cast<int>();
					latticePtsRi->write(latticePtsRiInt.data(), PredType::NATIVE_INT);

					//shared_ptr<DataSet> latticePtsStd(new DataSet(shpraw->createDataSet("latticePtsStd", PredType::NATIVE_FLOAT, 
					//	fspacePts, plist_3)));
					//Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> latticePtsStdRow(shp->latticePtsStd);
					//latticePtsStd->write(latticePtsStdRow.data(), PredType::NATIVE_FLOAT);

					//shared_ptr<DataSet> latticePtsNorm(new DataSet(shpraw->createDataSet("latticePtsNorm", PredType::NATIVE_FLOAT, 
					//	fspacePts, plist_3)));
					//Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> latticePtsNormRow(shp->latticePtsNorm);
					//latticePtsNorm->write(latticePtsNormRow.data(), PredType::NATIVE_FLOAT);

					hsize_t fDimBasic1[] = { shp->numPoints() };
					DataSpace fspacePts1(1, fDimBasic1);
					shared_ptr<DataSet> latticePtsIndex(new DataSet(shpraw->createDataSet("latticePtsIndex", PredType::NATIVE_INT,
						fspacePts1, plist_1)));
					latticePtsIndex->write(shp->latticeIndex.data(), PredType::NATIVE_INT);

					// Description
					addAttr<std::string, Group>(shpraw, "Description", shp->desc);
					// Number of points
					addAttr<size_t, Group>(shpraw, "Number_of_points", shp->numPoints());
					// Dielectrics
					// The full hash
					addAttr<uint64_t, Group>(shpraw, "Hash_Lower", shp->hash().lower);

					addAttr<float, Group>(shpraw, "Standard_Dipole_Spacing", shp->standardD);

					// mins, maxs, means
					addAttrEigen<Eigen::Array3f, Group>(shpraw, "mins", shp->mins);
					addAttrEigen<Eigen::Array3f, Group>(shpraw, "maxs", shp->maxs);
					addAttrEigen<Eigen::Array3f, Group>(shpraw, "means", shp->means);
					// a1, a2, a3, d, x0, xd
					addAttrEigen<Eigen::Array3f, Group>(shpraw, "a1", shp->a1);
					addAttrEigen<Eigen::Array3f, Group>(shpraw, "a2", shp->a2);
					addAttrEigen<Eigen::Array3f, Group>(shpraw, "a3", shp->a3);
					addAttrEigen<Eigen::Array3f, Group>(shpraw, "d", shp->d);
					addAttrEigen<Eigen::Array3f, Group>(shpraw, "x0", shp->x0);
					addAttrEigen<Eigen::Array3f, Group>(shpraw, "xd", shp->xd);

					// Tags
					{
						addAttr<size_t, Group>(shpraw, "Num_Tags", shp->tags.size());

						if (shp->tags.size())
						{
							const size_t nTagCols = 2;
							typedef std::array<const char*, nTagCols> strdata;
							std::vector<strdata> sdata(shp->tags.size());
							size_t i = 0;
							for (const auto &t : shp->tags)
							{
								sdata.at(i).at(0) = t.first.c_str();
								sdata.at(i).at(1) = t.second.c_str();
								++i;
							}

							hsize_t dim[1] = { static_cast<hsize_t>(shp->tags.size()) };
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
				}
				catch (H5::Exception &e)
				{
					std::cerr << e.getCDetailMsg() << std::endl;
					throw e;
				}
				return shpraw;
			}

			template<>
			shared_ptr<IOhandler>
				write_file_type_multi<rtmath::ddscat::shapefile::shapefile>
				(std::shared_ptr<IOhandler> sh, std::shared_ptr<IO_options> opts,
					const std::shared_ptr<const scatdb::shape::shapeIO > p)
			{
				std::string filename = opts->filename();
				IOhandler::IOtype iotype = opts->iotype();
				std::string key = opts->getVal<std::string>("key", "");
				using std::shared_ptr;
				using namespace H5;
				Exception::dontPrint();
				std::shared_ptr<hdf5_handle> h = registry::construct_handle
					<registry::IOhandler, hdf5_handle>(
						sh, PLUGINID, [&]() {return std::shared_ptr<hdf5_handle>(
							new hdf5_handle(filename.c_str(), iotype)); });

				// Check for the existence of the appropriate:
				// Group "Hashed"
				shared_ptr<Group> grpHashes = openOrCreateGroup(h->file, "Hashed");
				// Group "Hashed"/shp->hash
				shared_ptr<Group> grpShape = openOrCreateGroup(grpHashes, shp->hash().string().c_str());
				// Group "Hashed"/shp->hash/"Shape". If it exists, overwrite it. There should be no hard links here.
				/// \note The unlink operation does not really free the space..... Should warn the user.
				if (groupExists(grpShape, "Shape")) return h; //grpShape->unlink("Shape");
				shared_ptr<Group> newbase = write_hdf5_shaperawdata(grpShape, shp);

				// An example...
				//boost::shared_ptr<rtmath::ddscat::shapefile::shapefile> shpcheck(new rtmath::ddscat::shapefile::shapefile);
				//read_hdf5_shaperawdata(openGroup(grpShape, "Shape"), shpcheck);

				return h; // Pass back the handle
			}
		}
	}
}
