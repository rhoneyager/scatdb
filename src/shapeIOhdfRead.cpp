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

#include "../../rtmath/rtmath/defs.h"
#include "../../rtmath/rtmath/ddscat/shapefile.h"
#include "../../rtmath/rtmath/plugin.h"
#include "../../rtmath/rtmath/error/debug.h"
#include <Ryan_Debug/error.h>

#include "plugin-hdf5.h"
#include "../../related/rtmath_hdf5_cpp/export-hdf5.h"
#include <hdf5.h>
#include <H5Cpp.h>
#include "cmake-settings.h"

namespace rtmath {
	namespace plugins {
		namespace hdf5 {

			bool read_hdf5_shaperawdata(std::shared_ptr<H5::Group> base,
				boost::shared_ptr<rtmath::ddscat::shapefile::shapefile > shp)
			{
				using std::shared_ptr;
				using namespace H5;

				// Read in attributes:

				// Description
				readAttr<std::string, Group>(base, "Description", shp->desc);
				// Shape hash
				Ryan_Debug::hash::HASH_t hash;
				readAttr<uint64_t, Group>(base, "Hash_Lower", hash.lower);
				readAttr<uint64_t, Group>(base, "Hash_Upper", hash.upper);
				shp->setHash(hash);
				// Number of points
				size_t numPoints;
				readAttr<size_t, Group>(base, "Number_of_points", numPoints);
				shp->resize(numPoints);
				// Source_filename
				readAttr<std::string, Group>(base, "Source_Filename", shp->filename);
				readAttr<std::string, Group>(base, "ingest_timestamp", shp->ingest_timestamp);
				readAttr<std::string, Group>(base, "ingest_hostname", shp->ingest_hostname);
				readAttr<int, Group>(base, "ingest_rtmath_version", shp->ingest_rtmath_version);
				readAttr<std::string, Group>(base, "ingest_username", shp->ingest_username);

				if (attrExists(base, "Standard_Dipole_Spacing"))
					readAttr<float, Group>(base, "Standard_Dipole_Spacing", shp->standardD);

				// mins, maxs, means
				readAttrEigen<Eigen::Array3f, Group>(base, "mins", shp->mins);
				readAttrEigen<Eigen::Array3f, Group>(base, "maxs", shp->maxs);
				readAttrEigen<Eigen::Array3f, Group>(base, "means", shp->means);
				// a1, a2, a3, d, x0, xd
				readAttrEigen<Eigen::Array3f, Group>(base, "a1", shp->a1);
				readAttrEigen<Eigen::Array3f, Group>(base, "a2", shp->a2);
				readAttrEigen<Eigen::Array3f, Group>(base, "a3", shp->a3);
				readAttrEigen<Eigen::Array3f, Group>(base, "d", shp->d);
				readAttrEigen<Eigen::Array3f, Group>(base, "x0", shp->x0);
				readAttrEigen<Eigen::Array3f, Group>(base, "xd", shp->xd);

				// Tags
				{
					//addAttr<size_t, Group>(shpraw, "Num_Tags", shp->tags.size());

					const size_t nTagCols = 2;
					typedef std::array<const char*, nTagCols> strdata;
					// Get size of the tags object
					size_t numTags = 0;
					readAttr<size_t, Group>(base, "Num_Tags", numTags);

					if (numTags)
					{
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



						//std::shared_ptr<DataSet> sdataset(new DataSet(base->createDataSet(
						//	"Tags", stringTableType, space)));
						//sdataset->write(sdata.data(), stringTableType);


						//size_t i = 0;
						for (const auto &t : sdata)
						{
							shp->tags.insert(std::pair<std::string, std::string>(
								std::string(t.at(0)), std::string(t.at(1))));
							//sdata.at(i).at(0) = t.first.c_str();
							//sdata.at(i).at(1) = t.second.c_str();
							//++i;
						}
					}
				}

				// Read in tables:
				Eigen::Matrix<int, Eigen::Dynamic, 1> lptsi;
				// Dielectrics
				readDatasetEigen<Eigen::Matrix<int, Eigen::Dynamic, 1>, Group>
					(base, "Dielectrics", lptsi);
				for (size_t i = 0; i < (size_t)lptsi.rows(); ++i)
					shp->Dielectrics.insert((size_t)lptsi(i));

				// latticePts
				Eigen::Matrix<int, Eigen::Dynamic, 3> lpts;
				Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> lptsf;
				readDatasetEigen<Eigen::Matrix<int, Eigen::Dynamic, 3>, Group>
					(base, "latticePts", lpts);
				shp->latticePts = lpts.cast<float>();
				// latticePtsIndex
				readDatasetEigen<Eigen::Matrix<int, Eigen::Dynamic, 1>, Group>
					(base, "latticePtsIndex", lptsi);
				shp->latticeIndex = lptsi;
				// latticePtsNorm
				//readDatasetEigen<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>, Group>
				//	(base, "latticePtsNorm", lptsf);
				shp->latticePtsNorm.resizeLike(shp->latticePts);
				for (size_t i = 0; i < numPoints; i++)
				{
					auto pt = shp->latticePts.block<1, 3>(i, 0);
					auto Npt = shp->latticePtsNorm.block<1, 3>(i, 0);
					Npt = pt.array().transpose() - shp->means;
				}
				// latticePtsRi
				readDatasetEigen<Eigen::Matrix<int, Eigen::Dynamic, 3>, Group>
					(base, "latticePtsRi", lpts);
				shp->latticePtsRi = lpts.cast<float>();
				// latticePtsStd
				//readDatasetEigen<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>, Group>
				//	(base, "latticePtsStd", lptsf);
				shp->latticePtsStd.resizeLike(shp->latticePts);
				for (size_t i = 0; i < numPoints; i++)
				{
					auto pt = shp->latticePts.block<1, 3>(i, 0);
					Eigen::Array3f crd = pt.array() * shp->d.transpose();
					auto Npt = shp->latticePtsStd.block<1, 3>(i, 0);
					Npt = crd.matrix() - shp->xd.matrix();
				}

				// Extras group
				// Read in all extras tables
				shared_ptr<Group> grpExtras = openGroup(base, "Extras");
				// Iterate over tables
				hsize_t nObjs = grpExtras->getNumObjs();
				for (hsize_t i = 0; i < nObjs; ++i)
				{
					std::string name = grpExtras->getObjnameByIdx(i);
					H5G_obj_t t = grpExtras->getObjTypeByIdx(i);
					if (t != H5G_obj_t::H5G_GROUP) continue;

					boost::shared_ptr<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> >
						mextra(new Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>);
					readDatasetEigen<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic>, Group>
						(grpExtras, name.c_str(), *(mextra));
					shp->latticeExtras[name] = mextra;
				}


				return true;
			}


		}
	}
}
namespace Ryan_Debug {
	namespace registry
	{
		using std::shared_ptr;
		using namespace rtmath::plugins::hdf5;

		template<>
		shared_ptr<IOhandler>
			read_file_type_multi<rtmath::ddscat::shapefile::shapefile>
			(shared_ptr<IOhandler> sh, shared_ptr<IO_options> opts,
				boost::shared_ptr<rtmath::ddscat::shapefile::shapefile > s,
				std::shared_ptr<const Ryan_Debug::registry::collectionTyped<rtmath::ddscat::shapefile::shapefile> > filter)
		{
			std::string filename = opts->filename();
			IOhandler::IOtype iotype = opts->getVal<IOhandler::IOtype>("iotype", IOhandler::IOtype::READONLY);
			//IOhandler::IOtype iotype = opts->iotype();
			std::string key = opts->getVal<std::string>("key");
			using std::shared_ptr;
			using namespace H5;
			Exception::dontPrint();
			std::shared_ptr<hdf5_handle> h = registry::construct_handle
				<registry::IOhandler, hdf5_handle>(
					sh, PLUGINID, [&]() {return std::shared_ptr<hdf5_handle>(
						new hdf5_handle(filename.c_str(), iotype)); });

			shared_ptr<Group> grpHashes = openGroup(h->file, "Hashed");
			if (!grpHashes) RDthrow(Ryan_Debug::error::xMissingKey())
				<< Ryan_Debug::error::key("Hashed")
				<< Ryan_Debug::error::hash(key);

			shared_ptr<Group> grpHash = openGroup(grpHashes, key.c_str());
			shared_ptr<Group> grpShape = openGroup(grpHash, "Shape");
			if (!grpShape) RDthrow(Ryan_Debug::error::xMissingFile())
				<< Ryan_Debug::error::key("Shape")
				<< Ryan_Debug::error::hash(key);
			read_hdf5_shaperawdata(grpShape, s);

			return h;
		}

		/// \brief Reads in all shapefile entries in file.
		/// \todo Add opts selector information to eventually narrow the returned data.
		template<>
		std::shared_ptr<IOhandler>
			read_file_type_iterate<rtmath::ddscat::shapefile::shapefile>
			(std::shared_ptr<IOhandler> sh, std::shared_ptr<IO_options> opts,
				std::function<void(
					std::shared_ptr<Ryan_Debug::registry::IOhandler>,
					std::shared_ptr<Ryan_Debug::registry::IO_options>,
					boost::shared_ptr<rtmath::ddscat::shapefile::shapefile>
					) > s,
				//std::vector<boost::shared_ptr<rtmath::ddscat::shapefile::shapefile> > &s,
				std::shared_ptr<const Ryan_Debug::registry::collectionTyped<rtmath::ddscat::shapefile::shapefile> > filter)
		{
			std::string filename = opts->filename();
			IOhandler::IOtype iotype = opts->getVal<IOhandler::IOtype>("iotype", IOhandler::IOtype::READONLY);
			//IOhandler::IOtype iotype = opts->iotype();
			std::string key = opts->getVal<std::string>("key", "");
			using std::shared_ptr;
			using namespace H5;
			Exception::dontPrint();
			std::shared_ptr<hdf5_handle> h = registry::construct_handle
				<registry::IOhandler, hdf5_handle>(
					sh, PLUGINID, [&]() {return std::shared_ptr<hdf5_handle>(
						new hdf5_handle(filename.c_str(), iotype)); });

			shared_ptr<Group> grpHashes = openGroup(h->file, "Hashed");
			if (grpHashes)
			{
				hsize_t sz = grpHashes->getNumObjs();
				//s.reserve(s.size() + sz);
				for (hsize_t i = 0; i < sz; ++i)
				{
					std::string hname = grpHashes->getObjnameByIdx(i);
					H5G_obj_t t = grpHashes->getObjTypeByIdx(i);
					if (t != H5G_obj_t::H5G_GROUP) continue;
					if (key.size() && key != hname) continue;

					shared_ptr<Group> grpHash = openGroup(grpHashes, hname.c_str());
					if (!grpHash) continue; // Should never happen
					shared_ptr<Group> grpShape = openGroup(grpHash, "Shape");
					if (grpShape)
					{
						boost::shared_ptr<rtmath::ddscat::shapefile::shapefile>
							shp = rtmath::ddscat::shapefile::shapefile::generate();
						read_hdf5_shaperawdata(grpShape, shp);
						if (filter) {
							if (filter->filter(shp.get()))
								s(sh, opts, shp);
							//s.push_back(shp);
						}
						else s(sh, opts, shp); //s.push_back(shp);
					}
				}
			}

			return h;
		}
	}
}
