#include <exedit.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "memref.h"
#include "util.h"
#include "config.h"
#include "repeatedKeyInput.h"
#ifdef _DEBUG
#include "dbgstream.h"
#endif

namespace mouseTarget {

	std::unordered_map<int, int> parentObjectIndex;
	bool mouseTargetsEdited = false;
	std::vector<ExEdit::MouseTarget> mouseTargetBackup;


	// マウスターゲット情報の一括取得・設定
	std::vector<ExEdit::MouseTarget> getMouseTargets() {
		std::vector<ExEdit::MouseTarget> res;
		const auto n = *memref::get<ExEdit::Variables::mouseTargets::length>();
		for (int i = 0; i < n; i++) {
			res.push_back(memref::get<ExEdit::Variables::mouseTargets>()[i]);
		}
		return res;
	}
	void setMouseTargets(const std::vector<ExEdit::MouseTarget>& mouseTargets) {
		const int n = min(mouseTargets.size(), ExEdit::MouseTarget::maxLength);
		*memref::get<ExEdit::Variables::mouseTargets::length>() = n;
		for (int i = 0; i < n; i++) {
			memref::get<ExEdit::Variables::mouseTargets>()[i] = mouseTargets[i];
		}
	}


	// マウスターゲット情報を書き換える

	bool processMouseTargets(const int mouseX, const int mouseY) {
		const auto mouseTargets = memref::get<ExEdit::Variables::mouseTargets>();
		const auto mouseTargetsLength = *memref::get<ExEdit::Variables::mouseTargets::length>();
		bool updated = false;

		// グループ制御選択時に上のレイヤーのオブジェクトを選択できない仕様（バグ？）を修正
		if (config::fixGroupTarget) {
			auto targets = getMouseTargets();

			// 全画面を覆うグループ制御のマウスターゲットを検出し、先頭に再配置する
			for (auto target = targets.begin(); target != targets.end(); target++) {
				if (target->objectIndex == *memref::get<ExEdit::Variables::objectDialog::objectIndex>()
					&& util::isGroupControl(*memref::get<ExEdit::Variables::objectTable>() + target->objectIndex)
					&& target->flag & 0x200000 /* 全画面フラグ？ */) {

					const ExEdit::MouseTarget temp = *target;
					targets.erase(target);
					targets.insert(targets.begin(), temp);
					setMouseTargets(targets);
					updated = true;
					break;

				}
			}
		}


		// 透過
		if (GetKeyState(config::transparentKey) < 0) {
			const auto level = repeatedKeyInput::getLevel(config::transparentKey);

			// 上から(level)個のターゲットを無効化する
			std::unordered_set<int> transparentObjectIndices;
			for (int i = mouseTargetsLength - 1; i >= 0; i--) {
				const auto target = mouseTargets + i;

				// 当たり判定
				if (-1 < static_cast<char>(target->flag >> 8) && (target->flag & 0x20000) == 0 && (target->flag & 3) != 0) {
					int left = target->x, right = left + target->width;
					int top = target->y, bottom = top + target->height;
					if (left <= mouseX && mouseX < right && top <= mouseY && mouseY < bottom) {
						// オブジェクトを透過対象リストに加える
						if (static_cast<int>(transparentObjectIndices.size()) < level) {
							transparentObjectIndices.insert(target->objectIndex);
						}
					}
				}

				// 透過処理
				if (transparentObjectIndices.contains(target->objectIndex)) {
					// 適当に画面外に飛ばしておく
					target->x = target->y = -1341893;
					target->width = target->height = 0;
				}
			}

			updated = true;
		}


		// 親グループ操作
		if (GetKeyState(config::ancestorKey) < 0) {
			const auto level = repeatedKeyInput::getLevel(config::ancestorKey);

			// マウスターゲットの操作対象オブジェクトを親グループに差し替える
			for (int i = 0; i < mouseTargetsLength; i++) {
				auto target = mouseTargets + i;
				for (int j = 0; j < level; j++) {
					if (parentObjectIndex.contains(target->objectIndex)) {
						target->objectIndex = parentObjectIndex[target->objectIndex];
						target->filterIndex = 0; // グループ制御は常に0番目のフィルタ（多分）
						target->target = ExEdit::MouseTarget::Target::Both;
					}
				}
			}

			updated = true;
		}


		return updated;
	}


	// 再生ウィンドウクリック時の処理

	void onPreviewWindowClick(int mouseX, int mouseY) {
		if (mouseTargetsEdited) return;

		// 加工前のマウスターゲットをバックアップ
		mouseTargetBackup = getMouseTargets();

		// マウスターゲットを加工
		if (processMouseTargets(mouseX, mouseY)) {
			mouseTargetsEdited = true;
		}
	}
	void onPreviewWindowClickEnd() {
		// クリックイベント内でマウスターゲットの再計算が行われなかった場合、加工前の状態に復元する
		if (mouseTargetsEdited) {
			setMouseTargets(mouseTargetBackup);
			mouseTargetsEdited = false;
		}
	}
	void onPreviewWindowDoubleClick(int mouseX, int mouseY) {
		onPreviewWindowClick(mouseX, mouseY);
	}
	void onPreviewWindowDoubleClickEnd() {
		onPreviewWindowClickEnd();
	}


	// 描画時にグループ制御の親子関係を記録する

	void (*fireFilterUpdateEvent_original)(int param1, int param2, unsigned int pt, int param4);
	void fireFilterUpdateEvent_hooked(int param1, int param2, unsigned int pt, int param4) {
		// 「真・グループ制御」が同じ関数をフックしてグループ制御に干渉するため、originalを先に実行する
		fireFilterUpdateEvent_original(param1, param2, pt, param4);

		// スタックから内部描画関数のローカル変数にアクセス
		const auto layerObject = reinterpret_cast<ExEdit::Object**>(pt + 0x4d0);
		const auto layerParent = reinterpret_cast<ExEdit::Object**>(pt + 0x1b0);

		// 各オブジェクトの親を記録する
		for (int i = 0; i < 100; i++) {
			if (layerObject[i] != nullptr) {
				const auto index = util::getObjectIndexFromPointer(layerParent[i]);
				if (layerParent[i] != nullptr) {
					parentObjectIndex[util::getObjectIndexFromPointer(layerObject[i])] = index;
				}
			}
		}
	}

	void onExEditProc() {
		parentObjectIndex.clear();

		// exedit->procではマウスターゲットの再計算が行われる
		mouseTargetsEdited = false;
	}


	// 初期化

	void init() {
		fireFilterUpdateEvent_original = reinterpret_cast<decltype(fireFilterUpdateEvent_original)>(
			util::rewriteCallTarget(memref::exedit_base + 0x48e99, fireFilterUpdateEvent_hooked));
	}

}
