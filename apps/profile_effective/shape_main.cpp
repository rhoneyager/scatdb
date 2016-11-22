#include "../../scatdb/defs.hpp"
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
			COL_AEFF,
			COL_MD,
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
		size_t i = 0;
		for (const auto &s : vsshapes) {
			sio.readFile(s);
			auto &bints = sints.block<1, NUM_COLS_INTS>(i, 0);
			auto &bfloats = sfloats.block<1, NUM_COLS_FLOATS>(i, 0);
			auto shp = sio.shapes[i];
			float ds = (float) shp->getPreferredDipoleSpacing();
			if (!ds) ds = 40.f;
			bints(COL_ID) = shp->hash()->lower;
			bints(COL_NUM_LATTICE) = (uint64_t) shp->numPoints();
			bfloats(COL_AEFF) = ds * std::pow(3.f*((float)shp->numPoints()) / (4.f*3.141592654f), 1.f / 3.f);
			
			for (int axis = 0; axis < 3; ++axis) {
				auto shpproj = shape::algorithms::projectShape(shp, axis);
				float pmd = 0, parea = 0, pcaf = 0;
				shape::algorithms::getProjectedStats(shpproj, pmd, parea, pcaf);

				if ((ds * pmd) > bfloats(COL_MD)) bfloats(COL_MD) = ds * pmd;
				bfloats(COL_PROJAREA) += ds * (1.f / 3.f) * parea;
				bfloats(COL_CIRCUMAREAFRAC) += (1.f / 3.f) * pcaf;
			}
			// Fall velocity is calculated directly from the final projected area.
			bfloats(COL_FALLVEL);
			++i;
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
		auto base = scatdb::plugins::hdf5::openOrCreateGroup(file, "output");
		auto fsbase = scatdb::plugins::hdf5::openOrCreateGroup(base, "shape");


		// Iterate over the profiles
		{
			// Perform the binning, and write to the binned tables

			// Calculate the bulk and mass-weighted quantities. Write these also.
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
