#pragma once

#include <aviutl.hpp>

namespace util {
	enum class ExEditVersion {
		unknown,
		version092,
		version093rc1,
	};

	AviUtl::FilterPlugin* get_exedit(AviUtl::FilterPlugin* fp);
	ExEditVersion get_exedit_version(AviUtl::FilterPlugin* exedit);
}
