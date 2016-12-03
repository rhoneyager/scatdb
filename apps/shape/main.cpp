#include "../../scatdb/defs.hpp"
/// This is a program that reads datasets from an hdf5 file.
/// It takes the desired datasets and writes them into the
/// specified text file or directory.
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <exception>
#include <iostream>
#include "../../scatdb/logging.hpp"
#include "../../scatdb/error.hpp"
#include "../../scatdb/hash.hpp"
#include "../../scatdb/debug.hpp"
#include "../../scatdb/units/units.hpp"
#include "../../scatdb/shape/shape.hpp"
#include "../../scatdb/shape/shapeForwards.hpp"
#include "../../scatdb/shape/shapeIO.hpp"

int main(int argc, char** argv) {
	using namespace std;
	int retval = 0;
	try {
		using namespace scatdb;
		namespace po = boost::program_options;

		po::options_description desc("Allowed options"), cmdline("Command-line options"),
			config("Config options"), hidden("Hidden options"), oall("all options");
		scatdb::debug::add_options(cmdline, config, hidden);

		cmdline.add_options()
			("help,h", "produce help message")
			("input,i", po::value< vector<string> >()->multitoken(), "Input shapes")
			("set-dipole-spacing", po::value<double>(), "Set dipole spacing (um)")
			("output,o", po::value< string >(), "Output file")
			("output-type", po::value<string>()->default_value(""), "Output file type (ddscat, raw, hdf5)")
			;

		desc.add(cmdline).add(config);
		oall.add(cmdline).add(config).add(hidden);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
			options(oall).run(), vm);
		po::notify(vm);

		scatdb::debug::process_static_options(vm);

		auto doHelp = [&](const std::string &m) { cerr << desc << "\n" << m << endl; exit(1); };
		if (vm.count("help") || argc < 2) doHelp("");
		if (!vm.count("input")) doHelp("Must specify input files.");
		vector<string> sinputs = vm["input"].as<vector<string> >();
		auto sio = shape::shapeIO::generate();
		sio->shapes.reserve(sinputs.size());
		std::vector<std::shared_ptr<scatdb::shape::shape> > modifiableShapes;
		for (const auto &fi : sinputs) sio->readFile(fi, modifiableShapes);

		double dSpacing = 0;
		if (vm.count("set-dipole-spacing")) dSpacing = vm["set-dipole-spacing"].as<double>();
		for (auto &shp : modifiableShapes) {
			cerr << shp->hash()->lower << endl;
			if (dSpacing) shp->setPreferredDipoleSpacing(dSpacing);
		}

		string soutput;
		if (vm.count("output")) soutput = vm["output"].as<string>();
		string stype = vm["output-type"].as<string>();
		if (soutput.size()) {
			cerr << "Writing output file: " << soutput << endl;
			sio->writeFile(soutput,stype);
		}
	}
	catch (std::exception &e) {
		cerr << "An exception has occurred: " << e.what() << endl;
		retval = 2;
	}
	return retval;
}
