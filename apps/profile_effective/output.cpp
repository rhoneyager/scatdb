#include "parser.hpp"
#include "../../scatdb/error.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "../../scatdb/export-hdf5.hpp"
#include <hdf5.h>
#include <H5Cpp.h>
#include <boost/filesystem.hpp>

namespace scatdb {
	namespace profiles {
		void forward_conc_table::writeText(const char* filename) const {
			using namespace std;
			string sfile(filename);
			ofstream out(sfile.c_str());

			// Write the header
			out << "Heymsfield Round 1" << endl;
			out << defs::stringify(this->getParticleTypes()) << endl;
			out << "Temp: " << this->getTempC() << " degC" << endl;
			out << "Bin Low\tBin Mid\tBin High\tBin Width\tConc" << endl;
			for (int i = 0; i < this->data->rows(); ++i) {
				for (int j = 0; j < data->cols(); ++j) {
					if (j) out << "\t";
					out << (*data)(i, j);
				}
				out << endl;
			}
		}

		void forward_conc_table::writeHDF5File(std::shared_ptr<H5::Group> grp,
			const char* hdfinternalpath) const {

			if (!hdfinternalpath) SDBR_throw(scatdb::error::error_types::xBadInput)
				.add<std::string>("Reason", "hdfinternalpath is null");
			std::string sinternal = std::string(hdfinternalpath);

			using namespace H5;
			//Exception::dontPrint();

			auto pfloats = scatdb::plugins::hdf5::make_plist((int)data->rows(), (int)data->cols());
			std::ostringstream dsetname;
			dsetname << "Case_" << sinternal;
			std::string sd = dsetname.str();
			if (scatdb::plugins::hdf5::datasetExists(grp, sd.c_str())) {
				grp->unlink(sd);
			}
			auto dfloats = scatdb::plugins::hdf5::addDatasetEigen<
				Eigen::Matrix<float, Eigen::Dynamic, defs::NUM_COLUMNS>, Group>
				(grp, sd.c_str(), *(data.get()), pfloats);
			scatdb::plugins::hdf5::addColNames(dfloats, defs::NUM_COLUMNS,
				[](int i) -> std::string {
				switch (i) {
				case defs::BIN_LOWER:
					return "BIN_LOWER (um)";
				case defs::BIN_MID:
					return "BIN_MID (um)";
				case defs::BIN_UPPER:
					return "BIN_UPPER (um)";
				case defs::BID_WIDTH:
					return "BIN_WIDTH (um)";
				case defs::CONCENTRATION:
					return "CONCENTRATION (1/m^4)";
				default:
					return "UNKNOWN";
				}
				return "";
			});
			std::string casetype = defs::stringify(this->getParticleTypes());
			scatdb::plugins::hdf5::addAttr(dfloats, "Case", casetype);
			scatdb::plugins::hdf5::addAttr(dfloats, "TempC", this->getTempC());

		}
		void forward_conc_table::writeHDF5File(const char* filename,
			const char* hdfinternalpath) const {
			scatdb::plugins::hdf5::useZLIB(true);
			if (!filename) SDBR_throw(scatdb::error::error_types::xBadInput)
				.add<std::string>("Reason", "dbfile is null");
			if (!hdfinternalpath) SDBR_throw(scatdb::error::error_types::xBadInput)
				.add<std::string>("Reason", "hdfinternalpath is null");
			std::string sinternal = std::string(hdfinternalpath);

			using namespace H5;
			//Exception::dontPrint();
			std::shared_ptr<H5::H5File> file;
			boost::filesystem::path p(filename);
			if (boost::filesystem::exists(p))
				file = std::shared_ptr<H5File>(new H5File(filename, H5F_ACC_RDWR));
			else
				file = std::shared_ptr<H5File>(new H5File(filename, H5F_ACC_TRUNC));

			if (scatdb::plugins::hdf5::groupExists(file, sinternal.c_str()))
				SDBR_throw(scatdb::error::error_types::xKeyExists)
				.add<std::string>("Reason", "HDF5 file already has the desired group.")
				.add<std::string>("hdfinternalpath", sinternal);;
			auto grp = scatdb::plugins::hdf5::openOrCreateGroup(file, "radar_insitu_colocations");
			writeHDF5File(grp, hdfinternalpath);
		}

		forward_set_p forward_conc_table::readHDF5File(
			const char* dbfile) {
			std::shared_ptr<std::vector<forward_p> > res(new std::vector<forward_p>);
			if (!dbfile) SDBR_throw(scatdb::error::error_types::xBadInput)
				.add<std::string>("Reason", "dbfile is null");

			using namespace H5;
			const std::string sinternal = "radar_insitu_colocations";
			//Exception::dontPrint();
			std::shared_ptr<H5::H5File> file
				= std::shared_ptr<H5File>(new H5File(dbfile, H5F_ACC_RDONLY));
			if (!scatdb::plugins::hdf5::groupExists(file, sinternal.c_str()))
				SDBR_throw(scatdb::error::error_types::xMissingKey)
				.add<std::string>("Reason", "HDF5 file does not have the desired group.")
				.add<std::string>("hdfinternalpath", sinternal);;
			auto grp = scatdb::plugins::hdf5::openGroup(file, sinternal.c_str());

			hsize_t numsets = grp->getNumObjs();
			for (hsize_t i = 0; i < numsets; ++i) {
				H5G_obj_t typ = grp->getObjTypeByIdx(i);
				if (typ != H5G_DATASET) continue;

				std::string scase = grp->getObjnameByIdx(i);

				std::shared_ptr<forward_conc_table> leg(new forward_conc_table);
				std::shared_ptr<tbl_t> dnew(new tbl_t);
				auto dset = scatdb::plugins::hdf5::readDatasetEigen< tbl_t, Group>(grp, scase.c_str(), *(dnew.get()));

				leg->data = dnew;
				scatdb::plugins::hdf5::readAttr(dset, "TempC", leg->tempC);
				std::string casenm;
				scatdb::plugins::hdf5::readAttr(dset, "Case", casenm);
				leg->pt = defs::enumify(casenm);

				res->push_back(leg);
			}

			return res;
		}
	}
}