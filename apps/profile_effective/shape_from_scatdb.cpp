#include "../../scatdb/defs.hpp"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/mean.hpp>
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
#include "../../scatdb/hash.hpp"
#include <hdf5.h>
#include <H5Cpp.h>
#include <H5ArrayType.h>

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
			("flaketypes,y", po::value<vector<string> >(), "Flake type ids for calculation")
			("output,o", po::value<string>(), "Output file path")
			;
		desc.add(cmdline).add(config);
		oall.add(cmdline).add(config).add(hidden);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
			options(oall).run(), vm);
		po::notify(vm);

		debug::process_static_options(vm);

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
		auto allprofiles = profiles::forward_conc_table::readHDF5File(profname.c_str());

		// Get the name of the output file.
		string sout;
		if (vm.count("output")) sout = vm["output"].as<string>();
		else doHelp("Need to specify an output file");

		// Select flake types

		// Filter flake results to a remove duplicates at different frequencies. We do not care about
		// multiple frequencies here.

		// Table for the shape information
		enum SINGLE_COLS_INTS {
			COL_ID,
			COL_NUM_LATTICE,
			NUM_COLS_INTS
		};
		enum SINGLE_COLS_FLOATS {
			COL_MASS_KG,
			COL_MD_M,
			COL_FALLVEL_LH_UNRIMED,
			COL_FALLVEL_LH_RIMED,
			COL_VOLM3,
			COL_REFF_UM,
			NUM_COLS_FLOATS
		};
		Eigen::Array<uint64_t, Eigen::Dynamic, NUM_COLS_INTS> sints_raw;
		Eigen::Array<float, Eigen::Dynamic, NUM_COLS_FLOATS> sfloats_raw;
		Eigen::Array<uint64_t, Eigen::Dynamic, NUM_COLS_INTS> sints;
		Eigen::Array<float, Eigen::Dynamic, NUM_COLS_FLOATS> sfloats;
		
		// Get records and sort by max dimension

		// The float is the max dimension, and the size_t is the row number
		//const pair<float, size_t> proto(0, 0);
		vector<pair<float, size_t> > id_sorter;
		id_sorter.resize(loadedShapes.size());

		size_t snum = 0;
		sints_raw.resize((int)loadedShapesV.size(), NUM_COLS_INTS);
		sfloats_raw.resize((int)loadedShapesV.size(), NUM_COLS_FLOATS);
		sints.resize((int)loadedShapesV.size(), NUM_COLS_INTS);
		sfloats.resize((int)loadedShapesV.size(), NUM_COLS_FLOATS);
		cerr << "Getting projected stats: ";
		
		for (const auto &shp : loadedShapesV) {
			cerr << ".";
			cerr.flush();
			SDBR_log("profile_shape", logging::NOTIFICATION, "Procesing number " << snum);
			SDBR_log("profile_shape", logging::NOTIFICATION, "Procesing shape " << shp->hash()->lower);
			auto bints = sints_raw.block<1, NUM_COLS_INTS>(snum, 0);
			auto bfloats = sfloats_raw.block<1, NUM_COLS_FLOATS>(snum, 0);
			float ds = (float)shp->getPreferredDipoleSpacing();
			if (!ds) ds = 40.f;
			bints(0, COL_ID) = shp->hash()->lower;
			bints(0, COL_NUM_LATTICE) = (uint64_t)shp->numPoints();
			float cmdm = 0, cpa = 0, ccaf = 0, cm = 0, cvol = 0, vmu = 0, vms = 0, reffm = 0;

			shape::algorithms::getProjectedStats(shp, ds, "um", cmdm, cpa, ccaf, cm, cvol, reffm);



			// Determine fall speed velocity (height assumed at 4 km, us standard 1976 atmosphere)
			double eta, rho_air, g, P_air;
			// Look up eta, rho_air, g from table.
			shape::algorithms::getEnvironmentConds(4000., 0, eta, P_air, rho_air, g);
			vmu = shape::algorithms::getV_HW10_m_s(rho_air, cm, g, eta, ccaf, cmdm);
			shape::algorithms::getEnvironmentConds(0., 0, eta, P_air, rho_air, g);
			vms = shape::algorithms::getV_HW10_m_s(rho_air, cm, g, eta, ccaf, cmdm);
			
			bfloats(0, COL_MD_M) = cmdm;
			bfloats(0, COL_PROJAREA) = cpa;
			bfloats(0, COL_CIRCUMAREAFRAC) = ccaf;
			bfloats(0, COL_MASS_KG) = cm;
			bfloats(0, COL_FALLVEL_HW_UPPER) = vmu;
			bfloats(0, COL_FALLVEL_HW_SFC) = vms;
			bfloats(0, COL_FALLVEL_LH_UNRIMED) = 0.82f * std::pow(cmdm*1000.f, 0.12f);
			bfloats(0, COL_FALLVEL_LH_RIMED) = 0.79f * std::pow(cmdm*1000.f, 0.27f);
			bfloats(0, COL_VOLM3) = cvol;
			bfloats(0, COL_REFF_UM) = reffm * 1000000.f;
			id_sorter[snum].first = bfloats(0, COL_MD_M);
			id_sorter[snum].second = snum;
			++snum;
		}
		
		// Sort according to max dimension, which will make the binning process much faster.
		cerr << "\nSorting all shapes according to maximum dimension." << endl;
		std::sort(id_sorter.begin(), id_sorter.end(), [&](pair<float, size_t> i, pair<float, size_t> j)
			{return i.first < j.first; });
		for (int i = 0; i < sfloats.rows(); ++i) {
			size_t raw_row = id_sorter[i].second;
			sints.block<1, NUM_COLS_INTS>(i, 0) = sints_raw.block<1, NUM_COLS_INTS>((int)raw_row, 0);
			sfloats.block<1, NUM_COLS_FLOATS>(i, 0) = sfloats_raw.block<1, NUM_COLS_FLOATS>((int)raw_row, 0);
		}
		
		// Write the raw tables
		cerr << "Writing the raw sorted shape tables for debugging purposes." << endl;
		using namespace H5;
		//Exception::dontPrint();
		std::shared_ptr<H5::H5File> file;
		boost::filesystem::path p(sout);
		file = std::shared_ptr<H5File>(new H5File(sout, H5F_ACC_TRUNC));
		auto base = scatdb::plugins::hdf5::openOrCreateGroup(file, "shape-derived");
		auto fsbase = scatdb::plugins::hdf5::openOrCreateGroup(base, "per-shape");
		//cout << sints << endl;
		auto rawi = plugins::hdf5::addDatasetEigen(fsbase, "ints", sints);
		auto rawf = plugins::hdf5::addDatasetEigen(fsbase, "floats", sfloats);
		// These, for some reason, cause a problem.
		plugins::hdf5::addAttr<std::string>(rawi, "col_00", std::string("COL_ID"));
		plugins::hdf5::addAttr<std::string>(rawi, "col_01", std::string("COL_NUM_LATTICE"));
		plugins::hdf5::addAttr<std::string>(rawf, "col_00", std::string("COL_MASS_KG"));
		plugins::hdf5::addAttr<std::string>(rawf, "col_01", std::string("COL_MD_M"));
		plugins::hdf5::addAttr<std::string>(rawf, "col_02", std::string("COL_FALLVEL_LH_UNRIMED_M_S"));
		plugins::hdf5::addAttr<std::string>(rawf, "col_03", std::string("COL_FALLVEL_LH_RIMED_M_S"));
		plugins::hdf5::addAttr<std::string>(rawf, "col_04", std::string("COL_VOLUME_M3"));
		plugins::hdf5::addAttr<std::string>(rawf, "col_05", std::string("COL_REFF_UM"));
		//return 99;
		auto fprof = scatdb::plugins::hdf5::openOrCreateGroup(file, "per-profile");

		// Write all of the scatteing database

		cerr << "Iterating over the profiles, and writing to the summary table." << endl;
		// Profile summary table
		Eigen::Array<float, Eigen::Dynamic, 10> profloats;
		profloats.resize((int)allprofiles->size(), 10);
		profloats.setZero();
		// Iterate over the profiles
		int pnum = 0;
		const float pi = 3.141592654f;
		const float rho_g_m3 = (float) 9.16e5; // 916 kg/m^3 = 9.16e5 g/m3
		const float rho_wat_g_m3 = (float) 1e6; // 1000 kg/m3 = 1e6 g/m3
		for (const auto &prof : *(allprofiles.get()))
		{
			++pnum;
			string profid("profile_");
			profid.append(boost::lexical_cast<string>(pnum));
			cerr << "\tProfile " << profid << endl << "\t\tWriting..." << endl;
			auto fpro = scatdb::plugins::hdf5::openOrCreateGroup(fprof, profid.c_str());
			auto fraw = scatdb::plugins::hdf5::openOrCreateGroup(fpro, "Raw");
			prof->writeHDF5File(fraw, profid.c_str());

			// Iterate over the flake types and create subfolder structure.

			// Also, write the subsetted database entries.

			auto pts = prof->getParticleTypes();
			string sproftype(scatdb::profiles::defs::stringify(pts));
			
			// Bin the scattering database according to the profile boundaries
			// Each cell contains the median value within each bin for the designated quantity.
			enum BINNED_COLS_FLOATS {
				COL_B_BIN_LOW_MM,
				COL_B_BIN_MID_MM,
				COL_B_BIN_WIDTH_MM,
				COL_B_BIN_HIGH_MM,
				COL_B_CONC_M_4,
				COL_B_NUM_IN_BIN,

				COL_B_MASS_KG,
				COL_B_MD_M,
				COL_B_FALLVEL_LH_UNRIMED,
				COL_B_FALLVEL_LH_RIMED,
				COL_B_VOL_M_3,

				COL_B_PARTIAL_IWC_G_M3,
				COL_B_PARTIAL_S_LH_UNRIMED_MM_H,
				COL_B_PARTIAL_S_LH_RIMED_MM_H,
				COL_B_SHAPE_LOW, // for sorting
				COL_B_SHAPE_HIGH, // for sorting
				NUM_B_COLS_FLOATS
			};
			Eigen::Array<float, Eigen::Dynamic, NUM_B_COLS_FLOATS> binned_floats;
			auto data = prof->getData();
			binned_floats.resize(data->rows(), NUM_B_COLS_FLOATS);
			binned_floats.setZero();
			cerr << "\t\tBinning: ";
			int lastSortedShapeRow = 0;
			for (int row = 0; row < data->rows(); ++row) {
				cerr << "."; cerr.flush();
				auto brow = binned_floats.block<1, NUM_B_COLS_FLOATS>(row, 0);
				brow(0, COL_B_BIN_LOW_MM) = (*data)(row, scatdb::profiles::defs::BIN_LOWER) / 1000.f; // mm
				brow(0, COL_B_BIN_HIGH_MM) = (*data)(row, scatdb::profiles::defs::BIN_UPPER) / 1000.f; // mm
				brow(0, COL_B_BIN_MID_MM) = (*data)(row, scatdb::profiles::defs::BIN_MID) / 1000.f; // mm
				brow(0, COL_B_BIN_WIDTH_MM) = brow(COL_B_BIN_HIGH_MM) - brow(COL_B_BIN_LOW_MM);
				brow(0, COL_B_CONC_M_4) = (*data)(row, scatdb::profiles::defs::CONCENTRATION); // m^-4
				
				using namespace boost::accumulators;
				// NOTE: Using doubles instead of floats to avoid annoying boost internal library warnings
				typedef accumulator_set<double, boost::accumulators::stats <
					tag::mean
				> > acc_type;
				std::array<acc_type, 5> accs;
				int count = 0;
				// Push the data to the accumulator functions.
				brow(0, COL_B_SHAPE_LOW) = (float) lastSortedShapeRow;
				for (int i = lastSortedShapeRow; i<sfloats.rows(); ++i) {
					const auto &shpfloats = sfloats.block<1, NUM_COLS_FLOATS>(i, 0);
					if (shpfloats(COL_MD_M) * 1000.f < brow(0, COL_B_BIN_LOW_MM)) continue;
					if (shpfloats(COL_MD_M) * 1000.f > brow(0, COL_B_BIN_HIGH_MM)) break;
					lastSortedShapeRow = i;

					accs[0]((double)shpfloats(COL_MASS_KG));
					accs[1]((double)shpfloats(COL_MD_M));
					accs[2]((double)shpfloats(COL_FALLVEL_LH_UNRIMED));
					accs[3]((double)shpfloats(COL_FALLVEL_LH_RIMED));
					accs[4]((double)shpfloats(COL_VOLM3));
					++count;
				}
				brow(0, COL_B_SHAPE_HIGH) = (float) lastSortedShapeRow;
				if (count) {
					brow(0, COL_B_NUM_IN_BIN) = (float)count;
					brow(0, COL_B_MASS_KG) = (float)boost::accumulators::mean(accs[0]);
					brow(0, COL_B_MD_M) = (float)boost::accumulators::mean(accs[1]);
					brow(0, COL_B_FALLVEL_LH_UNRIMED) = (float)boost::accumulators::mean(accs[2]);
					brow(0, COL_B_FALLVEL_LH_RIMED) = (float)boost::accumulators::mean(accs[3]);
					brow(0, COL_B_VOL_M_3) = (float)boost::accumulators::mean(accs[4]);
					brow(0, COL_B_PARTIAL_IWC_G_M3) = 
						brow(0, COL_B_CONC_M_4) // m^-4
						* brow(0, COL_B_BIN_WIDTH_MM) /1000.f // m
						* brow(0, COL_B_MASS_KG) * 1000.f // g
						;
					brow(0, COL_B_PARTIAL_S_LH_UNRIMED_MM_H) =
						brow(0, COL_B_MASS_KG) * 1000.f
						/ rho_wat_g_m3
						* brow(0, COL_B_CONC_M_4) // m^-4
						* brow(0, COL_B_BIN_WIDTH_MM) / 1000.f // m
						* brow(0, COL_B_FALLVEL_LH_UNRIMED) // m/s
						* 3600.f * 1000.f; // want mm/h
					brow(0, COL_B_PARTIAL_S_LH_RIMED_MM_H) =
						brow(0, COL_B_MASS_KG) * 1000.f
						/ rho_wat_g_m3
						* brow(0, COL_B_CONC_M_4) // m^-4
						* brow(0, COL_B_BIN_WIDTH_MM) / 1000.f // m
						* brow(0, COL_B_FALLVEL_LH_RIMED) // m/s
						* 3600.f * 1000.f; // want mm/h

				}
			}
			cerr << endl << "\t\tCalculating average quantities." << endl;
			auto dbf = plugins::hdf5::addDatasetEigen(fpro, "binned_floats", binned_floats);
			plugins::hdf5::addAttr<std::string>(dbf, "col_00", std::string("COL_B_BIN_LOW_MM"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_01", std::string("COL_B_BIN_MID_MM"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_02", std::string("COL_B_BIN_WIDTH_MM"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_03", std::string("COL_B_BIN_HIGH_MM"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_04", std::string("COL_B_CONC_M_4"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_05", std::string("COL_B_NUM_IN_BIN"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_06", std::string("COL_B_MASS_KG"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_07", std::string("COL_B_MD_M"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_08", std::string("COL_B_FALLVEL_LH_UNRIMED"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_09", std::string("COL_B_FALLVEL_LH_RIMED"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_10", std::string("COL_B_VOL_M_3"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_11", std::string("COL_B_PARTIAL_IWC_G_M3"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_12", std::string("COL_B_PARTIAL_S_LH_UNRIMED_MM_H"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_13", std::string("COL_B_PARTIAL_S_LH_RIMED_MM_H"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_14", std::string("COL_B_SHAPE_LOW"));
			plugins::hdf5::addAttr<std::string>(dbf, "col_15", std::string("COL_B_SHAPE_HIGH"));

			// Calculate the bulk and mass-weighted quantities. Write these also.
			
			// Mass-weighted fall velocity [m/s], #-weighted fall velocity
			// Snowfall rate [mm/h]
			// Ice water content [g/m^3]
			float v_LH_UNRIMED_mass_weighted_m_s = 0,
				v_LH_RIMED_mass_weighted_m_s = 0,
				S_LH_UNRIMED_mm_h = 0,
				S_LH_RIMED_mm_h = 0,
				IWC_g_m3 = 0;
			float Ndd_m_3 = 0, NddV_m_3_m_s = 0, NddM_m_3_kg = 0, 
				NddMVLHU_m_3_kg_m_s = 0, NddMVLHR_m_3_kg_m_s = 0;

			for (int bin = 0; bin < binned_floats.rows(); ++bin) {
				Ndd_m_3 += binned_floats(bin, COL_B_CONC_M_4) * binned_floats(bin, COL_B_BIN_WIDTH_MM) / 1000.f;
				NddV_m_3_m_s += binned_floats(bin, COL_B_CONC_M_4) * binned_floats(bin, COL_B_BIN_WIDTH_MM) / 1000.f * binned_floats(bin, COL_B_FALLVEL_HW_UPPER);
				NddM_m_3_kg += binned_floats(bin, COL_B_CONC_M_4) * binned_floats(bin, COL_B_BIN_WIDTH_MM) / 1000.f * binned_floats(bin, COL_B_MASS_KG);
				NddMVLHU_m_3_kg_m_s += binned_floats(bin, COL_B_CONC_M_4) * binned_floats(bin, COL_B_BIN_WIDTH_MM) / 1000.f
					* binned_floats(bin, COL_B_FALLVEL_LH_UNRIMED) * binned_floats(bin, COL_B_MASS_KG);
				NddMVLHR_m_3_kg_m_s += binned_floats(bin, COL_B_CONC_M_4) * binned_floats(bin, COL_B_BIN_WIDTH_MM) / 1000.f
					* binned_floats(bin, COL_B_FALLVEL_LH_RIMED) * binned_floats(bin, COL_B_MASS_KG);

				S_LH_UNRIMED_mm_h += binned_floats(bin, COL_B_PARTIAL_S_LH_UNRIMED_MM_H);
				S_LH_RIMED_mm_h += binned_floats(bin, COL_B_PARTIAL_S_LH_RIMED_MM_H);

				IWC_g_m3_check += (pi / 6.f) * rho_g_m3 // g/m^3
					* std::pow(binned_floats(bin, COL_B_MD_M), 3.f) // m^3
					* binned_floats(bin, COL_B_CONC_M_4) // m^-4
					* binned_floats(bin, COL_B_BIN_WIDTH_MM)/1000.f // m
					;
				IWC_g_m3 += 
					binned_floats(bin, COL_B_CONC_M_4) // m^-4
					* binned_floats(bin, COL_B_BIN_WIDTH_MM) /1000.f // m
					* binned_floats(bin, COL_B_MASS_KG) * 1000.f // g
					;
			}
			//v_num_weighted_m_s = NddV_m_3_m_s / Ndd_m_3;
			v_LH_UNRIMED_mass_weighted_m_s = NddMVLHU_m_3_kg_m_s / NddM_m_3_kg;
			v_LH_RIMED_mass_weighted_m_s = NddMVLHR_m_3_kg_m_s / NddM_m_3_kg;

			if (IWC_g_m3 != IWC_g_m3) IWC_g_m3 = -1;


			plugins::hdf5::addAttr<float>(fpro, "v_LH_UNRIMED_mass_weighted_m_s", v_LH_UNRIMED_mass_weighted_m_s);
			plugins::hdf5::addAttr<float>(fpro, "v_LH_RIMED_mass_weighted_m_s", v_LH_RIMED_mass_weighted_m_s);
			plugins::hdf5::addAttr<float>(fpro, "S_LH_UNRIMED_mm_h", S_LH_UNRIMED_mm_h);
			plugins::hdf5::addAttr<float>(fpro, "S_LH_RIMED_mm_h", S_LH_RIMED_mm_h);
			plugins::hdf5::addAttr<float>(fpro, "IWC_g_m3", IWC_g_m3);
			// And add to the summary table
			// TODO: add row-finding function
			profloats(pnum - 1, 0) = S_LH_UNRIMED_mm_h;
			profloats(pnum - 1, 1) = S_LH_RIMED_mm_h;
			profloats(pnum - 1, 2) = IWC_g_m3;
			profloats(pnum - 1, 3) = v_LH_UNRIMED_mass_weighted_m_s;
			profloats(pnum - 1, 4) = v_LH_RIMED_mass_weighted_m_s;
			//profloats(pnum - 1, 4) = v_num_weighted_m_s;
		}
		cerr << "All profiles done. Writing summary table and exiting." << endl;
		auto dbps = plugins::hdf5::addDatasetEigen(file, "profile_summary", profloats);
		plugins::hdf5::addAttr<std::string>(dbps, "col_00", std::string("S_HW_UPPER_mm_h"));
		plugins::hdf5::addAttr<std::string>(dbps, "col_01", std::string("S_HW_SFC_mm_h"));
		plugins::hdf5::addAttr<std::string>(dbps, "col_02", std::string("S_LH_UNRIMED_mm_h"));
		plugins::hdf5::addAttr<std::string>(dbps, "col_03", std::string("S_LH_RIMED_mm_h"));
		plugins::hdf5::addAttr<std::string>(dbps, "col_04", std::string("IWC_g_m3"));
		plugins::hdf5::addAttr<std::string>(dbps, "col_05", std::string("IWC_g_m3_check"));
		plugins::hdf5::addAttr<std::string>(dbps, "col_06", std::string("v_HW_UPPER_mass_weighted_m_s"));
		plugins::hdf5::addAttr<std::string>(dbps, "col_07", std::string("v_HW_SFC_mass_weighted_m_s"));
		plugins::hdf5::addAttr<std::string>(dbps, "col_08", std::string("v_LH_UNRIMED_mass_weighted_m_s"));
		plugins::hdf5::addAttr<std::string>(dbps, "col_09", std::string("v_LH_RIMED_mass_weighted_m_s"));
		//plugins::hdf5::addAttr<std::string>(dbps, "col_4", std::string("v_num_weighted_m_s"));

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
