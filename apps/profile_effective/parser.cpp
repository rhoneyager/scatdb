#include "parser.hpp"
#include "../../scatdb/error.hpp"

namespace scatdb {
	namespace profiles {
		forward_conc_table::forward_conc_table() {}
		forward_conc_table::~forward_conc_table() {}
		tbl_p forward_conc_table::getData() const {
			return data;
		}
		float forward_conc_table::getTempC() const {
			return tempC;
		}
		int forward_conc_table::getProfileNum() const {
			return profilenum;
		}
		defs::particle_types forward_conc_table::getParticleTypes() const {
			return pt;
		}
		namespace defs {
			const char* stringify(particle_types pt) {
				const char *names[] = {
					"Unknown",
					"Irregular Aggregates",
					"Needles and Needle Aggregates",
					"Small Compact Aggregates"
				};
				if (pt == particle_types::AGG_IRREG) return names[1];
				if (pt == particle_types::AGG_NEEDLE) return names[2];
				if (pt == particle_types::AGG_COMPACT) return names[3];
				SDBR_throw(scatdb::error::error_types::xArrayOutOfBounds);
				return names[0]; // To suppress warnings
			}
			particle_types enumify(const std::string & s) {
				if (s == "Irregular Aggregates") return particle_types::AGG_IRREG;
				if (s == "Needles and Needle Aggregates") return particle_types::AGG_NEEDLE;
				if (s == "Small Compact Aggregates") return particle_types::AGG_COMPACT;
				SDBR_throw(scatdb::error::error_types::xArrayOutOfBounds);
				return particle_types::AGG_COMPACT;
			}
		}
	}
}