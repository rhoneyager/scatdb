#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include "../../scatdb/debug.hpp"
#include "../../scatdb/splitSet.hpp"
#include "../../scatdb/scatdb.hpp"

int main(int argc, char** argv) {
	try {
		using namespace std;
		namespace po = boost::program_options;
		po::options_description desc("Allowed options"), cmdline("Command-line options"),
			config("Config options"), hidden("Hidden options"), oall("all options");

		scatdb::debug::add_options(cmdline, config, hidden);

		cmdline.add_options()
			("help,h", "produce help message")
			("stats", "Print stats for selected data")
			("xaxis,x", po::value<string>()->default_value("aeff"), "Specify independent "
			 "axis for interpolation and lowess regression. Default is aeff. Can also use "
			 "'md' for max dimension.")
			("lowess", "Use LOWESS Routine to regress data")
			//("interp", "Perform linear interpolation over the x axis.")
			("sort", "Sort values by the corresponding x axis.")
			//("integ", po::value<string>(), "Integrate using Sekhon-Srivastava distribution for "
			// "the specified rainfall rates [mm/h]. Suggested 0.1:0.1:3.")
			("output,o", po::value<string>(), "Output file")
			("flaketypes,y", po::value<string>(), "Filter flaketypes by number range")
			("frequencies,f", po::value<string>(), "Filter frequencies (GHz) by range")
			("temp,T", po::value<string>(), "Filter temperatures (K) by range")
			("aeff,a", po::value<string>(), "Range filter by effective radius (um)")
			("max-dimension,m", po::value<string>(), "Range filter by maximum dimension (mm)")
			("cabs", po::value<string>(), "Range filter by absorption cross-section (m^2)")
			("cbk", po::value<string>(), "Range filter by backscattering cross-section (m^2)")
			("cext", po::value<string>(), "Range filter by extinction cross-section (m^2)")
			("csca", po::value<string>(), "Range filter by scattering cross-section (m^2)")
			("asymmetry,g", po::value<string>(), "Range filter by asymmetry parameter")
			("ar", po::value<string>(), "Range filter by aspect ratio")
			("list-flaketypes", "List valid flaketypes")
			;

		desc.add(cmdline); //.add(config);
		oall.add(cmdline).add(hidden).add(config);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
			options(oall).run(), vm);
		po::notify(vm);


		auto doHelp = [&](const std::string& s)
		{
			cout << s << endl;
			cout << desc << endl;
			exit(3);
		};
		if (vm.count("help") || argc <= 1) doHelp("");

		if (vm.count("list-flaketypes")) {
			// This may eventually be a separate file.
			cerr << "Flake Category Listing\nId\t\tDescription\n"
				"----------------------------------------------------------\n"
				"0\t\tLiu [2004] Long hexagonal column l/d=4\n"
				"1\t\tLiu [2004] Short hexagonal column l/d=2\n"
				"2\t\tLiu [2004] Block hexagonal column l/d=1\n"
				"3\t\tLiu [2004] Thick hexagonal plate l/d=0.2\n"
				"4\t\tLiu [2004] Thin hexagonal plate l/d=0.05\n"
				"5\t\tLiu [2008] 3-bullet rosette\n"
				"6\t\tLiu [2008] 4-bullet rosette\n"
				"7\t\tLiu [2008] 5-bullet rosette\n"
				"8\t\tLiu [2008] 6-bullet rosette\n"
				"9\t\tLiu [2008] sector-like snowflake\n"
				"10\t\tLiu [2008] dendrite snowflake\n"
				"20\t\tNowell, Liu and Honeyager [2013] Rounded\n"
				"21\t\tHoneyager, Liu and Nowell [2016] Oblate\n"
				"22\t\tHoneyager, Liu and Nowell [2016] Prolate\n"
				;
			cerr << std::endl;
			exit(0);
		}
		
		scatdb::debug::process_static_options(vm);

		using namespace scatdb;
		auto sdb = db::loadDB();
		//cerr << "Database loaded. Performing filtering." << endl;

		auto f = filter::generate();

		if (vm.count("flaketypes")) f->addFilterInt(db::data_entries::SDBR_FLAKETYPE, vm["flaketypes"].as<string>());
		if (vm.count("frequencies")) f->addFilterFloat(db::data_entries::SDBR_FREQUENCY_GHZ, vm["frequencies"].as<string>());
		if (vm.count("temp")) f->addFilterFloat(db::data_entries::SDBR_TEMPERATURE_K, vm["temp"].as<string>());
		if (vm.count("aeff")) f->addFilterFloat(db::data_entries::SDBR_AEFF_UM, vm["aeff"].as<string>());
		if (vm.count("max-dimension")) f->addFilterFloat(db::data_entries::SDBR_MAX_DIMENSION_MM, vm["max-dimension"].as<string>());
		if (vm.count("cabs")) f->addFilterFloat(db::data_entries::SDBR_CABS_M, vm["cabs"].as<string>());
		if (vm.count("cbk")) f->addFilterFloat(db::data_entries::SDBR_CBK_M, vm["cbk"].as<string>());
		if (vm.count("cext")) f->addFilterFloat(db::data_entries::SDBR_CEXT_M, vm["cext"].as<string>());
		if (vm.count("csca")) f->addFilterFloat(db::data_entries::SDBR_CSCA_M, vm["csca"].as<string>());
		if (vm.count("asymmetry")) f->addFilterFloat(db::data_entries::SDBR_G, vm["asymmetry"].as<string>());
		if (vm.count("ar")) f->addFilterFloat(db::data_entries::SDBR_AS_XY, vm["ar"].as<string>());

		auto sdb_filtered = f->apply(sdb);
		string sxaxis = vm["xaxis"].as<string>();
		db::data_entries::data_entries_floats xaxis = db::data_entries::SDBR_AEFF_UM;
		if (sxaxis == "md") xaxis = db::data_entries::SDBR_MAX_DIMENSION_MM;

		auto s_sorted = sdb_filtered;
		if (vm.count("sort"))
			s_sorted = sdb_filtered->sort(xaxis);

		auto le_filtered = s_sorted;
		if (vm.count("lowess")) {
			le_filtered = s_sorted->regress(xaxis);
		}
		if (vm.count("stats")) {
			auto stats = le_filtered->getStats();
			cerr << "Stats tables:" << endl;
			stats->print(cerr);
		}
		auto interp_filtered = le_filtered;
		//if (vm.count("interp")) {
		//	interp_filtered = le_filtered->interpolate(xaxis);
		//}

		if (vm.count("integ")) {
			std::string sinteg = vm["integ"].as<std::string>();
		}

		if (vm.count("output")) {
			std::string fout = vm["output"].as<std::string>();
			//cerr << "Writing output to " << fout << endl;
			using namespace boost::filesystem;
			path pout(fout);
			if (pout.extension().string() == ".hdf5") {
				interp_filtered->writeHDFfile(fout.c_str(),
					SDBR_write_type::SDBR_TRUNCATE);
			} else {
				interp_filtered->writeTextFile(fout.c_str());
			}
		}
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}
