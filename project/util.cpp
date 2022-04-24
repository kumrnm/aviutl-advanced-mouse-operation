#include "util.h"
#include <aviutl.hpp>

namespace util {

	AviUtl::FilterPlugin* get_exedit(AviUtl::FilterPlugin* fp) {
		for (int i = 0; ; i++) {
			const auto filter = fp->exfunc->get_filterp(i);
			if (filter == nullptr) break;
			if (strcmp(filter->name, "拡張編集") == 0) return filter;
		}
		return nullptr;
	}

	ExEditVersion get_exedit_version(AviUtl::FilterPlugin* exedit) {
		if (strcmp(exedit->information, "拡張編集(exedit) version 0.92 by ＫＥＮくん") == 0) {
			return ExEditVersion::version092;
		} else if (strcmp(exedit->information, "拡張編集(exedit) version 0.93rc1 by ＫＥＮくん") == 0) {
			return ExEditVersion::version093rc1;
		}
		else {
			return ExEditVersion::unknown;
		}
	}

}
