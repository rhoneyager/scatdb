#include "../scatdb/defs.hpp"
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <cerrno>
#include <sstream>
#include "../scatdb/error.hpp"
#include "../scatdb/scatdb.hpp"

namespace scatdb {
	
	void db::print(std::ostream &out) const {
		out << "flaketype,frequencyghz,temperaturek,aeffum,max_dimension_mm,"
			"cabs,cbk,cext,csca,g,ar" << std::endl;
		int rows = (int)floatMat.rows();
		int icols = (int)intMat.cols();
		int fcols = (int)floatMat.cols();
		for (int i = 0; i < rows; ++i) {
			int col = 0;
			for (int j = 0; j<icols; ++j) {
				if (col) out << ",";
				out << intMat(i, j);
				col++;
			}
			for (int j = 0; j<fcols; ++j) {
				if (col) out << ",";
				if (isnan(floatMat(i, j))) out << "-999";
				else out << floatMat(i, j);
				col++;
			}
			out << std::endl;
		}
	}

	void db::writeTextFile(const char* filename) const {
		if (!filename)
			SDBR_throw(scatdb::error::error_types::xNullPointer)
			.add<std::string>("Reason", "The variable 'filename' was NULL.");
		std::FILE* f = nullptr;
#if defined _MSC_FULL_VER
		errno_t err;
		if (err = fopen_s(&f, filename, "w")) {
#else
		f= fopen(filename, "w");
		int err = errno;
		if (!f) {
#endif
			SDBR_throw(scatdb::error::error_types::xBadFunctionReturn)
				.add<std::string>("Reason", "fopen_s failed")
				.add<int>("err", err)
				.add<std::string>("filename", std::string(filename));
		}

		using namespace std;
		fprintf(f, "flaketype,frequencyghz,temperaturek,aeffum,max_dimension_mm,"
			"cabs,cbk,cext,csca,g,ar\n");
		int rows = (int)floatMat.rows();
		int icols = (int)intMat.cols();
		int fcols = (int)floatMat.cols();
		for (int i = 0; i < rows; ++i) {
			fprintf(f, "%lld,%f,%f,%f,%f,%e,%e,%e,%e,%e,%f\n",
				intMat(i, 0),
				floatMat(i, 0), floatMat(i, 1), floatMat(i, 2), floatMat(i, 3),
				floatMat(i, 4), floatMat(i, 5), floatMat(i, 6), floatMat(i, 7),
				floatMat(i, 8), floatMat(i, 9));
		}

		std::fclose(f);
	}

}

