#pragma once
#include "../Ryan_Scat/defs.hpp"
#include "../Ryan_Scat/shapeForwards.hpp"
namespace Ryan_Scat {
	namespace plugins {
		namespace builtin {
			namespace shape {
				INTERNAL_TAG void register_shape_text();
				INTERNAL_TAG void readDDSCAT(std::shared_ptr<::Ryan_Scat::shape::shape> s,
					std::istream &in, std::shared_ptr<registry::options> opts);
				INTERNAL_TAG void readHeader(const char* in, std::string &desc, size_t &np,
					std::shared_ptr<::Ryan_Scat::shape::shapeHeaderStorage_t> hdr,
					size_t &headerEnd);
				INTERNAL_TAG void readTextContents(const char *iin, size_t headerEnd,
					std::shared_ptr<::Ryan_Scat::shape::shapeStorage_t> data);
			}
		}
	}
}