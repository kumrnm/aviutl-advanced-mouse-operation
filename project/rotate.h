#pragma once

#include <aviutl.hpp>

namespace rotate {
	void init(AviUtl::FilterPlugin* exedit);
	void onPreviewWindowClick(int x, int y);
	void onExEditWndProc(UINT message);
	void onExEditWndProcEnd(UINT message);
}
