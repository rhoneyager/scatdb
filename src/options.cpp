#include <boost/lexical_cast.hpp>
#include <map>
#include <complex>
#include "../scatdb/defs.hpp"
#include "../private/options.hpp"
#include "../scatdb/error.hpp"

namespace scatdb {
	namespace registry {
		class options_inner {
		public:
			std::map<std::string, std::string> _mapStr;
		};
		options::options() {
			p = std::shared_ptr<options_inner>(new options_inner);
		}
		std::shared_ptr<options> options::generate() {
			auto res = std::shared_ptr<options>(new options); return res;
		}
		options::~options() {}
		void options::enumVals(std::ostream &out) const {
			//out << "\tName\t\tValue" << std::endl;
			for (const auto &v : p->_mapStr)
			{
				if (v.first != "password")
				{
					out << "\t" << v.first << ":\t" << v.second << std::endl;
				}
				else {
					out << "\t" << v.first << ":\t" << "********" << std::endl;
				}
			}
		}
		bool options::hasVal(const std::string &key) const {
			if (p->_mapStr.count(key)) return true;
			return false;
		}
		template <class T> T options::getVal(const std::string &key) const
		{
			if (!hasVal(key)) return boost::lexical_cast<T>(std::string(""));
			//RDthrow(Ryan_Debug::error::xMissingKey())
			//	<< Ryan_Debug::error::key(key);
			std::string valS = p->_mapStr.at(key);
			T res = boost::lexical_cast<T>(valS);
			return res;
		}
		template <class T> T options::getVal(const std::string &key, const T& defaultval) const
		{
			if (!hasVal(key)) return defaultval;
			return getVal<T>(key);
		}
		template <class T> options_ptr options::setVal(const std::string &key, const T &value)
		{
			std::string valS = boost::lexical_cast<std::string>(value);
			p->_mapStr[key] = valS;
			return this->shared_from_this();
		}
		template <class T> options_ptr options::add(const std::string &key, const T &value)
		{
			if (hasVal(key)) SDBR_throw(::scatdb::error::error_types::xKeyExists)
				.add<std::string>("key", key)
				.add<T>("newValue", value);
			return this->setVal<T>(key, value);
		}

#define DOTYPES(f) f(int); f(float); f(double); f(long); f(long long); \
	f(unsigned int); f(unsigned long); f(unsigned long long); f(std::string); f(bool); f(std::complex<double>);

#define IMPL_OPTS_SETVAL(T) template HIDDEN_SDBR options_ptr options::setVal<T>(const std::string&, const T&);
#define IMPL_OPTS_ADD(T) template HIDDEN_SDBR options_ptr options::add<T>(const std::string&, const T&);
#define IMPL_OPTS_GETVAL_A(T) template T HIDDEN_SDBR options::getVal<T>(const std::string&) const;
#define IMPL_OPTS_GETVAL_B(T) template T HIDDEN_SDBR options::getVal<T>(const std::string&, const T&) const;
#define IMPL_OPTS(T) IMPL_OPTS_SETVAL(T); IMPL_OPTS_GETVAL_A(T);IMPL_OPTS_GETVAL_B(T);IMPL_OPTS_ADD(T);
		DOTYPES(IMPL_OPTS);

	}

}
