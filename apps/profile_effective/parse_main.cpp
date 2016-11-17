#include "../../scatdb/defs.hpp"
/// This is a program that reads datasets from an hdf5 file.
/// It takes the desired datasets and writes them into the
/// specified text file or directory.
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <exception>
#include <iostream>
#include "parser.hpp"
#include "../../scatdb/logging.hpp"
#include "../../scatdb/error.hpp"
#include "../../scatdb/debug.hpp"

int main(int argc, char** argv) {
	using namespace std;
	try {
		namespace po = boost::program_options;
		po::options_description desc("Allowed options"), cmdline("Command-line options"),
			config("Config options"), hidden("Hidden options"), oall("all options");

		scatdb::debug::add_options(cmdline, config, hidden);

		cmdline.add_options()
			("help,h", "produce help message")
			("input,i", po::value<string>(), "Specify input profile")
			("output,o", po::value<string>(), "Specify output file (HDF5)")
			;

		desc.add(cmdline); //.add(config);
		oall.add(cmdline).add(hidden).add(config);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
			options(oall).run(), vm);
		po::notify(vm);

		using namespace scatdb;

		auto doHelp = [&](const std::string& s)
		{
			cout << s << endl;
			cout << desc << endl;
			exit(3);
		};
		if (vm.count("help") || argc <= 1) doHelp("");

		scatdb::debug::process_static_options(vm);
		if (!vm.count("input")) doHelp("Need to specify an input file.");
		if (!vm.count("output")) doHelp("Need to specify an output file.");
		string sInput = vm["input"].as<string>();
		string sOutput = vm["output"].as<string>();

		auto alldata = scatdb::profiles::forward_conc_table::import(sInput.c_str());
		
		cout << "Read " << alldata->size() << " cases." << endl;
		for (size_t i = 0; i < alldata->size(); ++i) {
			cout << "Case " << i << " temp " << alldata->at(i)->getTempC()
				<< " type " << scatdb::profiles::defs::stringify(alldata->at(i)->getParticleTypes())
				<< endl;
			cout << *(alldata->at(i)->getData().get()) << endl;
			std::string si = boost::lexical_cast<std::string>(i);
			alldata->at(i)->writeHDF5File(sOutput.c_str(), si.c_str());
		}
	}
	/// \todo Think of a method to rethrow an error without splicing.
	// Attempt a dynamic cast. Clone the object. Rethrow.
	// If dynamic cast fails, create an xOtherError object with the parameter of e.what(). Rethrow.
	catch (std::exception &e) {
		cerr << "An exception has occurred: " << e.what() << endl;
		return 2;
	}
	catch (...) {
		cerr << "An unknown exception has occurred." << endl;
		return 2;
	}
	return 0;
}