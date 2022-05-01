#include "util.h"
#include <imagehlp.h>
#pragma comment(lib, "imagehlp.lib")
#include <Windows.h>
#include <aviutl.hpp>
#include <exedit.hpp>
#include "memref.h"

namespace util {

	AviUtl::FilterPlugin* getExEdit(AviUtl::FilterPlugin* fp) {
		for (int i = 0; ; i++) {
			const auto filter = fp->exfunc->get_filterp(i);
			if (filter == nullptr) break;
			if (strcmp(filter->name, "拡張編集") == 0) return filter;
		}
		return nullptr;
	}

	ExEditVersion getExEditVersion(AviUtl::FilterPlugin* exedit) {
		if (strcmp(exedit->information, "拡張編集(exedit) version 0.92 by ＫＥＮくん") == 0) {
			return ExEditVersion::version092;
		} else if (strcmp(exedit->information, "拡張編集(exedit) version 0.93rc1 by ＫＥＮくん") == 0) {
			return ExEditVersion::version093rc1;
		}
		else {
			return ExEditVersion::unknown;
		}
	}

	int getObjectIndexFromPointer(const ExEdit::Object* ptr) {
		return (reinterpret_cast<unsigned int>(ptr) - reinterpret_cast<unsigned int>(*memref::get<ExEdit::Variables::objectTable>())) / sizeof(ExEdit::Object);
	}

	bool isGroupControl(const ExEdit::Object* obj) {
		return static_cast<int>(obj->flag & ExEdit::Object::Flag::Effect) != 0
			&& static_cast<int>(obj->flag & ExEdit::Object::Flag::Control) != 0
			&& static_cast<int>(obj->flag & ExEdit::Object::Flag::ShowRange) != 0;
	}

	// CALL命令（e8）の呼び出し先を書き換え、書き換え前のアドレスを返す。
	DWORD rewriteCallTarget(const DWORD callOperationAddress, const void* targetFunction) {
		const DWORD oldFunction = *(DWORD*)(callOperationAddress + 1) + (callOperationAddress + 5);
		DWORD oldProtect;
		VirtualProtect((LPVOID)callOperationAddress, 5, PAGE_READWRITE, &oldProtect);
		*(DWORD*)(callOperationAddress + 1) = (DWORD)targetFunction - (callOperationAddress + 5);
		VirtualProtect((LPVOID)callOperationAddress, 5, oldProtect, &oldProtect);
		return oldFunction;
	}

	// DLLによって読み込まれる関数をフックする
	// copied from yulib created by Auls (https://auls.client.jp/)
	void* apiHook(LPCSTR modname, LPCSTR funcname, void* funcptr) {
		// ベースアドレス
		DWORD dwBase = (DWORD)GetModuleHandle(modname);
		if (!dwBase) return NULL;

		// イメージ列挙
		ULONG ulSize;
		PIMAGE_IMPORT_DESCRIPTOR pImgDesc = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData((HMODULE)(intptr_t)dwBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ulSize);
		for (; pImgDesc->Name; pImgDesc++) {
			//const char* szModuleName = (char*)(intptr_t)(dwBase+pImgDesc->Name);
			// THUNK情報
			PIMAGE_THUNK_DATA pFirstThunk = (PIMAGE_THUNK_DATA)(intptr_t)(dwBase + pImgDesc->FirstThunk);
			PIMAGE_THUNK_DATA pOrgFirstThunk = (PIMAGE_THUNK_DATA)(intptr_t)(dwBase + pImgDesc->OriginalFirstThunk);
			// 関数列挙
			for (; pFirstThunk->u1.Function; pFirstThunk++, pOrgFirstThunk++) {
				if (IMAGE_SNAP_BY_ORDINAL(pOrgFirstThunk->u1.Ordinal))continue;
				PIMAGE_IMPORT_BY_NAME pImportName = (PIMAGE_IMPORT_BY_NAME)(intptr_t)(dwBase + (DWORD)pOrgFirstThunk->u1.AddressOfData);

				// 書き換え判定
				if (_stricmp((const char*)pImportName->Name, funcname) != 0)continue;

				// 保護状態変更
				DWORD dwOldProtect;
				if (!VirtualProtect(&pFirstThunk->u1.Function, sizeof(pFirstThunk->u1.Function), PAGE_READWRITE, &dwOldProtect))
					return NULL; // エラー

				// 書き換え
				void* pOrgFunc = (void*)(intptr_t)pFirstThunk->u1.Function; // 元のアドレスを保存しておく
				WriteProcessMemory(GetCurrentProcess(), &pFirstThunk->u1.Function, &funcptr, sizeof(pFirstThunk->u1.Function), NULL);
				pFirstThunk->u1.Function = (DWORD)(intptr_t)funcptr;

				// 保護状態戻し
				VirtualProtect(&pFirstThunk->u1.Function, sizeof(pFirstThunk->u1.Function), dwOldProtect, &dwOldProtect);
				return pOrgFunc; // 元のアドレスを返す
			}
		}
		return NULL;
	}

}
