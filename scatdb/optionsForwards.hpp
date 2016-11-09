#pragma once
#include "defs.hpp"
#include <memory>

namespace scatdb {
	namespace registry {
		class options_inner;
		class options;
		typedef std::shared_ptr<options> options_ptr;
		typedef std::shared_ptr<const options> const_options_ptr;
		enum class IOtype
		{
			READONLY,
			READWRITE,
			EXCLUSIVE,
			TRUNCATE,
			DEBUG,
			CREATE
		};
	}
}
