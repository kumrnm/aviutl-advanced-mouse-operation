#include <cmath>
#include <numbers>
#include <vector>
#include <unordered_map>
#include <aviutl.hpp>
#include <exedit.hpp>
#include <Eigen/Dense>
#include "util.h"
#include "memref.h"
#include "config.h"
#ifdef _DEBUG
#include "dbgstream.h"
#endif


namespace rotate {

	inline double rad2deg(const double rad) {
		return rad * 180 / std::numbers::pi;
	}
	inline double deg2rad(const double deg) {
		return deg * std::numbers::pi / 180;
	}
	inline double normalizeAngle(const double angle) {
		constexpr auto pi2 = std::numbers::pi * 2;
		return (angle / pi2 - round(angle / pi2)) * pi2;
	}

	template <typename Rotation>
	Eigen::Vector3d eulerAngleDiffForAdditionalRotation(const Eigen::Matrix3d& baseRotation, const Rotation& additionalRotation) {
		const auto baseEuler = baseRotation.eulerAngles(0, 1, 2);
		const auto newEuler = (baseRotation * additionalRotation).eulerAngles(0, 1, 2);
		Eigen::Vector3d diff = newEuler - baseEuler;
		Eigen::Vector3d diff2 = diff + Eigen::Vector3d::Ones() * std::numbers::pi;
		for (int i = 0; i < 3; i++) diff(i) = normalizeAngle(diff(i));
		for (int i = 0; i < 3; i++) diff2(i) = normalizeAngle(diff2(i));
		return diff.norm() < diff2.norm() ? diff : diff2;
	}

	int mouseGlobalX, mouseGlobalY;
	std::unordered_map<ExEdit::ObjectFilterIndex, std::pair<int, int>> previousMousePosition;
	int targetObjectCenterGlobalX, targetObjectCenterGlobalY;


	// マウス座標からオブジェクトの中心点の座標系への変換行列を取得する
	Eigen::Matrix3d getEnvMatrix() {
		const auto mp = memref::get<ExEdit::Variables::globalDiffToLocalDiff>();
		const auto m = Eigen::Matrix<double, 3, 2>({
			{static_cast<double>(mp[0]), static_cast<double>(mp[3])},
			{static_cast<double>(mp[1]), static_cast<double>(mp[4])},
			{static_cast<double>(mp[2]), static_cast<double>(mp[5])} }) / 0x4000;
		const auto forX = m.col(0), forY = m.col(1);
		const auto forZ = forX.cross(forY); // ここの縮尺は適当（向きだけ分かればよい）
		Eigen::Matrix3d res;
		res << forX, forY, forZ;
		return res;
	}


	// マウス操作の処理に介入する

	struct MouseOperationProcArgs {
		enum class Message : int {
			All = 0x702
		};

		enum class Command : short {
			MouseDown = 0x1e1f,
			Drag = 0x1e1e,
			MouseUp = 0x1e20,
		};

		enum class Target : short {
			Both = 30000,
			Left = 30001,
			Right = 30002,
		};

		ExEdit::Exfunc* exfunc;
		Message message;
		Command command;
		Target target;
		short diffX, diffY;
		int unknown;
		ExEdit::Filter* filter;

		inline bool isOperatingLeft() {
			return target == Target::Both || target == Target::Left;
		}
		inline bool isOperatingRight() {
			return target == Target::Both || target == Target::Right;
		}
		inline double getTrackValue(const int trackBarIndex) {
			// トラックバーの現在の値を取得するのが難しいため両端の値で妥協
			return static_cast<double>((isOperatingLeft() ? filter->track_value_left : filter->track_value_right)[trackBarIndex])
				/ filter->track_scale[trackBarIndex];
		}
		template <typename T>
		inline void addTrackValue(const int trackBarIndex, const T value) {
			const auto s = filter->track_scale[trackBarIndex];
			const int v = static_cast<int>(value * filter->track_scale[trackBarIndex]);
			if (isOperatingLeft()) {
				filter->track_value_left[trackBarIndex] += v;
			}
			if (isOperatingRight()) {
				filter->track_value_right[trackBarIndex] += v;
			}
		}
	};
	BOOL(*mouseOperationProc_original)(MouseOperationProcArgs args);
	BOOL mouseOperationProc_hooked(MouseOperationProcArgs args) {

		if (args.command == MouseOperationProcArgs::Command::Drag) {
			BOOL updated = FALSE;

			// マウス座標を更新
			mouseGlobalX += args.diffX;
			mouseGlobalY += args.diffY;

			const auto diffX = mouseGlobalX - previousMousePosition[args.filter->processing].first;
			const auto diffY = mouseGlobalY - previousMousePosition[args.filter->processing].second;

			if (GetKeyState(config::rotateKey) < 0) {

				// 座標変換行列（ただし並進は考慮しない）
				const auto envMatrix = getEnvMatrix();
				const Eigen::Matrix3d selfMatrixInv =
					Eigen::Scaling(args.getTrackValue(args.filter->track_gui->zoom) / 100)
					* Eigen::AngleAxisd(deg2rad(args.getTrackValue(args.filter->track_gui->rx)), Eigen::Vector3d::UnitX())
					* Eigen::AngleAxisd(deg2rad(args.getTrackValue(args.filter->track_gui->ry)), Eigen::Vector3d::UnitY())
					* Eigen::AngleAxisd(deg2rad(args.getTrackValue(args.filter->track_gui->rz)), Eigen::Vector3d::UnitZ());
				const auto selfMatrix = selfMatrixInv.inverse();
				const auto wholeMatrix = selfMatrix * envMatrix;

				if (GetKeyState(VK_SHIFT) < 0 && GetKeyState(VK_MENU) >= 0) {
					// 中心座標変更
					const auto diff =
						-wholeMatrix
						* Eigen::Vector3d({ static_cast<double>(diffX), static_cast<double>(diffY), 0 });

					if (args.filter->track_gui->cx >= 0) {
						args.addTrackValue(args.filter->track_gui->cx, diff(0));
						updated = TRUE;
					}
					if (args.filter->track_gui->cy >= 0) {
						args.addTrackValue(args.filter->track_gui->cy, diff(1));
						updated = TRUE;
					}
					if (args.filter->track_gui->cz >= 0) {
						args.addTrackValue(args.filter->track_gui->cz, diff(2));
						updated = TRUE;
					}
				}
				else {
					Eigen::Vector3d diff = Eigen::Vector3d::Zero();
					bool locked = false;

					if (GetKeyState(VK_MENU) < 0) {
						// グローバルXY軸回転

						// XY軸回転が不可能かつオブジェクトが正面（Z軸方向）を向いている状況では操作禁止
						if (args.filter->track_gui->rx < 0
							&& args.filter->track_gui->ry < 0
							&& std::abs(envMatrix.col(2).normalized().dot(Eigen::Vector3d::UnitZ())) > 0.9) {
							locked = true;
						}

						const auto rgx = config::rotateSpeed * diffX;
						diff += eulerAngleDiffForAdditionalRotation(
							selfMatrixInv,
							Eigen::AngleAxisd(rgx, -wholeMatrix.col(1).normalized()));

						const auto rgy = config::rotateSpeed * diffY;
						diff += eulerAngleDiffForAdditionalRotation(
							selfMatrixInv,
							Eigen::AngleAxisd(rgy, wholeMatrix.col(0).normalized()));
					}
					else {
						// グローバルZ軸回転
						const auto mouseAngle = std::atan2(mouseGlobalY - targetObjectCenterGlobalY, mouseGlobalX - targetObjectCenterGlobalX);
						const auto preMouseAngle = std::atan2(mouseGlobalY - diffY - targetObjectCenterGlobalY, mouseGlobalX - diffX - targetObjectCenterGlobalX);
						auto mouseAngleDiff = normalizeAngle(mouseAngle - preMouseAngle);

						diff = eulerAngleDiffForAdditionalRotation(
							selfMatrixInv,
							Eigen::AngleAxisd(mouseAngleDiff, wholeMatrix.col(2).normalized()));
					}

					if (!locked) {
						if (args.filter->track_gui->rx >= 0) {
							args.addTrackValue(args.filter->track_gui->rx, rad2deg(diff(0)));
							updated = TRUE;
						}
						if (args.filter->track_gui->ry >= 0) {
							args.addTrackValue(args.filter->track_gui->ry, rad2deg(diff(1)));
							updated = TRUE;
						}
						if (args.filter->track_gui->rz >= 0) {
							args.addTrackValue(args.filter->track_gui->rz, rad2deg(diff(2)));
							updated = TRUE;
						}
					}
				}

				if (updated) {
					// プロパティウィンドウの更新
					args.exfunc->x00(args.filter->processing);
				}

				previousMousePosition[args.filter->processing] = { mouseGlobalX, mouseGlobalY };

				return updated;
			}

		}
		else if (args.command == MouseOperationProcArgs::Command::MouseDown) {
			previousMousePosition[args.filter->processing] = { mouseGlobalX, mouseGlobalY };
		}
		else if (args.command == MouseOperationProcArgs::Command::MouseUp) {
			previousMousePosition.erase(args.filter->processing);
		}

		// 既定の動作
		return mouseOperationProc_original(args);
	}


	// 拡張編集のウィンドウイベント処理に介入し、マウス座標のグローバル座標を取得する
	void onPreviewWindowClick(int x, int y) {
		mouseGlobalX = x;
		mouseGlobalY = y;
	}


	// 座標変換処理に介入し、クリックしたオブジェクトの中心点のグローバル座標を取得する
	struct TransformCoordinateArgs {
		ExEdit::ObjectFilterIndex* objectFilterIndex;
		ExEdit::FilterProcInfo* efpip;
		unsigned int flag;
		struct { int x, y, z; } *p;
	};
	int (*transformCoordinate_original)(TransformCoordinateArgs args);
	int transformCoordinate_hooked(TransformCoordinateArgs args) {
		const auto res = transformCoordinate_original(args);

		targetObjectCenterGlobalX = (args.p->x + 0x500) / 0x1000 + args.efpip->scene_w / 2;
		targetObjectCenterGlobalY = (args.p->y + 0x500) / 0x1000 + args.efpip->scene_h / 2;

		return res;
	}


	// 回転操作中はShift+ドラッグによる垂直・水平固定機能をキャンセルする

	bool inExEditMouseMoveProc = false;
	void onExEditWndProc(UINT message) {
		if (message == AviUtl::detail::FilterPluginWindowMessage::MainMouseMove) {
			inExEditMouseMoveProc = true;
		}
	}
	void onExEditWndProcEnd(UINT message) {
		if (message == AviUtl::detail::FilterPluginWindowMessage::MainMouseMove) {
			inExEditMouseMoveProc = false;
		}
	}
	SHORT(WINAPI* GetKeyState_original)(int key);
	SHORT WINAPI GetKeyState_hooked(int key) {
		if (key == VK_SHIFT && inExEditMouseMoveProc
			&& GetKeyState_original(config::rotateKey) < 0
			&& GetKeyState_original(VK_MENU) >= 0) {
			return 0;
		}

		return GetKeyState_original(key);
	}


	// 初期化

	void init(AviUtl::FilterPlugin* exedit) {

		// マウス操作の処理を行う内部関数（0x1b550）の呼び出し箇所を全てフック
		const std::vector<unsigned int> call_offsets_of_mouseOperationProc({
			0x2603f,
			0x5a541,
			0x6f3b6,
			0x8a080 });
		for (const auto offset : call_offsets_of_mouseOperationProc) {
			mouseOperationProc_original = reinterpret_cast<decltype(mouseOperationProc_original)>(util::rewriteCallTarget(memref::exedit_base + offset, mouseOperationProc_hooked));
		}

		// 座標変換行列更新関数（0x1ae20）内の座標変換関数をフック
		transformCoordinate_original = reinterpret_cast<decltype(transformCoordinate_original)>(util::rewriteCallTarget(memref::exedit_base + 0x1aeb4, transformCoordinate_hooked));

		// WinAPIのGetKeyStateをフック
		char exeditPath[MAX_PATH] = {};
		GetModuleFileName(exedit->dll_hinst, exeditPath, sizeof(exeditPath));
		GetKeyState_original = reinterpret_cast<decltype(GetKeyState_original)>(util::apiHook(exeditPath, "GetKeyState", GetKeyState_hooked));

	}

}
