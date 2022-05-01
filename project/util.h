#pragma once

#include <Windows.h>
#include <aviutl.hpp>
#include <exedit.hpp>

namespace util {

	enum class ExEditVersion {
		unknown,
		version092,
		version093rc1,
	};
	AviUtl::FilterPlugin* getExEdit(AviUtl::FilterPlugin* fp);
	ExEditVersion getExEditVersion(AviUtl::FilterPlugin* exedit);

	int getObjectIndexFromPointer(const ExEdit::Object* ptr);

	bool isGroupControl(const ExEdit::Object* obj);

	// CALL命令（e8）の呼び出し先を書き換え、書き換え前のアドレスを返す。
	DWORD rewriteCallTarget(const DWORD callOperationAddress, const void* targetFunction);

	// DLLによって読み込まれる関数をフックする
	void* apiHook(LPCSTR modname, LPCSTR funcname, void* funcptr);

}
