#pragma once
#include "../../scatdb/defs.hpp"
#include <memory>
#include <vector>
#include <Eigen/Dense>
#include <vector>
namespace H5 { class Group; }
namespace scatdb {
	namespace profiles {
		namespace defs {
			enum tbl_columns {
				BIN_LOWER,
				BIN_MID,
				BIN_UPPER,
				BID_WIDTH,
				CONCENTRATION,
				NUM_COLUMNS
			};
			enum class particle_types {
				AGG_IRREG,
				AGG_NEEDLE,
				AGG_COMPACT
			};
			const char* stringify(particle_types);
			particle_types enumify(const std::string &);
		}
		typedef Eigen::Array<float, Eigen::Dynamic, defs::NUM_COLUMNS> tbl_t;
		typedef std::shared_ptr<const tbl_t> tbl_p;
		class forward_conc_table;
		typedef std::shared_ptr<const forward_conc_table> forward_p;
		typedef std::shared_ptr<const std::vector<forward_p> > forward_set_p;
		class forward_conc_table {
			forward_conc_table();
			tbl_p data;
			float tempC;
			defs::particle_types pt;
		public:
			~forward_conc_table();
			static forward_set_p import(const char* filename);
			static forward_p readText(const char* filename);
			static forward_set_p readHDF5File(const char* filename);
			tbl_p getData() const;
			float getTempC() const;
			defs::particle_types getParticleTypes() const;
			void writeText(const char* filename) const;
			void writeHDF5File(const char* filename, const char* intpath) const;
			void writeHDF5File(std::shared_ptr<H5::Group>, const char* hdfinternalpath) const;
		};
	}
}