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

	inline struct _Init {
		_Init() {
			static nnStreambuf dbgstream;
			std::cout.rdbuf(&dbgstream);
		}
	} _init;
}

inline std::ostream& cdbg = std::cout;
