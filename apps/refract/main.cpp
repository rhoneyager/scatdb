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
#include "../../scatdb/debug.hpp"
#include "../../scatdb/refract/refract.hpp"
#include "../../scatdb/units/units.hpp"

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
			("list-all", "List all refractive index providers")
			("list-subst", po::value<string>(), "List all refractive index providers for a given substance")
			("subst", po::value<string>(), "Substance of interest (ice, water)")
			("freq,f", po::value<double>(), "Frequency")
			("freq-units", po::value<string>()->default_value("GHz"), "Frequency units")
			("temp,T", po::value<double>(), "Temperature")
			("temp-units", po::value<string>()->default_value("K"), "Temperature units")
			;

		po::positional_options_description p;
		p.add("subst", 1);
		p.add("freq", 2);
		p.add("freq-units", 3);
		p.add("temp", 4);
		p.add("temp-units", 5);

		desc.add(cmdline).add(config);
		oall.add(cmdline).add(config).add(hidden);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).
			options(oall).positional(p).run(), vm);
		po::notify(vm);

		scatdb::debug::process_static_options(vm);

		auto doHelp = [&](const std::string &m) { cerr << desc << "\n" << m << endl; exit(1); };
		if (vm.count("help") || argc < 2) doHelp("");

		if (vm.count("list-all")) {
			auto ps = scatdb::refract::listAllProviders();
			scatdb::refract::enumProviders(ps);
			return 0;
		}
		if (vm.count("list-subst")) {
			string lsubst = vm["list-subst"].as<string>();
			auto ps = scatdb::refract::listAllProviders(lsubst);
			scatdb::refract::enumProviders(ps);
			return 0;
		}
		if (!vm.count("subst")) doHelp("Must specify a substance.");
		string subst = vm["subst"].as<string>();
		string freqUnits = vm["freq-units"].as<string>();
		string tempUnits = vm["temp-units"].as<string>();
		bool hasFreq = false, hasTemp = false;
		double inFreq = 0, inTemp = 0;
		if (vm.count("freq")) { hasFreq = true; inFreq = vm["freq"].as<double>(); }
		if (vm.count("temp")) { hasTemp = true; inTemp = vm["temp"].as<double>(); }
		complex<double> m(0, 0);

		// If an exact provider is specified by name, only a single entry is returned.
		// Otherwise, all possible matches are returned.
		auto provAll = scatdb::refract::findProviders(subst, hasFreq, hasTemp);
		// Iterate until a provider works.
		bool found = false;
		string prov;
		for (const auto &p : *(provAll.get())) {
			prov = p.second->name;
			if (p.second->speciality_function_type == scatdb::refract::provider_s::spt::FREQTEMP) {
				scatdb::refract::refractFunction_freq_temp_t f;
				scatdb::refract::prepRefract(p.second, freqUnits, tempUnits, f);
				if (f) {
					try {
						f(inFreq, inTemp, m);
						found = true;
						break;
					}
					catch (std::exception &e) { if (prov == subst) cerr << e.what(); } // Out of range
				}
			}
			else if (p.second->speciality_function_type == scatdb::refract::provider_s::spt::FREQ) {
				scatdb::refract::refractFunction_freqonly_t f;
				scatdb::refract::prepRefract(p.second, freqUnits, f);
				if (f) {
					try {
						f(inFreq, m);
						found = true;
						break;
					}
					catch (std::exception &e) { if (prov == subst) cerr << e.what(); } // Out of range
				}
			}
			else {
				continue;
			}
		}
		if (found) {
			cout << m << "\twas found using provider " << prov << "." << endl;
		}
		else {
			cerr << "A refractive index provider that could handle the input cannot be found." << endl;
			return 3;
		}
		//std::shared_ptr<scatdb::units::converter> cnv;
		//cnv = std::shared_ptr<scatdb::units::converter>(new scatdb::units::converter(inUnits, outUnits));
	}
	catch (std::exception &e) {
		cerr << "An exception has occurred: " << e.what() << endl;
		retval = 2;
	}
	return retval;
}
