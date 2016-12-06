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
		const float pi = 3.141592654f;
		const float rho_g_m3 = (float) 9.16e5; // 916 kg/m^3 = 9.16e5 g/m3
		const float rho_wat_g_m3 = (float) 1e6; // 1000 kg/m3 = 1e6 g/m3

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

		if (!vm.count("flaketypes")) doHelp("Need to specify flake types");
		auto sdb = db::loadDB();
		// Select flake types. Remove duplicates, as we do not care about multiple frequencies.
		// Also, sort the data by max dimension.
		vector<string> vsflaketypes = vm["flaketypes"].as<vector<string> >();

		enum SINGLE_COLS_FLOATS {
			COL_MASS_KG,
			COL_MD_M,
			COL_FALLVEL_LH_UNRIMED,
			COL_FALLVEL_LH_RIMED,
			COL_VOLM3,
			COL_REFF_UM,
			NUM_COLS_FLOATS
		};
		typedef Eigen::Array<float, Eigen::Dynamic, NUM_COLS_FLOATS> Float_Raw_t;
		//vector<std::pair<std::string, Float_Raw_t> > rawtables;
		using namespace H5;
		cerr << "Creating the HDF5 file." << endl;
		std::shared_ptr<H5::H5File> file;
		boost::filesystem::path p(sout);
		file = std::shared_ptr<H5File>(new H5File(sout, H5F_ACC_TRUNC));
		auto baseshp = scatdb::plugins::hdf5::openOrCreateGroup(file, "per-shape-subset-then-profile");

		// Creating a final results table
		struct FinalResults_s {
			int idnum;
			const char* shapes;
			int profilenum;
			const char* profiledesc;
			float IWC_g_m3;
			float S_LH_unrimed_mm_h;
			float S_LH_rimed_mm_h;
			float v_LH_unrimed_m_s;
			float v_LH_rimed_m_s;
		};
		const size_t numProfs = allprofiles->size();
		const size_t numFlakeTypes = vsflaketypes.size();
		const size_t numFinalResultsRows = numProfs * numFlakeTypes;
		vector<FinalResults_s> vFinal;
		vFinal.resize(numFinalResultsRows);
		//for (const auto &prof : *(allprofiles.get()))

		cerr << "Subsetting and sorting: ";
		int finalResRow = 0;
		for (const auto &s : vsflaketypes) {
			cerr << ".";
			cerr.flush();
			SDBR_log("profile_shape", logging::NOTIFICATION, "Subsetting and sorting flake types " << s);
			auto f = filter::generate();
			f->addFilterInt(db::data_entries::SDBR_FLAKETYPE, s);
			auto ff = f->apply(sdb);
			// Sort by max dimension, then aeff, then frequency and remove values that are duplicated across frequency
			typedef tuple<int, int, int, size_t> sort_t;
			vector<sort_t> id_sorter;
			int raw_rows = (int) ff->intMat.rows();
			id_sorter.resize(raw_rows);
			for (int i = 0; i < raw_rows; ++i) {
				std::get<0>(id_sorter[i]) = (int) ff->floatMat(i, db::data_entries::SDBR_MAX_DIMENSION_MM) * 100;
				std::get<1>(id_sorter[i]) = (int) ff->floatMat(i, db::data_entries::SDBR_AEFF_UM) * 100;
				std::get<2>(id_sorter[i]) = (int) ff->floatMat(i, db::data_entries::SDBR_FREQUENCY_GHZ);
				std::get<3>(id_sorter[i]) = i;
			}
			std::sort(id_sorter.begin(), id_sorter.end(), [](const sort_t &lhs, const sort_t &rhs) {
				if (std::get<0>(lhs) != std::get<0>(rhs)) return std::get<0>(lhs) < std::get<0>(rhs);
				if (std::get<1>(lhs) != std::get<1>(rhs)) return std::get<1>(lhs) < std::get<1>(rhs);
				return std::get<2>(lhs) < std::get<2>(rhs);
			});
			list<sort_t> id_sorter_filtered;
			//id_sorter_filtered.reserve(raw_rows);
			auto ot = id_sorter.cbegin();
			for (auto it = id_sorter.cbegin(); it != id_sorter.cend(); ++it) {
				if (it == id_sorter.cbegin()) continue;
				bool isNew = false;
				if (std::get<0>(*it) != std::get<0>(*ot)) isNew = true;
				else if (std::get<1>(*it) != std::get<1>(*ot)) isNew = true;
				if (isNew) {
					id_sorter_filtered.push_back(*it);
				}
				ot = it;
			}
			Float_Raw_t filtFloats;
			filtFloats.resize((int)id_sorter_filtered.size(), NUM_COLS_FLOATS);
			int j = 0;
			for (const auto &i : id_sorter_filtered) {
				// Translate from raw table into our new column scheme
				auto res = filtFloats.block<1, NUM_COLS_FLOATS>(j, 0);
				res(0, COL_MD_M) = ff->floatMat((int)std::get<3>(i), db::data_entries::SDBR_MAX_DIMENSION_MM) / 1000.f;
				res(0, COL_REFF_UM) = ff->floatMat((int)std::get<3>(i), db::data_entries::SDBR_AEFF_UM);
				res(0, COL_VOLM3) = (4.f * pi / 3.f) * std::pow(res(0, COL_REFF_UM) / (float) 1.e6, 3.f);
				res(0, COL_MASS_KG) = rho_g_m3 * res(0, COL_VOLM3) / 1000.f;
				res(0, COL_FALLVEL_LH_UNRIMED) = 0.82f * std::pow(res(0, COL_MD_M)*1000.f, 0.12f);
				res(0, COL_FALLVEL_LH_RIMED) = 0.79f * std::pow(res(0, COL_MD_M)*1000.f, 0.27f);
				++j;
			}
			//rawtables.push_back(std::pair<string, Float_Raw_t>(s, std::move(filtFloats)));

			// Write the filtered shape data, then iterate over the profiles
			// The containing group
			auto hshapeset = scatdb::plugins::hdf5::openOrCreateGroup(baseshp, s.c_str());
			// The filtered data
			auto hshape = plugins::hdf5::addDatasetEigen(hshapeset, "ShapeDataRaw", filtFloats);
			plugins::hdf5::addAttr<std::string>(hshape, "col_0", std::string("COL_MASS_KG"));
			plugins::hdf5::addAttr<std::string>(hshape, "col_1", std::string("COL_MD_M"));
			plugins::hdf5::addAttr<std::string>(hshape, "col_2", std::string("COL_FALLVEL_LH_UNRIMED_M_S"));
			plugins::hdf5::addAttr<std::string>(hshape, "col_3", std::string("COL_FALLVEL_LH_RIMED_M_S"));
			plugins::hdf5::addAttr<std::string>(hshape, "col_4", std::string("COL_VOL_M3"));
			plugins::hdf5::addAttr<std::string>(hshape, "col_5", std::string("COL_REFF_UM"));
			// Add a dataset that tags the names of the filtered data
			// TODO

			auto hProfiles = plugins::hdf5::openOrCreateGroup(hshapeset, "Profiles");
			// Profile summary table
			enum Profile_Summary_Per_ShapeSet_Names {
				SUMMARY_SHAPESET_IWC_g_m3,
				SUMMARY_SHAPESET_S_LH74_UNRIMED_mm_h,
				SUMMARY_SHAPESET_S_LH74_RIMED_mm_h,
				SUMMARY_SHAPESET_v_mass_weighted_LH74_UNRIMED_m_s,
				SUMMARY_SHAPESET_v_mass_weighted_LH74_RIMED_m_s,
				NUM_PROFILE_SUMMARY_PER_SHAPESET_NAMES
			};
			Eigen::Array<float, Eigen::Dynamic, NUM_PROFILE_SUMMARY_PER_SHAPESET_NAMES> profloats;
			profloats.resize((int)allprofiles->size(), NUM_PROFILE_SUMMARY_PER_SHAPESET_NAMES);
			profloats.setZero();

			// Iterate over the profiles
			int pnum = 0;
			for (const auto &prof : *(allprofiles.get()))
			{
				++pnum;
				string profid("profile_");
				profid.append(boost::lexical_cast<string>(pnum));
				cerr << "\tProfile " << profid << endl << "\t\tWriting..." << endl;
				auto fpro = scatdb::plugins::hdf5::openOrCreateGroup(hProfiles, profid.c_str());
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
					std::array<acc_type, NUM_COLS_FLOATS> accs;
					int count = 0;
					// Push the data to the accumulator functions.
					brow(0, COL_B_SHAPE_LOW) = (float)lastSortedShapeRow;
					for (int i = lastSortedShapeRow; i<filtFloats.rows(); ++i) {
						const auto &shpfloats = filtFloats.block<1, NUM_COLS_FLOATS>(i, 0);
						if (shpfloats(COL_MD_M) * 1000.f < brow(0, COL_B_BIN_LOW_MM)) continue;
						if (shpfloats(COL_MD_M) * 1000.f > brow(0, COL_B_BIN_HIGH_MM)) break;
						lastSortedShapeRow = i;

						for (int j = 0; j < NUM_COLS_FLOATS; ++j) {
							accs[j]((double)shpfloats(0, j));
						}
						++count;
					}
					brow(0, COL_B_SHAPE_HIGH) = (float)lastSortedShapeRow;

					if (count) {
						brow(0, COL_B_NUM_IN_BIN) = (float)count;
						brow(0, COL_B_MASS_KG) = (float)boost::accumulators::mean(accs[COL_MASS_KG]);
						brow(0, COL_B_MD_M) = (float)boost::accumulators::mean(accs[COL_MD_M]);
						brow(0, COL_B_FALLVEL_LH_UNRIMED) = (float)boost::accumulators::mean(accs[COL_FALLVEL_LH_UNRIMED]);
						brow(0, COL_B_FALLVEL_LH_RIMED) = (float)boost::accumulators::mean(accs[COL_FALLVEL_LH_RIMED]);
						brow(0, COL_B_VOL_M_3) = (float)boost::accumulators::mean(accs[COL_VOLM3]);
						brow(0, COL_B_PARTIAL_IWC_G_M3) =
							brow(0, COL_B_CONC_M_4) // m^-4
							* brow(0, COL_B_BIN_WIDTH_MM) / 1000.f // m
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
					NddM_m_3_kg += binned_floats(bin, COL_B_CONC_M_4) * binned_floats(bin, COL_B_BIN_WIDTH_MM) / 1000.f * binned_floats(bin, COL_B_MASS_KG);
					NddMVLHU_m_3_kg_m_s += binned_floats(bin, COL_B_CONC_M_4) * binned_floats(bin, COL_B_BIN_WIDTH_MM) / 1000.f
						* binned_floats(bin, COL_B_FALLVEL_LH_UNRIMED) * binned_floats(bin, COL_B_MASS_KG);
					NddMVLHR_m_3_kg_m_s += binned_floats(bin, COL_B_CONC_M_4) * binned_floats(bin, COL_B_BIN_WIDTH_MM) / 1000.f
						* binned_floats(bin, COL_B_FALLVEL_LH_RIMED) * binned_floats(bin, COL_B_MASS_KG);

					S_LH_UNRIMED_mm_h += binned_floats(bin, COL_B_PARTIAL_S_LH_UNRIMED_MM_H);
					S_LH_RIMED_mm_h += binned_floats(bin, COL_B_PARTIAL_S_LH_RIMED_MM_H);

					IWC_g_m3 +=
						binned_floats(bin, COL_B_CONC_M_4) // m^-4
						* binned_floats(bin, COL_B_BIN_WIDTH_MM) / 1000.f // m
						* binned_floats(bin, COL_B_MASS_KG) * 1000.f // g
						;
				}
				//v_num_weighted_m_s = NddV_m_3_m_s / Ndd_m_3;
				v_LH_UNRIMED_mass_weighted_m_s = NddMVLHU_m_3_kg_m_s / NddM_m_3_kg;
				v_LH_RIMED_mass_weighted_m_s = NddMVLHR_m_3_kg_m_s / NddM_m_3_kg;

				plugins::hdf5::addAttr<float>(fpro, "v_LH_UNRIMED_mass_weighted_m_s", v_LH_UNRIMED_mass_weighted_m_s);
				plugins::hdf5::addAttr<float>(fpro, "v_LH_RIMED_mass_weighted_m_s", v_LH_RIMED_mass_weighted_m_s);
				plugins::hdf5::addAttr<float>(fpro, "S_LH_UNRIMED_mm_h", S_LH_UNRIMED_mm_h);
				plugins::hdf5::addAttr<float>(fpro, "S_LH_RIMED_mm_h", S_LH_RIMED_mm_h);
				plugins::hdf5::addAttr<float>(fpro, "IWC_g_m3", IWC_g_m3);
				// And add to the summary table
				profloats(pnum - 1, SUMMARY_SHAPESET_S_LH74_UNRIMED_mm_h) = S_LH_UNRIMED_mm_h;
				profloats(pnum - 1, SUMMARY_SHAPESET_S_LH74_RIMED_mm_h) = S_LH_RIMED_mm_h;
				profloats(pnum - 1, SUMMARY_SHAPESET_IWC_g_m3) = IWC_g_m3;
				profloats(pnum - 1, SUMMARY_SHAPESET_v_mass_weighted_LH74_UNRIMED_m_s) = v_LH_UNRIMED_mass_weighted_m_s;
				profloats(pnum - 1, SUMMARY_SHAPESET_v_mass_weighted_LH74_RIMED_m_s) = v_LH_RIMED_mass_weighted_m_s;
				//profloats(pnum - 1, 4) = v_num_weighted_m_s;

				// The strings are already allocated elsewhere, and they do not go out of scope.
				vFinal[finalResRow].idnum = finalResRow;
				vFinal[finalResRow].IWC_g_m3 = IWC_g_m3;
				vFinal[finalResRow].profiledesc = scatdb::profiles::defs::stringify(pts);
				vFinal[finalResRow].profilenum = pnum;
				vFinal[finalResRow].shapes = s.c_str();
				vFinal[finalResRow].S_LH_rimed_mm_h = S_LH_RIMED_mm_h;
				vFinal[finalResRow].S_LH_unrimed_mm_h = S_LH_UNRIMED_mm_h;
				vFinal[finalResRow].v_LH_rimed_m_s = v_LH_RIMED_mass_weighted_m_s;
				vFinal[finalResRow].v_LH_unrimed_m_s = v_LH_UNRIMED_mass_weighted_m_s;
				finalResRow++;
			}
			cerr << "All profiles done for set " << s << ". Writing summary table." << endl;
			auto dbps = plugins::hdf5::addDatasetEigen(hshapeset, "profile_summary", profloats);
			plugins::hdf5::addAttr<std::string>(dbps, "col_01", std::string("S_LH_UNRIMED_mm_h"));
			plugins::hdf5::addAttr<std::string>(dbps, "col_02", std::string("S_LH_RIMED_mm_h"));
			plugins::hdf5::addAttr<std::string>(dbps, "col_00", std::string("IWC_g_m3"));
			plugins::hdf5::addAttr<std::string>(dbps, "col_03", std::string("v_LH_UNRIMED_mass_weighted_m_s"));
			plugins::hdf5::addAttr<std::string>(dbps, "col_04", std::string("v_LH_RIMED_mass_weighted_m_s"));

		}

		// Create the final summary dataset, which has a compound type.
		hsize_t dim[1] = { (hsize_t) vFinal.size() };
		DataSpace space(1, dim);
		CompType sType(sizeof(FinalResults_s));
		H5::StrType strtype(0, H5T_VARIABLE);

		/*struct FinalResults_s {
			int idnum;
			const char* shapes;
			int profilenum;
			const char* profiledesc;
			float IWC_g_m3;
			float S_LH_unrimed_mm_h;
			float S_LH_rimed_mm_h;
			float v_LH_unrimed_m_s;
			float v_LH_rimed_m_s;
		};*/

		sType.insertMember("ID", HOFFSET(FinalResults_s, idnum), PredType::NATIVE_INT);
		sType.insertMember("Shapes", HOFFSET(FinalResults_s, shapes), strtype);
		sType.insertMember("Profile Number", HOFFSET(FinalResults_s, profilenum), PredType::NATIVE_INT);
		sType.insertMember("Profile Description", HOFFSET(FinalResults_s, profiledesc), strtype);
		sType.insertMember("IWC [g/m3]", HOFFSET(FinalResults_s, IWC_g_m3), PredType::NATIVE_FLOAT);
		sType.insertMember("LESR (LH_unrimed) [mm/h]", HOFFSET(FinalResults_s, S_LH_unrimed_mm_h), PredType::NATIVE_FLOAT);
		sType.insertMember("LESR (LH_rimed) [mm/h]", HOFFSET(FinalResults_s, S_LH_rimed_mm_h), PredType::NATIVE_FLOAT);
		sType.insertMember("Mean Velocity (LH_unrimed) [m/s]", HOFFSET(FinalResults_s, v_LH_unrimed_m_s), PredType::NATIVE_FLOAT);
		sType.insertMember("Mean Velocity (LH_rimed) [m/s]", HOFFSET(FinalResults_s, v_LH_rimed_m_s), PredType::NATIVE_FLOAT);

		std::shared_ptr<DataSet> g(new DataSet(file->createDataSet("Overall_Results", sType, space)));
		g->write(vFinal.data(), sType);
		
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
