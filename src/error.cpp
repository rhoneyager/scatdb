#include "../scatdb/error.hpp"
#include "../private/options.hpp"
#include <complex>
#include <list>
#include <sstream>
namespace scatdb {
	namespace error {
		class DLEXPORT_SDBR error_options_inner {
		public:
			std::list<::scatdb::registry::const_options_ptr> stk;
			::scatdb::registry::options_ptr cur;
			error::error_types et;
			std::string emessage;
		};
		xError::xError(error::error_types xe) {
			ep = std::shared_ptr<error_options_inner>(new error_options_inner);
			ep->et = xe;
		}
		xError::~xError() {}
		const char* xError::what() const noexcept {
			//static const char* msg = "An unknown error has occurred.";
			//return msg;
			std::ostringstream o;
			o << "Error: " << stringify(ep->et) << std::endl;
			// Pull from stack
			int i = 1;
			for (const auto &e : ep->stk) {
				o << "Throw frame " << i << std::endl;
				e->enumVals(o);
				++i;
			}
			ep->emessage = o.str();
			return ep->emessage.c_str();
		}
		xError& xError::push(::scatdb::registry::options_ptr op)
		{
			ep->stk.push_back(op);
			ep->cur = op;
			return *this;
		}
		xError& xError::push()
		{
			::scatdb::registry::options_ptr op = ::scatdb::registry::options::generate();
			ep->stk.push_back(op);
			ep->cur = op;
			return *this;
		}

		template <class T> xError& xError::add(const std::string &key, const T &value)
		{
			if (!ep->cur) push();
			this->ep->cur->add<T>(key, value);
			return *this;
		}

#define DOTYPES(f) f(int); f(float); f(double); f(long); f(long long); \
	f(unsigned int); f(unsigned long); f(unsigned long long); f(std::string); f(bool); f(std::complex<double>);

#define IMPL_XERROR_ADD(T) template DLEXPORT_SDBR xError& xError::add<T>(const std::string&, const T&);
		DOTYPES(IMPL_XERROR_ADD);

	}
}
