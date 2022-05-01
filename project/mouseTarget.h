#pragma once

#include <aviutl.hpp>

namespace mouseTarget {
	void init();
	void onExEditProc();
	void onPreviewWindowClick(int x, int y);
	void onPreviewWindowClickEnd();
	void onPreviewWindowDoubleClick(int x, int y);
	void onPreviewWindowDoubleClickEnd();
}
