#include "../../scatdb/defs.hpp"
/// This is a program that reads datasets from an hdf5 file.
/// It takes the desired datasets and writes them into the
/// specified text file or directory.
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <exception>
#include <complex>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "parser.hpp"
#include "../../scatdb/logging.hpp"
#include "../../scatdb/error.hpp"
#include "../../scatdb/debug.hpp"
#include "../../scatdb/units/units.hpp"
#include "../../scatdb/refract/refract.hpp"
#include "../../scatdb/scatdb.hpp"
#include "../../scatdb/export-hdf5.hpp"
#include "../../scatdb/splitSet.hpp"
#include <hdf5.h>
#include <H5Cpp.h>
#include <H5ArrayType.h>

class ResultsTable {
public:
	struct StringRowData {
		std::string profileName, profileDesc,
			flakeTypeName, flakeTypeData;
	};
	struct BandData {
		std::string bandName, bandVals;
	};
private:
	typedef Eigen::Array<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> FloatType;
	FloatType data;
	size_t numProfiles, numFlakeTypes, numFreqs, numRows,
		numFloatCols, numDPRcols, numFreqCols;
	std::vector<StringRowData> stringRowData;
	std::vector<BandData> bandData;
	std::vector<std::string> dprNames;

	static size_t getFactorial(size_t n, size_t lower) {
		size_t res = 1;
		while (n >= lower) {
			res *= n;
			n--;
		}
		return res;
	}
	void resize(size_t numProfiles, size_t numFlakeTypes, size_t numFreqs) {
		this->numProfiles = numProfiles;
		this->numFlakeTypes = numFlakeTypes;
		this->numFreqs = numFreqs;

		numRows = numProfiles * numFlakeTypes;
		numFreqCols = 3 * numFreqs;
		numDPRcols = getFactorial(numFreqs, 3);
		numFloatCols = numFreqCols + numDPRcols;

		data.resize((int)numRows, (int)numFloatCols);
		data.setZero();
		stringRowData.resize(numRows);
		bandData.resize(numFreqs);
		dprNames.resize(numDPRcols);
	}

	ResultsTable(size_t numProfiles, size_t numFlakeTypes, size_t numFreqs) {
		resize(numProfiles, numFlakeTypes, numFreqs);
	}
public:
	static std::shared_ptr<ResultsTable> generate(size_t numProfiles, size_t numFlakeTypes, size_t numFreqs) {
		std::shared_ptr<ResultsTable> res(new ResultsTable(numProfiles, numFlakeTypes, numFreqs));
		return res;
	}
	size_t getRowId(size_t profileNum, size_t ftNum) const {
		size_t res = (profileNum * numFlakeTypes) + ftNum;
		return res;
	}
	StringRowData& getStringRowData(size_t profileNum, size_t ftNum) {
		size_t row = getRowId(profileNum, ftNum);
		return stringRowData.at(row);
	}
	void setStrings(size_t profileNum, size_t ftNum,
		const std::string &pname, const std::string &pdesc,
		const std::string &ftname, const std::string &ftdesc) {
		size_t row = getRowId(profileNum, ftNum);
		stringRowData.at(row).profileName = pname;
		stringRowData.at(row).profileDesc = pdesc;
		stringRowData.at(row).flakeTypeName = ftname;
		stringRowData.at(row).flakeTypeData = ftdesc;
	}
	void setFreq(size_t freqId, const std::string &bandName, const std::string &bandVals) {
		bandData.at(freqId).bandName = bandName;
		bandData.at(freqId).bandVals = bandVals;
	}
	void setZeData(size_t profileNum, size_t ftNum, size_t freqId, float val) {
		size_t row = getRowId(profileNum, ftNum);
		data(row, 3 * freqId) = val;
	}
	std::string getFloatColName(size_t col) {
		bool isDPR = (col >= numFreqCols) ? true : false;
		if (isDPR) {
			return dprNames.at(col - numFreqCols);
		}
		else {
			size_t freqId = col / 3;
			std::string bn = bandData.at(freqId).bandName;
			std::string res("Ze_");
			res.append(bn);
			size_t sel = col % 3;
			if (sel == 0) res.append("_m^3");
			if (sel == 1) res.append("_mm^6.m^-3");
			if (sel == 2) res.append("_dBZe");
			return res;
		}
	}
	void finalize() {
		/// Convert the Ze data fields into appropriate units.
		for (size_t i = 0; i < numRows; ++i) {
			for (size_t j = 0; j < numFreqs; ++j) {
				size_t colZem = j * 3;
				size_t colZemm = colZem + 1;
				size_t coldbZe = colZem + 2;
				data(i, colZemm) = 1.e18f * data(i, colZem);
				data(i, coldbZe) = 10.f * std::log10f(data(i, colZemm));
			}
		}
		/// Determine all dpr names and calculate all dprs.
		size_t k = 0;
		for (size_t i = 0; i < numFreqs; ++i) {
			std::string bni = bandData.at(i).bandName;
			for (size_t j = i + 1; j < numFreqs; ++j) {
				std::string bnj = bandData.at(j).bandName;
				std::string res("DPR_");
				res.append(bni);
				res.append("_");
				res.append(bnj);
				res.append("_dBZe");
				dprNames.at(k) = res;

				// Calculate dprs.
				for (size_t r = 0; r < numRows; ++r) {
					size_t coldbZe1 = (i * 3) + 2;
					size_t coldbZe2 = (j * 3) + 2;
					data(r, (numFreqCols)+k) = data(r, coldbZe1) - data(r, coldbZe2);
					/*std::cerr << "Calculating dpr for " << res <<
					" row " << r << " band 1 - " << data(r, coldbZe1) << " , band 2 - " << data(r, coldbZe2)
					<< " , dpr - " << data(r, (numFreqCols) + k) << std::endl;
					*/
				}
				++k;
			}
		}
	}
	void writeTable(std::shared_ptr<H5::Group> base) {

		// Generate an HDF5-compatible structure
		const size_t rowmembytes = (sizeof(const char*) * 4) + (sizeof(float)*numFloatCols);
		const size_t charbytes = sizeof(const char*);
		const size_t offsetStrings[4] = { 0, charbytes, 2 * charbytes, 3 * charbytes };
		const size_t offSetFloatStart = 4 * charbytes;
		auto offsetFloat = [&charbytes](size_t floatnum) -> size_t {
			return (4 * charbytes) + (floatnum * (sizeof(float)));
		};
		auto offsetRow = [&rowmembytes](size_t row) -> size_t {
			return row * rowmembytes;
		};
		const size_t totalMemBytes = rowmembytes * numRows;
		std::unique_ptr<unsigned char[]> byteArray(new unsigned char[totalMemBytes]);
		std::fill_n(byteArray.get(), totalMemBytes, 0);
		auto copyStrPtr = [&byteArray](size_t offset, const char* ptr) {
			const char *ccstr[1] = { ptr };
			std::memcpy(byteArray.get() + offset, ccstr, sizeof(const char*));
		};
		auto copyFloat = [&byteArray](size_t offset, float val) {
			std::memcpy(byteArray.get() + offset, &val, sizeof(float));
		};
		for (size_t i = 0; i < numRows; ++i) {
			copyStrPtr(offsetRow(i) + offsetStrings[0], stringRowData.at(i).profileName.c_str());
			copyStrPtr(offsetRow(i) + offsetStrings[1], stringRowData.at(i).profileDesc.c_str());
			copyStrPtr(offsetRow(i) + offsetStrings[2], stringRowData.at(i).flakeTypeName.c_str());
			copyStrPtr(offsetRow(i) + offsetStrings[3], stringRowData.at(i).flakeTypeData.c_str());
			for (size_t j = 0; j < numFloatCols; ++j)
				copyFloat(offsetRow(i) + offsetFloat(j), data(i, j));
		}

		using namespace H5;
		hsize_t dim[1] = { numRows };
		DataSpace space(1, dim);
		CompType sType(rowmembytes);
		H5::StrType strtype(0, H5T_VARIABLE);
		hsize_t fdim[1] = { (hsize_t)numFloatCols };

		sType.insertMember("Profile_Name", offsetStrings[0], strtype);
		sType.insertMember("Profile_Description", offsetStrings[1], strtype);
		sType.insertMember("FlakeType_Name", offsetStrings[2], strtype);
		sType.insertMember("FlakeType_IDs", offsetStrings[3], strtype);
		for (size_t j = 0; j < numFloatCols; ++j) {
			sType.insertMember(getFloatColName(j), offsetFloat(j), H5::PredType::NATIVE_FLOAT);
		}


		/*
		H5::ArrayType arrfloattype(H5::PredType::NATIVE_FLOAT, 1, fdim);

		sType.insertMember("Profile_Name", HOFFSET(hstrdata, cProfileName), strtype);
		sType.insertMember("Profile_Description", HOFFSET(hstrdata, cProfileDesc), strtype);
		sType.insertMember("FlakeType_Name", HOFFSET(hstrdata, cFtName), strtype);
		sType.insertMember("FlakeType_IDs", HOFFSET(hstrdata, cFtDesc), strtype);
		//sType.insertMember("Floats", HOFFSET(hstrdata, floats), arrfloattype);
		//sType.insertMember("Profile_Name", HOFFSET(sdata, id), PredType::NATIVE_INT);
		*/
		std::shared_ptr<DataSet> g(new DataSet(base->createDataSet("Summary_Table", sType, space)));
		g->write(byteArray.get(), sType);
	}
};

int main(int argc, char** argv) {
	using namespace std;
	try {
		using namespace scatdb;
		using namespace std;
		namespace po = boost::program_options;
		po::options_description desc("Allowed options"), cmdline("Command-line options"),
			config("Config options"), hidden("Hidden options"), oall("all options");

		scatdb::debug::add_options(cmdline, config, hidden);

		cmdline.add_options()
			("help,h", "produce help message")
			("profiles", po::value<string>(), "hdf5 file containing atmospheric profiles")
			("profiles-path", po::value<vector<string> >(),
				"Path to the folder containing the profiles inside the HDF5 file. "
				"Can be specified multiple times.")
			("scatdb-path", po::value<string>()->default_value("scatdb"), "Path to the group "
				"in the HDF5 file that contains the scattering database. To change the database file itself, use --dbfile.")
			("frequencies", po::value<vector<string> >()->multitoken(),
				"Specify frequency ranges to filter and band name. Specify as [band name (e.g. Ka)]+low/high.")
			("filter", po::value<vector<string> >()->multitoken(),
				"Specify shape id, shape categories to use and temperatures. Separate the three fields using + signs. "
				"Example format is catname+catnums+temps. Can specify multiple times.")
			("output", po::value<string>(), "Output file path")
			("verbosity,v", po::value<int>()->default_value(1), "Change level of detail in "
				"the output file. Higher number means more intermediate results will be writen.")
			;
		desc.add(cmdline).add(config);
		oall.add(cmdline).add(config).add(hidden);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
			options(oall).run(), vm);
		po::notify(vm);

		scatdb::debug::process_static_options(vm);

		auto doHelp = [&](const std::string& s)
		{
			cout << s << endl;
			cout << desc << endl;
			exit(3);
		};

		if (vm.count("help") || vm.size() == 0) doHelp("");

		// Read the profiles
		string profname;
		if (vm.count("profiles")) profname = vm["profiles"].as<string>();
		else doHelp("Must specify profile path");
		auto allprofiles = scatdb::profiles::forward_conc_table::readHDF5File(profname.c_str());

		// Read the scattering database
		using namespace scatdb;
		string sdbname;
		db::findDB(sdbname);
		auto sdb = db::loadDB(sdbname.c_str());


		struct filtered_info {
			string sName;
			string sCats;
			string sTemps;
			filtered_info(string scomb) {
				// Split into categories and temperature ranges automatically.
				vector<string> vsplit;
				scatdb::splitSet::splitVector(scomb, vsplit, '+');
				if (vsplit.size() == 3) {
					sName = vsplit[0];
					sCats = vsplit[1];
					sTemps = vsplit[2];
				}
				else {
					SDBR_throw(scatdb::error::xError(scatdb::error::error_types::xBadInput))
						.add<string>("Reason", "Cannot split category name/ids/temperature information.")
						.add<string>("Problem-String", scomb);
				}
			}
		};
		vector<filtered_info> filters;
		if (!vm.count("filter")) {
			filters.push_back(std::move(filtered_info("Nowell_Rounded+5,6,7,8,20+262/264")));

			filters.push_back(std::move(filtered_info("Nowell_AR06+5,6,7,8,21,22+262/264")));
			filters.push_back(std::move(filtered_info("Ori_SAM+30+270/280")));
			filters.push_back(std::move(filtered_info("Leinonen_and_Szyrmer_2015_A_0.0+40+-1000/1000")));
			filters.push_back(std::move(filtered_info("Leinonen_and_Szyrmer_2015_A_0.1+41+-1000/1000")));
			filters.push_back(std::move(filtered_info("Leinonen_and_Szyrmer_2015_A_0.2+42+-1000/1000")));
			filters.push_back(std::move(filtered_info("Leinonen_and_Szyrmer_2015_A_0.5+43+-1000/1000")));
			filters.push_back(std::move(filtered_info("Leinonen_and_Szyrmer_2015_A_1.0+44+-1000/1000")));
			filters.push_back(std::move(filtered_info("Leinonen_and_Szyrmer_2015_A_2.0+45+-1000/1000")));
			filters.push_back(std::move(filtered_info("Leinonen_and_Szyrmer_2015_B_0.1+46+-1000/1000")));
			filters.push_back(std::move(filtered_info("Leinonen_and_Szyrmer_2015_B_0.2+47+-1000/1000")));
			filters.push_back(std::move(filtered_info("Leinonen_and_Szyrmer_2015_B_0.5+48+-1000/1000")));
			filters.push_back(std::move(filtered_info("Leinonen_and_Szyrmer_2015_B_1.0+49+-1000/1000")));
			filters.push_back(std::move(filtered_info("Leinonen_and_Szyrmer_2015_B_2.0+50+-1000/1000")));
			filters.push_back(std::move(filtered_info("Leinonen_and_Szyrmer_2015_C+51+-1000/1000")));
			filters.push_back(std::move(filtered_info("Tyynela_Chandrasekhar_2014_Fernlike_Dendrite+60+-1000/1000")));
			filters.push_back(std::move(filtered_info("Tyynela_Chandrasekhar_2014_Needle+61+-1000/1000")));
			filters.push_back(std::move(filtered_info("Tyynela_Chandrasekhar_2014_Rosette+62+-1000/1000")));
			filters.push_back(std::move(filtered_info("Tyynela_Chandrasekhar_2014_Stellar_Dendrite+63+-1000/1000")));

		}
		else {
			vector<string> sfilters = vm["filter"].as<vector<string> >();
			for (const auto &s : sfilters) {
				filters.push_back(std::move(filtered_info(s)));
			}
		}

		struct freq_info {
			string sBandName;
			string sRange;
			freq_info(string scomb) {
				// Split into categories and temperature ranges automatically.
				vector<string> vsplit;
				scatdb::splitSet::splitVector(scomb, vsplit, '+');
				if (vsplit.size() == 2) {
					sBandName = vsplit[0];
					sRange = vsplit[1];
				}
				else {
					SDBR_throw(scatdb::error::xError(scatdb::error::error_types::xBadInput))
						.add<string>("Reason", "Cannot split band name/frequency range information.")
						.add<string>("Problem-String", scomb);
				}
			}
		};
		vector<freq_info > freqranges;
		if (!vm.count("frequencies")) {
			freqranges.push_back(std::move(freq_info("Ku+13/14")));
			freqranges.push_back(std::move(freq_info("Ka+35/36")));
		}
		else {
			vector<string> sfilters = vm["frequencies"].as<vector<string> >();
			for (const auto &s : sfilters) {
				freqranges.push_back(std::move(freq_info(s)));
			}
		}


		string sout;
		if (vm.count("output")) sout = vm["output"].as<string>();
		else doHelp("Need to specify an output file");
		using namespace H5;
		//Exception::dontPrint();
		std::shared_ptr<H5::H5File> file;
		boost::filesystem::path p(sout);
		if (boost::filesystem::exists(p))
			file = std::shared_ptr<H5File>(new H5File(sout, H5F_ACC_TRUNC));
		else
			file = std::shared_ptr<H5File>(new H5File(sout, H5F_ACC_TRUNC));
		auto base = scatdb::plugins::hdf5::openOrCreateGroup(file, "output");
		auto fsbase = scatdb::plugins::hdf5::openOrCreateGroup(base, "scatdb_initial");
		sdb->writeHDFfile(fsbase);

		auto ft = ResultsTable::generate(allprofiles->size(), filters.size(), freqranges.size());

		// Iterate over each collection of flaketypes
		int filtnum = 0;
		for (const auto &filter : filters) {
			string filtname("group_");
			std::ostringstream filtnumout;
			filtnumout << std::right << std::setw(((int)std::log10(filters.size()) + 1))
				<< std::setfill('0') << filtnum;
			//filtname.append(boost::lexical_cast<string>(filtnum));
			filtname.append(filtnumout.str());

			auto filtbase = scatdb::plugins::hdf5::openOrCreateGroup(base, filtname.c_str());
			scatdb::plugins::hdf5::addAttr<string>(filtbase, "Filtered_Particle_Name", filter.sName);
			scatdb::plugins::hdf5::addAttr<string>(filtbase, "Filtered_Particle_Types", filter.sCats);
			scatdb::plugins::hdf5::addAttr<string>(filtbase, "Filtered_Temperatures", filter.sTemps);

			auto f = filter::generate();
			f->addFilterInt(db::data_entries::SDBR_FLAKETYPE, filter.sCats);
			f->addFilterFloat(db::data_entries::SDBR_TEMPERATURE_K, filter.sTemps);
			auto db_ros = f->apply(sdb);

			// Profile information
			std::cerr << "Working on case " << filtname << " with name " << filter.sName
				<< " and cats " << filter.sCats << " and temps " << filter.sTemps
				<< ", with " << db_ros->intMat.rows() << " rows. Sdb has " << sdb->intMat.rows() << " rows." << std::endl;
			auto fsros = scatdb::plugins::hdf5::openOrCreateGroup(filtbase, "scatdb_filtered");
			if (db_ros->intMat.rows() == 0) {
				std::cerr << filtname << " with filter cats " << filter.sCats << " and temps " << filter.sTemps
					<< " has no data." << std::endl;
				filtnum++;
				continue;
			}
			db_ros->writeHDFfile(fsros);

			int freqnum = 0;
			// Iterate over each profile and each set of frequencies
			for (const auto &freq : freqranges) {
				auto ff = filter::generate();
				ff->addFilterFloat(db::data_entries::SDBR_FREQUENCY_GHZ, freq.sRange);
				auto db_ros_f = ff->apply(db_ros);
				if (db_ros_f->floatMat.rows() == 0) {
					std::cerr << "Error: At frequency band " << freq.sBandName << " / range " << freq.sRange << ", there were "
						<< "no data points available in the selected dataset." << std::endl;
					freqnum++;
					continue;
				}
				ft->setFreq(freqnum, freq.sBandName, freq.sRange);

				auto db_ros_f_sorted = db_ros_f; // db_ros_f->sort(db::data_entries::MAX_DIMENSION_MM);

				auto fgrp = scatdb::plugins::hdf5::openOrCreateGroup(filtbase, freq.sBandName.c_str());

				if (!scatdb::plugins::hdf5::groupExists(fgrp, "freq_temp_scatdb_unsorted")) {
					auto fshpun = scatdb::plugins::hdf5::openOrCreateGroup(fgrp, "freq_temp_scatdb_unsorted");

					db_ros_f->writeHDFfile(fshpun);
				}
				//auto fshp = scatdb::plugins::hdf5::openOrCreateGroup(fgrp, "freq_temp_scatdb_sorted");
				//db_ros_f_sorted->writeHDF5File(fshp);

				auto db_ros_f_sorted_stats = db_ros_f_sorted->getStats();
				if (!scatdb::plugins::hdf5::groupExists(fgrp, "Stats")) {
					auto o_db_ros_f_sorted_stats = scatdb::plugins::hdf5::openOrCreateGroup(fgrp, "Stats");
					db_ros_f_sorted_stats->writeHDF5File(o_db_ros_f_sorted_stats);
				}

				// Get frequency
				float freq_ghz = db_ros_f_sorted_stats->floatStats(db::data_entries::SDBR_MEDIAN, db::data_entries::SDBR_FREQUENCY_GHZ);
				// Get wavelength
				auto specConv_m = scatdb::units::conv_spec::generate("GHz", "m");
				float wvlen_m = (float)specConv_m->convert(freq_ghz);
				scatdb::plugins::hdf5::addAttr<float>(fgrp, "frequency_GHz", freq_ghz);
				scatdb::plugins::hdf5::addAttr<float>(fgrp, "wavelength_m", wvlen_m);
				float wvlen_um = wvlen_m * 1000 * 1000;
				const float pi = 3.14159265358979f;

				float tIce = db_ros_f_sorted_stats->floatStats(db::data_entries::SDBR_MEDIAN, db::data_entries::SDBR_TEMPERATURE_K);
				if (tIce <= 0) tIce = 273;
				scatdb::plugins::hdf5::addAttr<float>(fgrp, "temp_ice_k", tIce);
				float tWater = 273.15f;
				scatdb::plugins::hdf5::addAttr<float>(fgrp, "temp_water_k", tWater);

				auto fprof = scatdb::plugins::hdf5::openOrCreateGroup(fgrp, "Profiles");

				int i = 0;
				for (const auto &prof : *(allprofiles.get())) {
					string profid("profile_");
					profid.append(boost::lexical_cast<string>(i));
					auto fpro = scatdb::plugins::hdf5::openOrCreateGroup(fprof, profid.c_str());
					auto fraw = scatdb::plugins::hdf5::openOrCreateGroup(fpro, "Raw");
					prof->writeHDF5File(fraw, profid.c_str());

					auto pts = prof->getParticleTypes();
					string sproftype(scatdb::profiles::defs::stringify(pts));
					ft->setStrings(i, filtnum, profid, sproftype, filter.sName, filter.sCats);

					// Bin the scattering database according to the profile boundaries
					auto data = prof->getData();
					vector < shared_ptr<const db> > binned_raw;
					vector<shared_ptr<const db::data_stats> > binned_stats;
					Eigen::Matrix<float, Eigen::Dynamic, 1> parInts;
					Eigen::Matrix<uint64_t, Eigen::Dynamic, 1> parCounts;
					parInts.resize(data->rows(), 1);
					parInts.setZero();
					parCounts.resize(data->rows(), 1);
					parCounts.setZero();
					auto fpartials = scatdb::plugins::hdf5::openOrCreateGroup(fpro, "Bins");
					const int w = ((int)log10(data->rows())) + 1;
					for (int row = 0; row < data->rows(); ++row) {
						string strbin;
						{
							ostringstream sstrbin;
							sstrbin << setfill('0') << setw(w) << row;
							strbin = sstrbin.str();
						}
						auto obin = scatdb::plugins::hdf5::openOrCreateGroup(fpartials, strbin.c_str());
						auto fbin = filter::generate();
						float binMin = (*data)(row, scatdb::profiles::defs::BIN_LOWER) / 1000.f; // mm
						float binMax = (*data)(row, scatdb::profiles::defs::BIN_UPPER) / 1000.f; // mm
						float binMid = (*data)(row, scatdb::profiles::defs::BIN_MID) / 1000.f; // mm
						float binWidth = binMax - binMin;
						float binConc = (*data)(row, scatdb::profiles::defs::CONCENTRATION); // m^-4
						fbin->addFilterFloat(db::data_entries::SDBR_MAX_DIMENSION_MM, binMin, binMax);
						auto dbin = fbin->apply(db_ros_f_sorted);
						binned_raw.push_back(dbin);
						auto sbin = dbin->getStats();
						binned_stats.push_back(sbin);
						auto odbin = scatdb::plugins::hdf5::openOrCreateGroup(obin, "Filtered");
						auto osbin = scatdb::plugins::hdf5::openOrCreateGroup(obin, "Stats");
						scatdb::plugins::hdf5::addAttr<float>(obin, "bin_min_mm", binMin);
						scatdb::plugins::hdf5::addAttr<float>(obin, "bin_max_mm", binMax);
						scatdb::plugins::hdf5::addAttr<float>(obin, "bin_mid_mm", binMid);
						scatdb::plugins::hdf5::addAttr<float>(obin, "bin_width_mm", binWidth);
						scatdb::plugins::hdf5::addAttr<float>(obin, "bin_conc_m^-4", binConc);
						float medCbk = sbin->floatStats(db::data_entries::SDBR_MEDIAN, db::data_entries::SDBR_CBK_M);
						// Calculate midpoint's size parameter
						float sizep_md = 2.f * pi * binMid * 1000 / wvlen_um;
						scatdb::plugins::hdf5::addAttr<float>(obin, "size_parameter_md", sizep_md);
						if (sbin->count) {
							dbin->writeHDFfile(odbin);
							sbin->writeHDF5File(osbin);
						}
						else {
							scatdb::plugins::hdf5::addAttr<uint64_t>(obin, "Empty", 1);
							// TODO: Add Rayleigh scattering result here if sizep_md is small
							medCbk = 0;
						}
						scatdb::plugins::hdf5::addAttr<float>(obin, "bin_median_cbk_m^2", medCbk);
						uint64_t count = sbin->count;
						// medbck has units of m^2
						// binconc has units of m^-4
						// binwidth has units of mm
						// parInt has units of m^-1
						float parInt = medCbk * binConc * (binWidth / 1000);
						parInts(row, 0) = parInt;
						parCounts(row, 0) = count;
						scatdb::plugins::hdf5::addAttr<float>(obin, "Partial_Integral_Sum_m^-1", parInt);
						scatdb::plugins::hdf5::addAttr<uint64_t>(obin, "Bin_Counts", count);
					}
					// Save the binned results
					scatdb::plugins::hdf5::addDatasetEigen(fpro, "Partial_Sums", parInts);
					scatdb::plugins::hdf5::addDatasetEigen(fpro, "Partial_Counts", parCounts);
					float intSum = parInts.sum();
					scatdb::plugins::hdf5::addAttr<float>(fpro, "Inner_Sum_m^-1", intSum);


					// Get refractive index of ice and water
					std::complex<double> mIce, mWater;
					auto provIce = scatdb::refract::findProvider("ice", true, true);
					auto provWater = scatdb::refract::findProvider("water", true, true);
					if (!provIce) SDBR_throw(scatdb::error::error_types::xBadFunctionReturn)
						.add<std::string>("Reason", "provIce is null");
					if (!provWater) SDBR_throw(scatdb::error::error_types::xBadFunctionReturn)
						.add<std::string>("Reason", "provIce is null");
					scatdb::refract::refractFunction_freq_temp_t r_ice, r_water;
					scatdb::refract::prepRefract(provIce, "GHz", "K", r_ice);
					scatdb::refract::prepRefract(provWater, "GHz", "K", r_water);
					r_ice(freq_ghz, tIce, mIce);
					r_water(freq_ghz, tWater, mWater);
					scatdb::plugins::hdf5::addAttrComplex(fpro, "m_ice", &mIce, 1, 1);
					scatdb::plugins::hdf5::addAttrComplex(fpro, "m_water", &mWater, 1, 1);

					// K_w
					complex<double> Kwater = ((mWater*mWater) + complex<double>(2, 0)) /
						((mWater*mWater) - complex<double>(1, 0));
					scatdb::plugins::hdf5::addAttrComplex(fpro, "K_water", &Kwater, 1, 1);
					// Kw^2
					float kw2 = (float)(Kwater*conj(Kwater)).real();
					scatdb::plugins::hdf5::addAttr<float>(fpro, "Kw^2", kw2);

					// Calculate the radar effective reflectivity
					float Ze = intSum * pow(wvlen_m, 4.f) / (pow(pi, 5.f)*kw2);
					scatdb::plugins::hdf5::addAttr<float>(fpro, "Ze_m^3", Ze);
					float Zemmm = Ze * (float) 1.e18; // Fixed 10/2/16. I never used this field when reporting to scatdb::profiles.
					scatdb::plugins::hdf5::addAttr<float>(fpro, "Ze_mm^6m^-3", Zemmm);
					// Calculate effective radar reflectivity in db
					ft->setZeData(i, filtnum, freqnum, Ze);


					++i;
				}
				freqnum++;
			}
			filtnum++;
		}
		// Take data and write an output summary table.
		// Give the reflectivities for each profile in dBz, m^3 and mm^6m^-3.
		// Also, give the depolarization ratios in dBZe for all of the frequency pairs.
		ft->finalize();
		ft->writeTable(base);
	}
	/// \todo Think of a method to rethrow an error without splicing.
	// Attempt a dynamic cast. Clone the object. Rethrow.
	// If dynamic cast fails, create an xOtherError object with the parameter of e.what(). Rethrow.
	catch (std::exception &e) {
		cerr << "An exception has occurred: " << e.what() << endl;
		return 2;
	}
	/*catch (...) {
	cerr << "An unknown exception has occurred." << endl;
	return 2;
	} */
	return 0;
}
