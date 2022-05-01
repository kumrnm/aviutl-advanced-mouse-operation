#pragma once

#include <aviutl.hpp>
#include <exedit.hpp>

namespace ExEdit {
	struct MouseTarget {
		static inline constexpr int maxLength = 256;

		enum class Target : int {
			Both = 30000,
			Left = 30001,
			Right = 30002,
		};

		int x, y, width, height;
		int unknown[2];
		unsigned int flag;
		int objectIndex;
		int filterIndex;
		Target target;
		int unknown2[3];
	};

	namespace Variables {
		using globalDiffToLocalDiff = Variable<int, true, 0x11efb0, NULL>;
		struct mouseTargets : Variable<MouseTarget, true, 0x146258, NULL> {
			using length = Variable<int, false, 0x17921c, NULL>;
		};
	}
}


namespace memref {
	inline unsigned int exedit_base;

	inline void init(AviUtl::FilterPlugin* exedit) {
		exedit_base = reinterpret_cast<decltype(exedit_base)>(exedit->dll_hinst);
	}

	template<typename Variable>
	inline Variable::reference_type get() {
		return ExEdit::Variables::get092<Variable>(exedit_base);
	}
}
