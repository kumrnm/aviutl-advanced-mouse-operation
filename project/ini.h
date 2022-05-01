#pragma once

#include <Windows.h>
#include <algorithm>
#include <string>
#include <sstream>

namespace ini {

	template <typename T>
	inline T fromString(const std::string& str);

	template <>
	inline int fromString<int>(const std::string& str) {
		return std::stoi(str, nullptr, 0);
	}

	template <>
	inline double fromString<double>(const std::string& str) {
		return std::stod(str, nullptr);
	}

	template <>
	inline bool fromString<bool>(const std::string& str) {
		std::string s(str);
		std::transform(s.begin(), s.end(), s.begin(), std::tolower);
		if (s == "" || s == "false" || s == "off") {
			return false;
		}
		else if (s == "true" || s == "on") {
			return true;
		}
		else {
			return static_cast<bool>(fromString<double>(s));
		}
	}

	// INIファイルを読み込む。失敗した場合はデフォルト値を書き込む。
	// ユーザー定義型 T に対しては ini::fromString<T> が使用されます。
	template <typename T>
	inline T load(LPCSTR fileName, LPCSTR sectionName, LPCSTR keyName, const T& defaultValue = T(), LPCSTR defaultValueText = nullptr) {
		constexpr int strSize = 1024;
		try {
			CHAR str[strSize] = {};
			const auto len = GetPrivateProfileString(sectionName, keyName, "", str, strSize, fileName);
			if (len == 0) throw std::runtime_error("specified key does not exist");
			return fromString<T>(std::string(str, std::strlen(str)));
		}
		catch (...) {
			if (defaultValueText != nullptr) {
				WritePrivateProfileString(sectionName, keyName, defaultValueText, fileName);
			}
			else {
				std::stringstream ss;
				ss << defaultValue;
				WritePrivateProfileString(sectionName, keyName, ss.str().c_str(), fileName);
			}
			return defaultValue;
		}
	}

}
