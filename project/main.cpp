#include <aviutl.hpp>
#include <exedit.hpp>
#include "util.h"

#define PLUGIN_NAME "マウス操作・改"
#define PLUGIN_VERSION "v1.0.0"
#define PLUGIN_DEVELOPER "蝙蝠の目"

#ifdef _DEBUG
#define PLUGIN_VERSION_SUFFIX " (Debug)"
#include "dbgstream.h"
#else
#define PLUGIN_VERSION_SUFFIX ""
#endif


void show_error(LPCTSTR text) {
	MessageBox(NULL, text, PLUGIN_NAME, MB_OK | MB_ICONERROR);
}


// デバッグ

#ifdef _DEBUG

void debug() {
	cdbg << std::flush;
}

decltype(AviUtl::FilterPlugin::func_WndProc) exedit_WndProc_original;
BOOL exedit_WndProc_hooked(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp) {
	// F5キーでデバッグ動作
	if ((message == WM_KEYUP || message == AviUtl::detail::FilterPluginWindowMessage::MainKeyUp) && wparam == VK_F5) {
		debug();
		return FALSE;
	}

	return exedit_WndProc_original(hwnd, message, wparam, lparam, editp, fp);
}

#endif


// 初期化

BOOL func_init(AviUtl::FilterPlugin* fp) {
	const auto exedit = util::get_exedit(fp);
	if (exedit == nullptr) {
		show_error("拡張編集が見つかりませんでした。");
	}
	else if (util::get_exedit_version(exedit) != util::ExEditVersion::version092) {
		show_error("このバージョンの拡張編集には対応していません。");
	}
	else {
#ifdef _DEBUG
		dbgstream::init();
		exedit_WndProc_original = exedit->func_WndProc;
		exedit->func_WndProc = exedit_WndProc_hooked;
#endif
	}

	return TRUE;
}


// エクスポート

extern "C" __declspec(dllexport) AviUtl::FilterPluginDLL* __stdcall GetFilterTable() {
	using Flag = AviUtl::detail::FilterPluginFlag;
	static AviUtl::FilterPluginDLL filter_info = {
		.flag = Flag::AlwaysActive | Flag::ExInformation | Flag::NoConfig,
		.name = PLUGIN_NAME,
		.func_init = func_init,
		.information = PLUGIN_NAME " " PLUGIN_VERSION PLUGIN_VERSION_SUFFIX " by " PLUGIN_DEVELOPER,
	};
	return &filter_info;
}
