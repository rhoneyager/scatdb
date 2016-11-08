#pragma once
#include "../scatdb/defs.hpp"
#pragma warning( disable : 4251 ) // DLL interface
#include <memory>
#include <string>
#include <iostream>
#include "../scatdb/optionsForwards.hpp"

namespace scatdb {
	namespace registry {
		class HIDDEN_SDBR options : public std::enable_shared_from_this<options>
		{
		protected:
			options();
			std::shared_ptr<options_inner> p;
		public:
			virtual ~options();
			static std::shared_ptr<options> generate();
			void enumVals(std::ostream &out) const;
			bool hasVal(const std::string &key) const;
			/// Retrieves an option. Throws if nonexistant.
			template <class T> T getVal(const std::string &key) const;
			/// Retrieves an option. Returns defaultval if nonexistant.
			template <class T> T getVal(const std::string &key, const T& defaultval) const;
			/// Adds or replaces an option.
			template <class T> options_ptr setVal(const std::string &key, const T &value);
			/// Adds an option. Throws if the same name already exists.
			template <class T> options_ptr add(const std::string &key, const T &value);

			// Some convenient definitions
			inline void filename(const std::string& val) { setVal<std::string>("filename", val); }
			inline std::string filename() const { return getVal<std::string>("filename", ""); }
			inline void extension(const std::string& val) { setVal<std::string>("extension", val); }
			inline std::string extension() const { return getVal<std::string>("extension", ""); }
			inline void filetype(const std::string &val) { setVal<std::string>("filetype", val); }
			inline std::string filetype() const { return getVal<std::string>("filetype", ""); }
			inline void exportType(const std::string &val) { setVal<std::string>("exportType", val); }
			inline std::string exportType() const { return getVal<std::string>("exportType", ""); }
			};
	}
}
