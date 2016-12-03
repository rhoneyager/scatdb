#pragma once
#include "../scatdb/defs.hpp"
#include "../scatdb/shape/shapeForwards.hpp"
namespace scatdb {
	namespace plugins {
		namespace builtin {
			namespace shape {
				HIDDEN_SDBR void register_shape_text();
				HIDDEN_SDBR std::shared_ptr<::scatdb::shape::shape> readDDSCAT(const char* in);
				HIDDEN_SDBR std::shared_ptr<::scatdb::shape::shape> readTextRaw(const char *iin);
				HIDDEN_SDBR void readHeader(const char* in, std::string &desc, size_t &np,
					std::shared_ptr<::scatdb::shape::shapeHeaderStorage_t> hdr,
					size_t &headerEnd);
				HIDDEN_SDBR void readTextContents(const char *iin, size_t headerEnd,
					std::shared_ptr<::scatdb::shape::shapeStorage_t> data);
				HIDDEN_SDBR std::shared_ptr<::scatdb::shape::shape> readTextFile(
					const std::string &filename);
				HIDDEN_SDBR void writeDDSCAT(const std::string &filename, ::scatdb::shape::shape_ptr);
				HIDDEN_SDBR void writeTextRaw(const std::string &filename, ::scatdb::shape::shape_ptr);
			}
		}
		namespace hdf5 {
			HIDDEN_SDBR void readShapesHDF5(const std::string &filename,
				std::vector<std::shared_ptr<const ::scatdb::shape::shape> > &shps);
			HIDDEN_SDBR void writeShapesHDF5(const std::string &filename,
				const std::vector<std::shared_ptr<const ::scatdb::shape::shape> > &shps);
		}
	}
}
