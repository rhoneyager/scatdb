#include "../../scatdb/defs.hpp"
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/median.hpp>
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
#include "../../scatdb/shape/shape.hpp"
#include "../../scatdb/shape/shapeIO.hpp"
#include "../../scatdb/shape/shapeAlgs.hpp"
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
			//("profiles-path", po::value<vector<string> >(),
			//	"Path to the folder containing the profiles inside the HDF5 file. "
			//	"Can be specified multiple times.")
			("shapes,s", po::value<vector<string> >()->multitoken(), "Shape files")
			//("dipole-spacings,d", po::value<double>()->default_value(40), "Shape dipole spacings (um)")
			("output", po::value<string>(), "Output file path")
			//("verbosity,v", po::value<int>()->default_value(1), "Change level of detail in "
			//	"the output file. Higher number means more intermediate results will be writen.")
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

		// Read the shapes
		if (!vm.count("shapes")) doHelp("Must specify shapes.");
		vector<string> vsshapes = vm["shapes"].as<vector<string> >();

		// Table for the shape information
		enum SINGLE_COLS_INTS {
			COL_ID,
			COL_NUM_LATTICE,
			NUM_COLS_INTS
		};
		enum SINGLE_COLS_FLOATS {
			COL_MASS_KG,
			COL_MD_M,
			COL_PROJAREA,
			COL_CIRCUMAREAFRAC,
			COL_FALLVEL,
			NUM_COLS_FLOATS
		};
		Eigen::Array<uint64_t, Eigen::Dynamic, NUM_COLS_INTS> sints;
		Eigen::Array<float, Eigen::Dynamic, NUM_COLS_FLOATS> sfloats;
		sints.resize((int)vsshapes.size(), NUM_COLS_INTS);
		sfloats.resize((int)vsshapes.size(), NUM_COLS_FLOATS);

		// Read the shape files
		shape::shapeIO sio;
		sio.shapes.reserve(vsshapes.size());
		size_t snum = 0;
		for (const auto &s : vsshapes) {
			sio.readFile(s);
			auto bints = sints.block<1, NUM_COLS_INTS>(snum, 0);
			auto bfloats = sfloats.block<1, NUM_COLS_FLOATS>(snum, 0);
			auto shp = sio.shapes[snum];
			float ds = (float) shp->getPreferredDipoleSpacing();
			if (!ds) ds = 40.f;
			bints(0, COL_ID) = shp->hash()->lower;
			bints(0, COL_NUM_LATTICE) = (uint64_t) shp->numPoints();
			shape::algorithms::getProjectedStats(shp, ds, "um", sfloats(0, COL_MD_M), sfloats(0, COL_PROJAREA),
				sfloats(0, COL_CIRCUMAREAFRAC), sfloats(0, COL_MASS_KG), sfloats(0, COL_FALLVEL));
			++snum;
		}                                       
		// Write the raw tables
		string sout;
		if (vm.count("output")) sout = vm["output"].as<string>();
		else doHelp("Need to specify an output file");
		using namespace H5;
		//Exception::dontPrint();
		std::shared_ptr<H5::H5File> file;
		boost::filesystem::path p(sout);
		file = std::shared_ptr<H5File>(new H5File(sout, H5F_ACC_TRUNC));
		auto base = scatdb::plugins::hdf5::openOrCreateGroup(file, "output-shape-falls");
		auto fsbase = scatdb::plugins::hdf5::openOrCreateGroup(base, "per-shape");
		auto rawi = plugins::hdf5::addDatasetEigen(fsbase, "ints", sints);
		auto rawf = plugins::hdf5::addDatasetEigen(fsbase, "floats", sfloats);
		plugins::hdf5::addAttr<std::string>(rawi, "col_0", "COL_ID");
		plugins::hdf5::addAttr<std::string>(rawi, "col_1", "COL_NUM_LATTICE");
		plugins::hdf5::addAttr<std::string>(rawf, "col_0", "COL_MASS_KG");
		plugins::hdf5::addAttr<std::string>(rawf, "col_1", "COL_MD_M");
		plugins::hdf5::addAttr<std::string>(rawf, "col_2", "COL_PROJAREA");
		plugins::hdf5::addAttr<std::string>(rawf, "col_3", "COL_CIRCUMAREAFRAC");
		plugins::hdf5::addAttr<std::string>(rawf, "col_4", "COL_FALLVEL");

		auto fprof = scatdb::plugins::hdf5::openOrCreateGroup(file, "per-profile");


		// Profile summary table
		Eigen::Array<float, Eigen::Dynamic, 4> profloats;
		profloats.resize((int)allprofiles->size(), 4);
		profloats.setZero();
		// Iterate over the profiles
		int pnum = 0;
		for (const auto &prof : *(allprofiles.get()))
		{
			++pnum;
			string profid("profile_");
			profid.append(boost::lexical_cast<string>(pnum));
			auto fpro = scatdb::plugins::hdf5::openOrCreateGroup(fprof, profid.c_str());
			auto fraw = scatdb::plugins::hdf5::openOrCreateGroup(fpro, "Raw");
			prof->writeHDF5File(fraw, profid.c_str());

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
				COL_B_PROJAREA,
				COL_B_CIRCUMAREAFRAC,
				COL_B_FALLVEL,
				NUM_B_COLS_FLOATS
			};
			Eigen::Array<float, Eigen::Dynamic, NUM_B_COLS_FLOATS> binned_floats;
			auto data = prof->getData();
			binned_floats.resize(data->rows(), NUM_B_COLS_FLOATS);
			
			for (int row = 0; row < data->rows(); ++row) {
				auto brow = binned_floats.block<1, NUM_B_COLS_FLOATS>(row, 0);
				brow(0, COL_B_BIN_LOW_MM) = (*data)(row, scatdb::profiles::defs::BIN_LOWER) / 1000.f; // mm
				brow(0, COL_B_BIN_HIGH_MM) = (*data)(row, scatdb::profiles::defs::BIN_UPPER) / 1000.f; // mm
				brow(0, COL_B_BIN_MID_MM) = (*data)(row, scatdb::profiles::defs::BIN_MID) / 1000.f; // mm
				brow(0, COL_B_BIN_WIDTH_MM) = brow(COL_B_BIN_HIGH_MM) - brow(COL_B_BIN_LOW_MM);
				brow(0, COL_B_CONC_M_4) = (*data)(row, scatdb::profiles::defs::CONCENTRATION); // m^-4
				
				
				// Take the shapes and filter them according to max dimension.
				//Eigen::Array<float, Eigen::Dynamic, NUM_COLS_FLOATS> sfloats;
				//enum SINGLE_COLS_FLOATS {
				//	COL_MASS_KG,
				//	COL_MD_M,
				//	COL_PROJAREA,
				//	COL_CIRCUMAREAFRAC,
				//	COL_FALLVEL,
				//	NUM_COLS_FLOATS
				//};
				using namespace boost::accumulators;
				// NOTE: Using doubles instead of floats to avoid annoying boost internal library warnings
				typedef accumulator_set<double, boost::accumulators::stats <
					tag::median
				> > acc_type;
				std::array<acc_type, 5> accs;
				int count = 0;
				// Push the data to the accumulator functions.
				for (int i = 0; i<sfloats.rows(); ++i) {
					const auto &shpfloats = sfloats.block<1, NUM_COLS_FLOATS>(i, 0);
					if ((shpfloats(COL_MD_M) * 1000. >= brow(0, COL_B_BIN_LOW_MM) )
						&& (shpfloats(COL_MD_M) * 1000. < brow(0, COL_B_BIN_HIGH_MM))) {
						accs[0]((double)shpfloats(COL_MASS_KG));
						accs[1]((double)shpfloats(COL_MD_M));
						accs[2]((double)shpfloats(COL_PROJAREA));
						accs[3]((double)shpfloats(COL_CIRCUMAREAFRAC));
						accs[4]((double)shpfloats(COL_FALLVEL));
						++count;
					}
				}
				brow(0, COL_B_NUM_IN_BIN) = (float)count;
				brow(0, COL_B_MASS_KG) = (float)boost::accumulators::median(accs[0]);
				brow(0, COL_B_MD_M) = (float)boost::accumulators::median(accs[1]);
				brow(0, COL_B_PROJAREA) = (float)boost::accumulators::median(accs[2]);
				brow(0, COL_B_CIRCUMAREAFRAC) = (float)boost::accumulators::median(accs[3]);
				brow(0, COL_B_FALLVEL) = (float)boost::accumulators::median(accs[4]);
			}

			auto dbf = plugins::hdf5::addDatasetEigen(fpro, "binned_floats", binned_floats);
			plugins::hdf5::addAttr<std::string>(dbf, "col_0", "COL_B_BIN_LOW_MM");
			plugins::hdf5::addAttr<std::string>(dbf, "col_1", "COL_B_BIN_MID_MM");
			plugins::hdf5::addAttr<std::string>(dbf, "col_2", "COL_B_BIN_WIDTH_MM");
			plugins::hdf5::addAttr<std::string>(dbf, "col_3", "COL_B_BIN_HIGH_MM");
			plugins::hdf5::addAttr<std::string>(dbf, "col_4", "COL_B_CONC_M_4");
			plugins::hdf5::addAttr<std::string>(dbf, "col_5", "COL_B_NUM_IN_BIN");
			plugins::hdf5::addAttr<std::string>(dbf, "col_6", "COL_B_MASS_KG");
			plugins::hdf5::addAttr<std::string>(dbf, "col_7", "COL_B_MD_M");
			plugins::hdf5::addAttr<std::string>(dbf, "col_8", "COL_B_PROJAREA");
			plugins::hdf5::addAttr<std::string>(dbf, "col_9", "COL_B_CIRCUMAREAFRAC");
			plugins::hdf5::addAttr<std::string>(dbf, "col_10", "COL_B_FALLVEL");


			// Calculate the bulk and mass-weighted quantities. Write these also.
			const float pi = 3.141592654f;
			// Mass-weighted fall velocity [m/s], #-weighted fall velocity
			// Snowfall rate [mm/h]
			// Ice water content [g/m^3]
			float v_mass_weighted_m_s = 0, v_num_weighted_m_s = 0, S_mm_h = 0, IWC_g_m3 = 0;
			float Ndd_m_3 = 0, NddV_m_3_m_s = 0, NddM_m_3_kg = 0, NddMV_m_3_kg_m_s = 0;
			float rho_kg_m3 = 916;
			for (int bin = 0; bin < binned_floats.rows(); ++bin) {
				Ndd_m_3 += binned_floats(bin, COL_B_CONC_M_4) * binned_floats(bin, COL_B_BIN_WIDTH_MM) / 1000.f;
				NddV_m_3_m_s += binned_floats(bin, COL_B_CONC_M_4) * binned_floats(bin, COL_B_BIN_WIDTH_MM) / 1000.f * binned_floats(bin, COL_B_FALLVEL);
				NddM_m_3_kg += binned_floats(bin, COL_B_CONC_M_4) * binned_floats(bin, COL_B_BIN_WIDTH_MM) / 1000.f * binned_floats(bin, COL_B_MASS_KG);
				NddMV_m_3_kg_m_s += binned_floats(bin, COL_B_CONC_M_4) * binned_floats(bin, COL_B_BIN_WIDTH_MM) / 1000.f * binned_floats(bin, COL_B_FALLVEL) * binned_floats(bin, COL_B_MASS_KG);

				S_mm_h = 0.6f * pi * binned_floats(bin, COL_B_FALLVEL) * binned_floats(bin, COL_B_CONC_M_4) // (m/s) * (m^-4)
					* std::pow(binned_floats(bin, COL_B_BIN_MID_MM), 3.f) * binned_floats(bin, COL_B_BIN_WIDTH_MM) // mm^3 * mm
					* (3600.f) // 3600 s in 1 h
					* 1000.f // 1000 mm in 1 m
					* (float) (1e-12) // (mm/m) ^ 4 conversion
					;
				IWC_g_m3 = (pi / 6.f) * rho_kg_m3 // kg/m^3
					* std::pow(binned_floats(bin, COL_B_BIN_MID_MM), 3.f) // mm^3
					* binned_floats(bin, COL_B_CONC_M_4) // m^-4
					* binned_floats(bin, COL_B_BIN_WIDTH_MM) // mm
					* (float) (1e-12) // m^-4 -> mm^-4
					* 1000.f // kg->g
					;
			}
			v_num_weighted_m_s = NddV_m_3_m_s / Ndd_m_3;
			v_mass_weighted_m_s = NddMV_m_3_kg_m_s / NddM_m_3_kg;

			plugins::hdf5::addAttr<float>(fpro, "v_mass_weighted_m_s", v_mass_weighted_m_s);
			plugins::hdf5::addAttr<float>(fpro, "v_num_weighted_m_s", v_num_weighted_m_s);
			plugins::hdf5::addAttr<float>(fpro, "S_mm_h", S_mm_h);
			plugins::hdf5::addAttr<float>(fpro, "IWC_g_m3", IWC_g_m3);
			// And add to the summary table
			profloats(pnum - 1, 0) = S_mm_h;
			profloats(pnum - 1, 1) = IWC_g_m3;
			profloats(pnum - 1, 2) = v_mass_weighted_m_s;
			profloats(pnum - 1, 3) = v_num_weighted_m_s;
		}
		auto dbps = plugins::hdf5::addDatasetEigen(file, "profile_summary", profloats);
		plugins::hdf5::addAttr<std::string>(dbps, "col_0", "S_mm_h");
		plugins::hdf5::addAttr<std::string>(dbps, "col_1", "IWC_g_m3");
		plugins::hdf5::addAttr<std::string>(dbps, "col_2", "v_mass_weighted_m_s");
		plugins::hdf5::addAttr<std::string>(dbps, "col_3", "v_num_weighted_m_s");

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
