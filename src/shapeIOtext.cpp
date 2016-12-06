#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <map>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cstdio>

#include "../scatdb/error.hpp"
#include "../scatdb/shape/shape.hpp"
#include "../private/shapeIOtext.hpp"

namespace scatdb {
	namespace plugins {
		namespace builtin {
			namespace shape {
				using namespace scatdb::registry;
				const char* PLUGINID = "scatdb::plugins::builtin::shape";
				struct shape_text_handle : public scatdb::registry::IOhandler
				{
					shape_text_handle(const char* filename, IOtype t) : IOhandler(PLUGINID) {
						open(filename, t);
					}
					virtual ~shape_text_handle() {}
					std::shared_ptr<std::ifstream> in;
					std::shared_ptr<std::ofstream> out;
					void open(const char* filename, IOtype t) {
						using namespace boost::filesystem;
						switch (t)
						{
						case IOtype::EXCLUSIVE:
						case IOtype::DEBUG:
							SDBR_throw(scatdb::error::error_types::xOtherError)
								.add<std::string>("Reason", "Unsupported IOtype")
								.add<std::string>("filename", std::string(filename));
							break;
						case IOtype::READONLY:
							if (!exists(path(filename))) SDBR_throw(scatdb::error::error_types::xMissingFile)
								.add<std::string>("filename", std::string(filename))
								.add<std::string>("Reason", "Attempting to open a file for reading, but the file does not exist.");
							in = std::shared_ptr<std::ifstream>(new std::ifstream(filename));
							break;
						case IOtype::CREATE:
							if (exists(path(filename))) SDBR_throw(scatdb::error::error_types::xFileExists)
								.add<std::string>("filename", std::string(filename))
								.add<std::string>("Reason", "Attempting to create a file, which already exists.");
						case IOtype::TRUNCATE:
							out = std::shared_ptr<std::ofstream>(new std::ofstream(filename, std::ios_base::trunc));
							break;
						case IOtype::READWRITE: // Append mode, in this case
						{
							//bool e = false;
							//if (exists(path(filename))) e = true;
							out = std::shared_ptr<std::ofstream>(new std::ofstream(filename, std::ios_base::app));
						}
						break;
						}
					}
				};
				void register_shape_text() {
					const int nexts = 2;
					const char *exts[nexts] = { "shp", "dat" };
					genAndRegisterIOregistryPlural_writer<::scatdb::shape::shapeIO,
						scatdb::shape::shape_IO_output_registry>(nexts, exts, PLUGINID);
					genAndRegisterIOregistryPlural_reader<::scatdb::shape::shapeIO,
						scatdb::shape::shape_IO_input_registry>(nexts, exts, PLUGINID);
				}
				std::shared_ptr<IOhandler> export_raw(std::shared_ptr<IOhandler> sh, std::shared_ptr<options> opts,
					const std::shared_ptr<const ::scatdb::shape::shapeIO > s) {
					auto h = prepHandle<shape_text_handle>(sh, opts, std::string(PLUGINID));
					if (!h->out) SDBR_throw(scatdb::error::error_types::xOtherError)
						.add<std::string>("Reason", "Output stream not set. Wrong handle opening io type!");
					std::string delim = opts->getVal<std::string>("delimiter", "\t");
					for (const auto &i : s->shapes) {
						// Write header
						*(h->out.get()) << i->hash().string() << "\nID" << delim << "x" << delim << "y" << delim <<"z"
							<< delim << "IX" << delim << "IY" << delim << "IZ\n";
						// Write points
						auto pts = i->getPoints();
						size_t numPts = i->numPoints();
						const long linesz = 200;
						const long sz = (long) numPts * linesz;
						std::unique_ptr<char[]> buf(new char[sz]);
						long srem = sz;
						const char* d = delim.c_str();
						char* cur = buf.get();
						// Using the printf-based functions, since they are faster.
						for (int i = 0; i < (int)numPts; ++i) {
							const auto &b = pts->block<1, scatdb::shape::backends::NUM_SHAPECOLS>(i, 0);
							long sl = sprintf_s(cur, srem, "%d%s%d%s%d%s%d%s%d%s%d%s%d\n",
								(int)b(0), d, (int)b(1), d, (int)b(2), d, (int)b(3), d,
								(int)b(4), d, (int)b(5), d, (int)b(6));
							srem -= sl;
							cur += sl;
						}
						*(h->out.get()) << buf.get();
					}
					
					return h;
				}
				std::shared_ptr<IOhandler> export_ddscat(std::shared_ptr<IOhandler> sh, std::shared_ptr<options> opts,
					const std::shared_ptr<const ::scatdb::shape::shapeIO > s) {
					auto h = prepHandle<shape_text_handle>(sh, opts, std::string(PLUGINID));
					if (!h->out) SDBR_throw(scatdb::error::error_types::xOtherError)
						.add<std::string>("Reason", "Output stream not set. Wrong handle opening io type!");
					using namespace scatdb::shape::backends;
					for (const auto &i : s->shapes) {
						// Write header
						*(h->out.get()) << i->getDescription() << std::endl;
						const int fw = 7;
						auto hrd = i->getHeader();
						*(h->out.get()) << std::left << std::setw(fw) << i->numPoints() << " = NAT" << std::endl
							<< std::left << std::setw(fw) << (*(hrd.get()))(0, A1) << " "
							<< std::left << std::setw(fw) << (*(hrd.get()))(1, A1) << " "
							<< std::left << std::setw(fw) << (*(hrd.get()))(2, A1) << " = target vector a1 (in TF)\n"
							<< std::left << std::setw(fw) << (*(hrd.get()))(0, A2) << " "
							<< std::left << std::setw(fw) << (*(hrd.get()))(1, A2) << " "
							<< std::left << std::setw(fw) << (*(hrd.get()))(2, A2) << " = target vector a2 (in TF)\n"
							<< std::left << std::setw(fw) << (*(hrd.get()))(0, D) << " "
							<< std::left << std::setw(fw) << (*(hrd.get()))(1, D) << " "
							<< std::left << std::setw(fw) << (*(hrd.get()))(2, D) << " = d_x/d d_y/d d_z/d (normally 1 1 1)\n"
							<< std::left << std::setw(fw) << (*(hrd.get()))(0, X0) << " "
							<< std::left << std::setw(fw) << (*(hrd.get()))(1, X0) << " "
							<< std::left << std::setw(fw) << (*(hrd.get()))(2, X0) << " = X0(1-3) = location of lattice of \"target origin\"\n"
							<< "J       JX      JY      JZ  ICOMP,X ICOMP,Y ICOMP,Z\n";

						// Write points
						auto pts = i->getPoints();
						size_t numPts = i->numPoints();
						const long linesz = 200;
						const long sz = (long)numPts * linesz;
						std::unique_ptr<char[]> buf(new char[sz]);
						long srem = sz;
						char* cur = buf.get();
						// Using the printf-based functions, since they are faster.
						for (int i = 0; i < (int)numPts; ++i) {
							const auto &b = pts->block<1, scatdb::shape::backends::NUM_SHAPECOLS>(i, 0);
							long sl = sprintf_s(cur, srem, "%*d%*d%*d%*d%*d%*d%*d\n",
								-8, (int)b(0), -5, (int)b(1), -5, (int)b(2), -5, (int)b(3),
								-5, (int)b(4), -5, (int)b(5), -5, (int)b(6));
							srem -= sl;
							cur += sl;
						}
						*(h->out.get()) << buf.get();
					}

					return h;
				}


			}
		}
	}
	namespace registry {
		template<>
		std::shared_ptr<IOhandler>
			write_file_type_multi<::scatdb::shape::shapeIO>
			(std::shared_ptr<IOhandler> sh, std::shared_ptr<options> opts,
				const std::shared_ptr<const ::scatdb::shape::shapeIO > s)
		{
			std::string exporttype = opts->exportType();
			/// \todo If no exporttype, attempt to guess from the file extension?
			if (exporttype == "raw") return plugins::builtin::shape::export_raw(sh, opts, s);
			else if (exporttype == "ddscat" || !exporttype.size()) return plugins::builtin::shape::export_ddscat(sh, opts, s);
			//else if (exporttype == "adda") return plugins::builtin::shape::export_adda(sh, opts, s);
			else SDBR_throw(scatdb::error::error_types::xUnimplementedFunction)
				.add<std::string>("Reason", "Attempting to write a shape to a file, but the format specifier is not understood.")
				.add<std::string>("Format", exporttype);
			return nullptr;
		}

		template<>
		std::shared_ptr<IOhandler>
			read_file_type_multi<::scatdb::shape::shapeIO>
			(std::shared_ptr<IOhandler> sh, std::shared_ptr<options> opts,
				std::shared_ptr<::scatdb::shape::shapeIO > s)
		{
			IOtype iotype = opts->getVal<IOtype>("iotype", IOtype::READONLY);
			auto h = prepHandle<plugins::builtin::shape::shape_text_handle>
				(sh, opts, std::string(plugins::builtin::shape::PLUGINID));
			
			s->shapes.clear();

			auto shp = ::scatdb::shape::shape::generate();
			if (!h->in) SDBR_throw(scatdb::error::error_types::xOtherError)
				.add<std::string>("Reason", "Input stream not set. Wrong handle opening io type!");
			scatdb::plugins::builtin::shape::readDDSCAT(shp, *(h->in.get()), opts);
			s->shapes.push_back(shp);
			return h;
		}

	}
}