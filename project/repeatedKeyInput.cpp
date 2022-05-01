#include <Windows.h>
#include "config.h"

namespace repeatedKeyInput {

	constexpr int maxKeyCode = 256;

	struct KeyState {
		bool pressed = false;
		double previousPressedTime = -1e100;
		int level = 1;
	} state[maxKeyCode];
	
	// システム時刻を基準時刻からの経過秒数として取得する
	double getSystemTime() {
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		return static_cast<double>((static_cast<UINT64>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime) / 10000000;
	}

	void onKeyDown(int key) {
		if (0 <= key && key < maxKeyCode) {
			if (!state[key].pressed) {
				state[key].pressed = true;
				double currentTime = getSystemTime();
				if (currentTime - state[key].previousPressedTime <= config::repeatedKeyInputTimeLimit) {
					state[key].level += 1;
				}
				else {
					state[key].level = 1;
				}
				state[key].previousPressedTime = currentTime;
			}
		}
	}

	void onKeyUp(int key) {
		if (0 <= key && key < maxKeyCode) {
			state[key].pressed = false;
		}
	}

	int getLevel(int key) {
		if (0 <= key && key < maxKeyCode) {
			if (GetKeyState(key) < 0) {
				return state[key].level;
			}
		}
		return 0;
	}

}
