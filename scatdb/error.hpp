#pragma once
#include "defs.hpp"
#pragma warning( disable : 4251 ) // DLL interface
#pragma warning( disable : 4275 ) // DLL interface
#include <exception>
#include <memory>
#include <string>
#include "optionsForwards.hpp"
#include "errorTypes.hpp"

namespace scatdb {
	namespace error {
		class DLEXPORT_SDBR error_options_inner;
		class xError;
		typedef std::shared_ptr<xError> xError_ptr;
		class DLEXPORT_SDBR xError : public std::exception
		{
		protected:
			std::shared_ptr<error_options_inner> ep;
		public:
			xError(error_types = error_types::xOtherError);
			virtual ~xError();
			virtual const char* what() const noexcept;

			/// Insert a context of errors.
			//void push(::scatdb::registry::const_options_ptr);
			xError& push(::scatdb::registry::options_ptr);
			xError& push();

			template <class T> xError& add(const std::string &key, const T &value);

		};

	}
}

#ifdef _MSC_FULL_VER
#define FSIG_SDBR __FUNCSIG__
#endif
#ifdef __GNUC__
#define FSIG_SDBR __PRETTY_FUNCTION__
#endif

#define RSpusherrorvars \
	.add<std::string>("source_filename", std::string(__FILE__)) \
	.add<int>("source_line", (int)__LINE__) \
	.add<std::string>("source_function", std::string(FSIG_SDBR))
#define RSmkerror(x) ::scatdb::error::xError(x).push() \
	RSpusherrorvars
/// \todo Detect if inherits from std::exception or not.
/// If inheritable, check if it is an xError. If yes, push a new context.
/// If not inheritable, push a new context with what() as the expression.
/// If not an exception, then create a new xError and push the appropriate type in a context.
#define SDBR_throw(x) throw RSmkerror(x)

