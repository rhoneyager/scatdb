#include "../scatdb/defs.hpp"
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "../scatdb/error.hpp"
#include "../scatdb/scatdb.hpp"
#include "../scatdb/export-hdf5.hpp"
#include <hdf5.h>
#include <H5Cpp.h>


namespace scatdb {

	void db::readDBhdf5(std::shared_ptr<db> res,
		const char* dbfile, const char* hdfinternalpath) {
		if (!dbfile) SDBR_throw(scatdb::error::error_types::xBadInput)
			.add<std::string>("Reason", "dbfile is null");
		std::string sinternal;
		if (hdfinternalpath) sinternal = std::string(hdfinternalpath);
		else sinternal = "scatdb";

		using namespace H5;
		Exception::dontPrint();
		std::shared_ptr<H5::H5File> file
			= std::shared_ptr<H5File>(new H5File(dbfile, H5F_ACC_RDONLY));
		if (!scatdb::plugins::hdf5::groupExists(file, sinternal.c_str()))
			SDBR_throw(scatdb::error::error_types::xMissingKey)
			.add<std::string>("Reason", "HDF5 file does not have the desired group.")
			.add<std::string>("hdfinternalpath", sinternal);;
		auto grp = scatdb::plugins::hdf5::openGroup(file, sinternal.c_str());

		if (!scatdb::plugins::hdf5::datasetExists(grp, "floatMat"))
			SDBR_throw(scatdb::error::error_types::xMissingKey)
			.add<std::string>("Reason", "HDF5 path does not have the desired dataset.")
			.add<std::string>("name", "floatMat");
		if (!scatdb::plugins::hdf5::datasetExists(grp, "intMat"))
			SDBR_throw(scatdb::error::error_types::xMissingKey)
			.add<std::string>("Reason", "HDF5 path does not have the desired dataset.")
			.add<std::string>("name", "intMat");
		scatdb::plugins::hdf5::readDatasetEigen<
			Eigen::Matrix<float, Eigen::Dynamic, data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS, Eigen::RowMajor>, Group>
			(grp, "floatMat", res->floatMat);
		scatdb::plugins::hdf5::readDatasetEigen<
			Eigen::Matrix<uint64_t, Eigen::Dynamic, data_entries::SDBR_NUM_DATA_ENTRIES_INTS>, Group>
			(grp, "intMat", res->intMat);

		//res->floatMat.resize(numLines, data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS);
		//res->intMat.resize(numLines, data_entries::SDBR_NUM_DATA_ENTRIES_INTS);
	}

	void db::data_stats::writeHDF5File(std::shared_ptr<H5::Group> grp) const {
		using namespace H5;
		Exception::dontPrint();

		auto pfloats = scatdb::plugins::hdf5::make_plist((int)floatStats.rows(), data_entries::SDBR_NUM_DATA_ENTRIES_STATS);
		auto dfloats = scatdb::plugins::hdf5::addDatasetEigen<StatsFloatType, Group>
			(grp, "floatStats", floatStats, pfloats);
		//scatdb::plugins::hdf5::addColNames(dfloats, data_entries::SDBR_NUM_DATA_ENTRIES_STATS,
		//	[](int i) -> std::string { return std::string(data_entries::stringifyStats(i)); });
		scatdb::plugins::hdf5::addNames(dfloats, "ROW_",
			data_entries::SDBR_NUM_DATA_ENTRIES_STATS,
			[](int i) -> std::string { return std::string(data_entries::stringifyStats(i)); });

		scatdb::plugins::hdf5::addNames(dfloats, "COLUMN_",
			data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS,
			[](int i) -> std::string { return std::string(data_entries::stringify<float>(i)); });

		scatdb::plugins::hdf5::addAttr<int>(grp, "Count", (int) count);
	}

	void db::writeHDFfile(std::shared_ptr<H5::Group> grp) const {
		using namespace H5;
		Exception::dontPrint();
		
		auto pfloats = scatdb::plugins::hdf5::make_plist((int)floatMat.rows(), data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS);
		auto pints = scatdb::plugins::hdf5::make_plist((int)intMat.rows(), data_entries::SDBR_NUM_DATA_ENTRIES_INTS);


		auto dfloats = scatdb::plugins::hdf5::addDatasetEigen<
			Eigen::Matrix<float, Eigen::Dynamic, data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS>, Group>
			(grp, "floatMat", floatMat, pfloats);
		scatdb::plugins::hdf5::addColNames(dfloats, data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS,
			[](int i) -> std::string { return std::string(data_entries::stringify<float>(i)); });
		auto dints = scatdb::plugins::hdf5::addDatasetEigen<
			Eigen::Matrix<uint64_t, Eigen::Dynamic, data_entries::SDBR_NUM_DATA_ENTRIES_INTS>, Group>
			(grp, "intMat", intMat, pints);
		scatdb::plugins::hdf5::addColNames(dints, data_entries::SDBR_NUM_DATA_ENTRIES_INTS,
			[](int i) -> std::string { return std::string(data_entries::stringify<int>(i)); });


		/// \todo Write flakeTypes table
		{
			std::set<int> known_cats;

			int prev = -1;
			for (int i = 0; i < intMat.rows(); ++i) {
				const int &val = (int) intMat(i, data_entries::SDBR_FLAKETYPE);
				if (val != prev) {
					prev = val;
					known_cats.emplace(val);
				}
			}

			struct catdata {
				int id;
				const char* description;
			};
			std::vector<catdata> sdata(known_cats.size());
			size_t i = 0;
			for (const auto &t : known_cats)
			{
				sdata[i].id = t;
				sdata[i].description = data_entries::getCategoryDescription((int)t);
				++i;
			}
			i = 0;

			hsize_t dim[1] = { sdata.size() };
			DataSpace space(1, dim);
			CompType sType(sizeof(catdata));
			H5::StrType strtype(0, H5T_VARIABLE);

			sType.insertMember("ID", HOFFSET(catdata, id), PredType::NATIVE_INT);
			sType.insertMember("Method", HOFFSET(catdata, description), strtype);

			std::shared_ptr<DataSet> g(new DataSet(grp->createDataSet("Categories", sType, space)));
			g->write(sdata.data(), sType);
		}

		/// \note Example bugfix for string attribute write...
		/// String attr hdf5 override was not being hit. Instead, cast to c_str and give
		/// an actual size instead of H5T_VARIABLE. The other way to fix is to call
		/// addAttr<const char*,...>.
		//const std::string test("test"), testb("test");
		//scatdb::plugins::hdf5::addAttr(grp, "aname", test);
		//std::shared_ptr<H5::AtomType> vls_type = 
		//	std::shared_ptr<H5::AtomType>(new H5::StrType(0, H5T_VARIABLE));
		//H5::DataSpace att_space(H5S_SCALAR);
		//H5::Attribute attr = grp->createAttribute("bname", *vls_type, att_space);
		//attr.write(*vls_type, testb.c_str());
		//attr.write(*vls_type, testb);
	}
	void db::writeHDFfile(const char* filename,
		SDBR_write_type wt, const char* hdfinternalpath) const {
		scatdb::plugins::hdf5::useZLIB(true);
		std::string sinternal;
		if (!filename) SDBR_throw(scatdb::error::error_types::xBadInput)
			.add<std::string>("Reason", "dbfile is null");
		if (hdfinternalpath) sinternal = std::string(hdfinternalpath);
		else sinternal = "scatdb";

		using namespace H5;
		Exception::dontPrint();
		std::shared_ptr<H5::H5File> file;
		if (wt == SDBR_write_type::SDBR_CREATE)
			file = std::shared_ptr<H5File>(new H5File(filename, H5F_ACC_CREAT));
		else if (wt == SDBR_write_type::SDBR_READWRITE)
			file = std::shared_ptr<H5File>(new H5File(filename, H5F_ACC_RDWR));
		else if (wt == SDBR_write_type::SDBR_TRUNCATE)
			file = std::shared_ptr<H5File>(new H5File(filename, H5F_ACC_TRUNC));
		else SDBR_throw(scatdb::error::error_types::xUnsupportedIOaction)
			.add<std::string>("Reason", "write type unhandled in code");

		if (scatdb::plugins::hdf5::groupExists(file, sinternal.c_str()))
			SDBR_throw(scatdb::error::error_types::xKeyExists)
			.add<std::string>("Reason", "HDF5 file already has the desired group.")
			.add<std::string>("hdfinternalpath", sinternal);;
		auto grp = scatdb::plugins::hdf5::openOrCreateGroup(file, sinternal.c_str());
		writeHDFfile(grp);
	}
}

