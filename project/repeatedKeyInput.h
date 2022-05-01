#pragma once

namespace repeatedKeyInput {

	// キーイベント発生時に外部から呼び出す
	void onKeyDown(int key);
	void onKeyUp(int key);
	
	// キーの長押し前の連打回数を取得する
	int getLevel(int key);

}
