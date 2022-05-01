#pragma once

#include <Windows.h>
#include "ini.h"

namespace config {

	inline int rotateKey;
	inline int ancestorKey;
	inline int transparentKey;

	inline bool fixGroupTarget;

	inline double rotateSpeed;
	inline double repeatedKeyInputTimeLimit;
	
	inline void init(LPCSTR fileName) {
		rotateKey = ini::load(fileName, "key", "rotate", 0x52, "0x52");
		ancestorKey = ini::load(fileName, "key", "ancestor", 0x41, "0x41");
		transparentKey = ini::load(fileName, "key", "transparent", 0x54, "0x54");

		fixGroupTarget = ini::load(fileName, "bugFix", "fixGroupTarget", true, "on");

		rotateSpeed = ini::load(fileName, "other", "rotateSpeed", 0.01);
		repeatedKeyInputTimeLimit = ini::load(fileName, "other", "repeatedKeyInputTimeLimit", 0.3);
	}

}
