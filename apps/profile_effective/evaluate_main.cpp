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
#include "../../scatdb/refract.hpp"
#include "../../scatdb/scatdb.hpp"
#include "../../scatdb/export-hdf5.h"
#include <hdf5.h>
#include <H5Cpp.h>

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
			("scatdb", po::value<string>(), "Location of scattering database")
			("output", po::value<string>(), "Output file path")
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
		auto allprofiles = Andy::forward_conc_table::readHDF5File(profname.c_str());

		// Read the scattering database
		using namespace scatdb_ryan;
		string sdbname;
		if (vm.count("scatdb")) sdbname = vm["scatdb"].as<string>();
		else db::findDB(sdbname);
		auto sdb = db::loadDB(sdbname.c_str());
		auto f = filter::generate();
		// Only use rosettes and rosette aggregates
		f->addFilterInt(db::data_entries::FLAKETYPE, "5/8,20");
		auto db_ros = f->apply(sdb);

		vector<pair<string, string> > freqranges;
		freqranges.push_back(pair<string, string>("Ku", "13/14"));
		freqranges.push_back(pair<string, string>("Ka", "35/36"));

		string sout;
		if (vm.count("output")) sout = vm["output"].as<string>();
		else doHelp("Need to specify an output file");
		scatdb::plugins::hdf5::useZLIB(true);
		using namespace H5;
		//Exception::dontPrint();
		std::shared_ptr<H5::H5File> file;
		boost::filesystem::path p(sout);
		if (boost::filesystem::exists(p))
			file = std::shared_ptr<H5File>(new H5File(sout, H5F_ACC_TRUNC));
		else
			file = std::shared_ptr<H5File>(new H5File(sout, H5F_ACC_TRUNC));
		auto base = scatdb::plugins::hdf5::openOrCreateGroup(file, "andy_output");
		auto fsbase = scatdb::plugins::hdf5::openOrCreateGroup(base, "scatdb_initial");
		sdb->writeHDF5File(fsbase);
		auto fsros = scatdb::plugins::hdf5::openOrCreateGroup(base, "scatdb_rosettes");
		db_ros->writeHDF5File(fsros);

		// Iterate over each profile and each set of frequencies
		for (const auto &freq : freqranges) {
			auto ff = filter::generate();
			ff->addFilterFloat(db::data_entries::FREQUENCY_GHZ, freq.second);
			ff->addFilterFloat(db::data_entries::TEMPERATURE_K, "262/264");
			auto db_ros_f = ff->apply(db_ros);
			auto db_ros_f_sorted = db_ros_f; // db_ros_f->sort(db::data_entries::MAX_DIMENSION_MM);

			auto fgrp = scatdb::plugins::hdf5::openOrCreateGroup(base, freq.first.c_str());
			auto fshpun = scatdb::plugins::hdf5::openOrCreateGroup(fgrp, "freq_temp_scatdb_unsorted");
			db_ros_f->writeHDF5File(fshpun);
			//auto fshp = scatdb::plugins::hdf5::openOrCreateGroup(fgrp, "freq_temp_scatdb_sorted");
			//db_ros_f_sorted->writeHDF5File(fshp);

			auto db_ros_f_sorted_stats = db_ros_f_sorted->getStats();
			auto o_db_ros_f_sorted_stats = scatdb::plugins::hdf5::openOrCreateGroup(fgrp, "Stats");
			db_ros_f_sorted_stats->writeHDF5File(o_db_ros_f_sorted_stats);

			// Get frequency
			float freq_ghz = db_ros_f_sorted_stats->floatStats(db::data_entries::MEDIAN, db::data_entries::FREQUENCY_GHZ);
			// Get wavelength
			auto specConv_m = scatdb::units::conv_spec::generate("GHz", "m");
			float wvlen_m = (float)specConv_m->convert(freq_ghz);
			scatdb::plugins::hdf5::addAttr<float>(fgrp, "frequency_GHz", freq_ghz);
			scatdb::plugins::hdf5::addAttr<float>(fgrp, "wavelength_m", wvlen_m);
			float wvlen_um = wvlen_m * 1000 * 1000;
			const float pi = 3.14159265358979f;

			float tIce = db_ros_f_sorted_stats->floatStats(db::data_entries::MEDIAN, db::data_entries::TEMPERATURE_K);
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
				
				

				// Bin the scattering database according to the profile boundaries
				auto data = prof->getData();
				vector < shared_ptr<const db> > binned_raw;
				vector<shared_ptr<const db::data_stats> > binned_stats;
				Eigen::Matrix<float, Eigen::Dynamic, 1> parInts, parCbk;
				Eigen::Matrix<int, Eigen::Dynamic, 1> parCounts;
				parInts.resize(data->rows(), 1);
				parInts.setZero();
				parCbk.resize(data->rows(), 1);
				parCbk.setZero();
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
					float binMin = (*data)(row, Andy::defs::BIN_LOWER) / 1000.f; // mm
					float binMax = (*data)(row, Andy::defs::BIN_UPPER) / 1000.f; // mm
					float binMid = (*data)(row, Andy::defs::BIN_MID) / 1000.f; // mm
					float binWidth = binMax - binMin;
					float binConc = (*data)(row, Andy::defs::CONCENTRATION); // m^-4
					fbin->addFilterFloat(db::data_entries::MAX_DIMENSION_MM, binMin, binMax);
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
					float medCbk = sbin->floatStats(db::data_entries::MEDIAN, db::data_entries::CBK_M);
					// Calculate midpoint's size parameter
					float sizep_md = 2.f * pi * binMid * 1000 / wvlen_um;
					scatdb::plugins::hdf5::addAttr<float>(obin, "size_parameter_md", sizep_md);
					if (sbin->count) {
						dbin->writeHDF5File(odbin);
						sbin->writeHDF5File(osbin);
					} else {
						scatdb::plugins::hdf5::addAttr<int>(obin, "Empty", 1);
						// TODO: Add Rayleigh scattering result here if sizep_md is small
						medCbk = 0;
					}
					scatdb::plugins::hdf5::addAttr<float>(obin, "bin_median_cbk_m^2", medCbk);
					int count = sbin->count;
					// medbck has units of m^2
					// binconc has units of m^-4
					// binwidth has units of mm
					// parInt has units of m^-1
					float parInt = medCbk * binConc * (binWidth / 1000);
					parInts(row, 0) = parInt;
					parCounts(row, 0) = count;
					parCbk(row, 0) = medCbk;
					scatdb::plugins::hdf5::addAttr<float>(obin, "Partial_Integral_Sum_m^-1", parInt);
					scatdb::plugins::hdf5::addAttr<int>(obin, "Bin_Counts", count);
				}
				// Save the binned results
				scatdb::plugins::hdf5::addDatasetEigen(fpro, "Partial_Sums", parInts);
				scatdb::plugins::hdf5::addDatasetEigen(fpro, "Partial_Counts", parCounts);
				scatdb::plugins::hdf5::addDatasetEigen(fpro, "Bin_Median_Cbk_m^2", parCbk);

				float intSum = parInts.sum();
				scatdb::plugins::hdf5::addAttr<float>(fpro, "Inner_Sum_m^-1", intSum);

				
				// Get refractive index of ice and water
				std::complex<double> mIce, mWater;
				auto provIce = scatdb::refract::findProvider("ice", true, true);
				auto provWater = scatdb::refract::findProvider("water", true, true);
				if (!provIce) RSthrow(scatdb::error::error_types::xBadFunctionReturn)
					.add<std::string>("Reason", "provIce is null");
				if (!provWater) RSthrow(scatdb::error::error_types::xBadFunctionReturn)
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
				float kw2 = (float) (Kwater*conj(Kwater)).real();
				scatdb::plugins::hdf5::addAttr<float>(fpro, "Kw^2", kw2);

				// Calculate the radar effective reflectivity
				float Ze = intSum * pow(wvlen_m, 4.f) / (pow(pi,5.f)*kw2);
				scatdb::plugins::hdf5::addAttr<float>(fpro, "Ze_m^3", Ze);
				

				++i;
			}
		}

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