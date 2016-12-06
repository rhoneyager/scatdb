#ifndef SDBR_MAINPP
#define SDBR_MAINPP

#include "defs.hpp"

#include <memory>
#include <string>
#include <Eigen/Dense>
#include <cstdint>

namespace H5 { class Group; }
/// This is the main namespace, under which all functions are located.
namespace scatdb {
	/// Base class used in pointer marshalling
	class DLEXPORT_SDBR scatdb_base { public: scatdb_base(); virtual ~scatdb_base(); };
	class filter;
	class filterImpl;
	
	class DLEXPORT_SDBR db : public scatdb_base {
		friend class filter;
		friend class filterImpl;
		db();
		static void readDBtext(std::shared_ptr<db>, const char* dbfile);
		static void readDBhdf5(std::shared_ptr<db>, const char* dbfile, const char* hdfinternalpath = 0);
		static void readDBscatdb(std::shared_ptr<db>, const char* dbfile = nullptr);
	public:
		virtual ~db();
		static std::shared_ptr<const db> loadDB(const char* dbfile = 0, const char* hdfinternalpath = 0);
		static bool findDB(std::string& out);
		void print(std::ostream &out) const;
		void writeTextFile(const char* filename) const;
		void writeHDFfile(const char* filename,
			SDBR_write_type, const char* hdfinternalpath = nullptr) const;
		void writeHDFfile(std::shared_ptr<H5::Group>) const;

		// The data in the database, in tabular form
		struct DLEXPORT_SDBR data_entries {
#include "data.h"
			template<class T> static const char* stringify(uint64_t enumval);
			static const char* stringifyStats(uint64_t enumval);
			static const char* getCategoryDescription(uint64_t catid);
		};
		typedef Eigen::Matrix<float, Eigen::Dynamic, data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS, Eigen::RowMajor> FloatMatType;
		typedef Eigen::Matrix<uint64_t, Eigen::Dynamic, data_entries::SDBR_NUM_DATA_ENTRIES_INTS> IntMatType;
		FloatMatType floatMat;
		IntMatType intMat;

		typedef Eigen::Matrix<float, data_entries::SDBR_NUM_DATA_ENTRIES_STATS,
			data_entries::SDBR_NUM_DATA_ENTRIES_FLOATS> StatsFloatType;

		struct DLEXPORT_SDBR data_stats : public scatdb_base {
			StatsFloatType floatStats;
			uint64_t count;
			std::shared_ptr<const db> srcdb;
			void print(std::ostream&) const;
			virtual ~data_stats();
			static std::shared_ptr<const data_stats> generate(const db*);
			void writeHDF5File(std::shared_ptr<H5::Group>) const;
		private:
			data_stats();
		};
		std::shared_ptr<const data_stats> getStats() const;

		/// Regression. See lowess.cpp. delta can equal xrange / 50.
		/// The initial regression routine uses input x values in the output.
		/// For convenience, we re-interpolate over the entire x-value domain,
		/// uniformly spaced in increments of 10 microns. For the lower bound,
		/// interpolation is combined with Rayleigh scattering.
		/// \note This routine should be pre-filtered by temperature,
		///   frequency and flake type.
		std::shared_ptr<const db> regress(
			db::data_entries::data_entries_floats xaxis = db::data_entries::SDBR_AEFF_UM,
			double f = 0.1,
			uint64_t nsteps = 2, double delta = 0.) const;
		//std::shared_ptr<const db> interpolate(
		//	db::data_entries::data_entries_floats xaxis = db::data_entries::SDBR_AEFF_UM
		//	) const;

		std::shared_ptr<const db> sort(
			db::data_entries::data_entries_floats xaxis = db::data_entries::SDBR_AEFF_UM
			) const;
	private:
		mutable std::shared_ptr<const data_stats> pStats;
	};
	typedef std::shared_ptr<const db> db_t;
	class DLEXPORT_SDBR filter : public scatdb_base {
	private:
		std::shared_ptr<filterImpl> p;
		filter();
	public:
		virtual ~filter();
		static std::shared_ptr<filter> generate();
		void addFilterFloat(db::data_entries::data_entries_floats param, float minval, float maxval);
		void addFilterInt(db::data_entries::data_entries_ints param, uint64_t minval, uint64_t maxval);
		void addFilterFloat(db::data_entries::data_entries_floats param, const std::string &rng);
		void addFilterInt(db::data_entries::data_entries_ints param, const std::string &rng);
		template <class T>
		void addFilter(uint64_t param, T minval, T maxval);
		template <class T>
		void addFilter(uint64_t param, const std::string &rng);

		//void addFilterMaxFlakes(int maxFlakes);
		//void addFilterStartingRow(int row);

		enum class sortDir { SDBR_ASCENDING, SDBR_DESCENDING };
		//void addSortFloat(db::data_entries::data_entries_floats param, sortDir);
		//void addSortInt(db::data_entries::data_entries_ints param, sortDir);

		std::shared_ptr<const db> apply(std::shared_ptr<const db>) const;
		std::shared_ptr<const db> apply(const db*) const;
	};
}

#endif

