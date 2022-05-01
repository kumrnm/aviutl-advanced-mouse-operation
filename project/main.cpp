#include <Windows.h>
#include "shlwapi.h"
#pragma comment(lib, "shlwapi.lib")
#include <aviutl.hpp>
#include "config.h"
#include "util.h"
#include "memref.h"
#include "rotate.h"
#include "mouseTarget.h"
#include "repeatedKeyInput.h"

#define PLUGIN_NAME "マウスオペレーション改"
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

#endif


// フック

decltype(AviUtl::FilterPlugin::func_WndProc) exedit_WndProc_original;
BOOL exedit_WndProc_hooked(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp) {
	using Message = AviUtl::detail::FilterPluginWindowMessage;

#ifdef _DEBUG
	// F5キーでデバッグ動作
	if ((message == WM_KEYUP || message == Message::MainKeyUp) && wparam == VK_F5) {
		debug();
		return FALSE;
	}
#endif

	if (message == WM_KEYDOWN || message == Message::MainKeyDown) {
		repeatedKeyInput::onKeyDown(wparam);
	}
	else if (message == WM_KEYUP || message == Message::MainKeyUp) {
		repeatedKeyInput::onKeyUp(wparam);
	}

	rotate::onExEditWndProc(message);

	const auto res = exedit_WndProc_original(hwnd, message, wparam, lparam, editp, fp);

	rotate::onExEditWndProcEnd(message);

	return res;
}

decltype(AviUtl::FilterPlugin::func_proc) exedit_proc_original;
BOOL exedit_proc_hooked(AviUtl::FilterPlugin* fp, AviUtl::FilterProcInfo* fpip) {
	mouseTarget::onExEditProc();
	return exedit_proc_original(fp, fpip);
}

int (*onPreviewWindowClick_original)(int x, int y, int param3, int param4);
int onPreviewWindowClick_hooked(int x, int y, int param3, int param4) {
	rotate::onPreviewWindowClick(x, y);
	mouseTarget::onPreviewWindowClick(x, y);

	const auto res = onPreviewWindowClick_original(x, y, param3, param4);

	mouseTarget::onPreviewWindowClickEnd();

	return res;
}

int (*onPreviewWindowDoubleClick_original)(int x, int y);
int onPreviewWindowDoubleClick_hooked(int x, int y, int param3, int param4) {
	mouseTarget::onPreviewWindowDoubleClick(x, y);

	const auto res = onPreviewWindowDoubleClick_original(x, y);

	mouseTarget::onPreviewWindowDoubleClickEnd();

	return res;
}


// 初期化

BOOL func_init(AviUtl::FilterPlugin* fp) {
	const auto exedit = util::getExEdit(fp);
	if (exedit == nullptr) {
		show_error("拡張編集が見つかりませんでした。");
	}
	else if (util::getExEditVersion(exedit) != util::ExEditVersion::version092) {
		show_error("このバージョンの拡張編集には対応していません。");
	}
	else {
		// 設定の読み込み
		CHAR fileName[MAX_PATH] = {};
		GetModuleFileName(fp->dll_hinst, fileName, MAX_PATH);
		PathRenameExtension(fileName, ".ini");
		config::init(fileName);

		memref::init(exedit);
		rotate::init(exedit);
		mouseTarget::init();

		// 拡張編集のWndProcをフック
		exedit_WndProc_original = exedit->func_WndProc;
		exedit->func_WndProc = exedit_WndProc_hooked;

		// 拡張編集のprocをフック
		exedit_proc_original = exedit->func_proc;
		exedit->func_proc = exedit_proc_hooked;

		// 再生ウィンドウクリック処理関数（0x4b6e0）の呼び出し箇所をフック
		onPreviewWindowClick_original = reinterpret_cast<decltype(onPreviewWindowClick_original)>(util::rewriteCallTarget(memref::exedit_base + 0x3ef84, onPreviewWindowClick_hooked));

		// 再生ウィンドウダブルクリック処理関数（0x4b420）の呼び出し箇所をフック
		onPreviewWindowDoubleClick_original = reinterpret_cast<decltype(onPreviewWindowDoubleClick_original)>(util::rewriteCallTarget(memref::exedit_base + 0x3eee3, onPreviewWindowDoubleClick_hooked));
	}

	return TRUE;
}


// エクスポート

extern "C" __declspec(dllexport) AviUtl::FilterPluginDLL * __stdcall GetFilterTable() {
	using Flag = AviUtl::detail::FilterPluginFlag;
	static AviUtl::FilterPluginDLL filter_info = {
		.flag = Flag::AlwaysActive | Flag::ExInformation | Flag::NoConfig,
		.name = PLUGIN_NAME,
		.func_init = func_init,
		.information = PLUGIN_NAME " " PLUGIN_VERSION PLUGIN_VERSION_SUFFIX " by " PLUGIN_DEVELOPER,
	};
	return &filter_info;
}
