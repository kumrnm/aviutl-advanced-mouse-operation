#pragma once

#include <iostream>
#include <Windows.h>

namespace dbgstream {
	struct nnStreambuf : public std::streambuf {
		virtual int_type overflow(int_type c = EOF) {
			if (c != EOF) {
				char buf[] = { static_cast<char>(c), '\0' };
				OutputDebugString(buf);
			}
			return c;
		}
	};

	inline void init() {
		static nnStreambuf dbgstream;
		std::cout.rdbuf(&dbgstream);
	}
}

#define cdbg (std::cout)
