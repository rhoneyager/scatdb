#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
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
			("output,o", po::value<string>(), "Output file")
			("db-file,d", "Manually specify database location")
			("flaketypes,y", po::value<vector<string> >()->multitoken(),
			 "Filter flaketypes by number range")
			("frequencies,f", po::value<vector<string> >()->multitoken(),
			 "Filter frequencies (GHz) by range")
			("temp,T", po::value<vector<string> >()->multitoken(),
			 "Filter temperatures (K) by range")
			("aeff,a", po::value<vector<string> >()->multitoken(),
			 "Range filter by effective radius (um)")
			("max-dimension,m", po::value<vector<string> >()->multitoken(),
			 "Range filter by maximum dimension (mm)")
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

		if (!vm.count("output")) doHelp("Must specify output file");
		using namespace scatdb;
		std::string dbfile;
		if (vm.count("db-file")) dbfile = vm["db-file"].as<string>();
		db::findDB(dbfile);
		if (!dbfile.size()) doHelp("Unable to detect database file.");
		cerr << "Database file is at: " << dbfile << endl;
		auto sdb = db::loadDB();
		cerr << "Database loaded. Performing filtering." << endl;

		vector<string> vFlakeTypes(1,"1/50"), vFreqs(1,"1/500"), vTemps(1,"200/300"),
			vAeffs(1,"1/50000"), vMDs(1,"1/50000");
		if (vm.count("flaketypes")) vFlakeTypes = vm["flaketypes"].as<vector<string> >();
		if (vm.count("frequencies")) vFreqs = vm["frequencies"].as<vector<string> >();
		if (vm.count("temp")) vTemps = vm["temp"].as<vector<string> >();
		if (vm.count("aeff")) vAeffs = vm["aeff"].as<vector<string> >();
		if (vm.count("max-dimension")) vMDs = vm["max-dimension"].as<vector<string> >();

		std::string fout = vm["output"].as<std::string>();
		cerr << "Writing output to " << fout << endl;
		std::ofstream out(fout.c_str());
		auto makeHeader = [&]() {
			out << "filter_ft\tfilter_freq_GHz\tfilter_temp_K\t"
				"filter_aeff_UM\tfilter_maximum_dimension_MM";
			for (int i=0; i<db::data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS; ++i) {
				out << "\t" << db::data_entries::stringify<float>(i) << "_min"
					<< "\t" << db::data_entries::stringify<float>(i) << "_max"
					<< "\t" << db::data_entries::stringify<float>(i) << "_median"
					<< "\t" << db::data_entries::stringify<float>(i) << "_mean"
					<< "\t" << db::data_entries::stringify<float>(i) << "_sd";
			}
			out << std::endl;
		};
		makeHeader();
		cout << "Header written" << std::endl;

		for (const auto &sFT : vFlakeTypes)
		for (const auto &sFr : vFreqs)
		for (const auto &sT  : vTemps)
		for (const auto &sAe : vAeffs)
		for (const auto &sMD : vMDs)
		{
			auto f = filter::generate();

			cout << sFT << "\t" << sFr << "\t" << sT << "\t" << sAe << "\t" << sMD << std::endl;
			//f->addFilterInt(db::data_entries::SDBR_FLAKETYPE, sFT);
			f->addFilterFloat(db::data_entries::SDBR_FREQUENCY_GHZ, sFr);
			f->addFilterFloat(db::data_entries::SDBR_TEMPERATURE_K, sT);
			f->addFilterFloat(db::data_entries::SDBR_AEFF_UM, sAe);
			f->addFilterFloat(db::data_entries::SDBR_MAX_DIMENSION_MM, sMD);

			auto sdb_filtered = f->apply(sdb);
			auto stats = sdb_filtered->getStats();
			if (stats->count == 0) continue;

			out << sFT << "\t" << sFr << "\t" << sT << "\t" << sAe << "\t" << sMD << "\t";


//				out << "\tMIN\tMAX\tMEDIAN\tMEAN\tSD\tSKEWNESS\tKURTOSIS\n";
			for (int i=0; i<db::data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS; ++i) {
	//			out << data_entries::SDBR_stringify<float>(i) << std::endl;
				out << "\t" << stats->floatStats(db::data_entries::SDBR_S_MIN,i) << "\t"
					<< stats->floatStats(db::data_entries::SDBR_S_MAX,i) << "\t"
					<< stats->floatStats(db::data_entries::SDBR_MEDIAN,i) << "\t"
					<< stats->floatStats(db::data_entries::SDBR_MEAN,i) << "\t"
					<< stats->floatStats(db::data_entries::SDBR_SD,i);
			}
			out << std::endl;
			//stats->print(cerr);
		}


	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}
