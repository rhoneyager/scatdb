#include "../scatdb/export-hdf5.hpp"
#include "../private/versioning.hpp"
//#include "cmake-settings.h"

namespace {
	bool zlib = false;
}

namespace scatdb {
	namespace plugins
	{
		namespace hdf5
		{
			void useZLIB(bool val)
			{
				zlib = val;
			}
			bool useZLIB() { 
				static bool inited = false;
				if (inited) return zlib;
				auto lvi = scatdb::versioning::getLibVersionInfo();
				if (lvi->vb[versioning::versionInfo::V_HAS_GZIP]) zlib = true;
				return zlib; }

			template <class DataType>
			MatchAttributeTypeType MatchAttributeType() { throw("Unsupported type during attribute conversion in rtmath::plugins::hdf5::MatchAttributeType."); }
			template<> DLEXPORT_SDBR MatchAttributeTypeType MatchAttributeType<std::string>() { return std::shared_ptr<H5::AtomType>(new H5::StrType(0, H5T_VARIABLE)); }
			template<> DLEXPORT_SDBR MatchAttributeTypeType MatchAttributeType<const char*>() { return std::shared_ptr<H5::AtomType>(new H5::StrType(0, H5T_VARIABLE)); }
			template<> DLEXPORT_SDBR MatchAttributeTypeType MatchAttributeType<int>() { return std::shared_ptr<H5::AtomType>(new H5::IntType(H5::PredType::NATIVE_INT)); }
			template<> DLEXPORT_SDBR MatchAttributeTypeType MatchAttributeType<unsigned long long>() { return std::shared_ptr<H5::AtomType>(new H5::IntType(H5::PredType::NATIVE_ULLONG)); }
			template<> DLEXPORT_SDBR MatchAttributeTypeType MatchAttributeType<unsigned long>() { return std::shared_ptr<H5::AtomType>(new H5::IntType(H5::PredType::NATIVE_ULONG)); }
			template<> DLEXPORT_SDBR MatchAttributeTypeType MatchAttributeType<float>() { return std::shared_ptr<H5::AtomType>(new H5::IntType(H5::PredType::NATIVE_FLOAT)); }
			template<> DLEXPORT_SDBR MatchAttributeTypeType MatchAttributeType<short>() { return std::shared_ptr<H5::AtomType>(new H5::IntType(H5::PredType::NATIVE_SHORT)); }
			template<> DLEXPORT_SDBR MatchAttributeTypeType MatchAttributeType<unsigned short>() { return std::shared_ptr<H5::AtomType>(new H5::IntType(H5::PredType::NATIVE_USHORT)); }
			template<> DLEXPORT_SDBR MatchAttributeTypeType MatchAttributeType<double>() { return std::shared_ptr<H5::AtomType>(new H5::IntType(H5::PredType::NATIVE_DOUBLE)); }
			// \note bools are not recommended in HDF5. This type may be switched later on.
			//template<> MatchAttributeTypeType MatchAttributeType<bool>() { return std::shared_ptr<H5::AtomType>(new H5::IntType(H5::PredType::NATIVE_HBOOL)); }

			template<> DLEXPORT_SDBR bool isStrType<std::string>() { return true; }
			template<> DLEXPORT_SDBR bool isStrType<const char*>() { return true; }

			template <> DLEXPORT_SDBR void insertAttr<std::string>(H5::Attribute &attr, std::shared_ptr<H5::AtomType> vls_type, const std::string& value)
			{
				attr.write(*vls_type, value);
			}
			template <> DLEXPORT_SDBR void loadAttr<std::string>(H5::Attribute &attr, std::shared_ptr<H5::AtomType> vls_type, std::string& value)
			{
				attr.read(*vls_type, value);
				//attr.write(*vls_type, value);
			}
			/*
			template <> void insertAttr<char const*>(H5::Attribute &, std::shared_ptr<H5::AtomType>, char const * const &);
			template <> void insertAttr<int>(H5::Attribute &, std::shared_ptr<H5::AtomType>, const int&);
			template <> void insertAttr<unsigned __int64>(H5::Attribute &, std::shared_ptr<H5::AtomType>, const unsigned __int64&);
			template <> void insertAttr<unsigned long long>(H5::Attribute &, std::shared_ptr<H5::AtomType>, const unsigned long long&);
			template <> void insertAttr<float>(H5::Attribute &, std::shared_ptr<H5::AtomType>, const float&);
			template <> void insertAttr<double>(H5::Attribute &, std::shared_ptr<H5::AtomType>, const double&);
			*/

			std::shared_ptr<H5::Group> openOrCreateGroup(std::shared_ptr<H5::H5Location> base, const char* name)
			{
				std::shared_ptr<H5::Group> res;
				try {
					res = std::shared_ptr<H5::Group>(new H5::Group( base->openGroup( name )));
				} catch( H5::GroupIException not_found_error ) {
					res = std::shared_ptr<H5::Group>(new H5::Group( base->createGroup( name )));
				} catch( H5::FileIException not_found_error ) {
					res = std::shared_ptr<H5::Group>(new H5::Group( base->createGroup( name )));
				}
				return res;
			}

			std::shared_ptr<H5::Group> openGroup(std::shared_ptr<H5::H5Location> base, const char* name)
			{
				std::shared_ptr<H5::Group> res;
				try {
					res = std::shared_ptr<H5::Group>(new H5::Group( base->openGroup( name )));
				} catch( H5::GroupIException not_found_error ) {
					return nullptr;
				} catch( H5::FileIException not_found_error ) {
					return nullptr;
				}
				return res;
			}

			bool attrExists(std::shared_ptr<H5::H5Object> base, const char* name)
			{
				try {
					H5::Attribute(base->openAttribute(name));
					return true;
				}
				catch (H5::AttributeIException not_found_error) {
					return false;
				}
				catch (H5::FileIException not_found_error) {
					return false;
				}
				catch (H5::GroupIException not_found_error) {
					return false;
				}
			}

			bool groupExists(std::shared_ptr<H5::H5Location> base, const char* name)
			{
				try {
					H5::Group( base->openGroup( name ));
					return true;
				} catch( H5::GroupIException not_found_error ) {
					return false;
				}
				catch (H5::FileIException not_found_error) {
					return false;
				}
			}

			std::pair<bool,bool> symLinkExists(std::shared_ptr<H5::H5Location> base, const char* name)
			{
				bool linkexists = false;
				bool linkgood = false;
				std::string lloc;
				try {
					H5G_stat_t stats;
					base->getObjinfo(name, false, stats);
					linkexists = true;

					//lloc = base->getLinkval(name);
					base->getObjinfo(name, true, stats);
					linkgood = true;
				} catch( H5::GroupIException not_found_error ) {
				} catch( H5::FileIException not_found_error) {
				}
				return std::pair<bool,bool>(linkexists,linkgood);
			}

			bool datasetExists(std::shared_ptr<H5::H5Location> base, const char* name)
			{
				try {
					H5::DataSet(base->openDataSet(name));
					return true;
				}
				catch (H5::GroupIException not_found_error) {
					return false;
				}
				catch (H5::FileIException not_found_error) {
					return false;
				}
			}

			std::shared_ptr<H5::DSetCreatPropList> make_plist(size_t rows, size_t cols, bool compress)
			{
				using namespace H5;
				hsize_t chunk[2] = { (hsize_t)rows, (hsize_t)cols };
				auto plist = std::shared_ptr<DSetCreatPropList>(new DSetCreatPropList);
				plist->setChunk(2, chunk);
				if (compress && zlib)
					plist->setDeflate(6);
//#if COMPRESS_ZLIB
//				plist->setDeflate(6);
//#endif
				return plist;
			}
		}
	}
}

