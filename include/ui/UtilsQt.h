#pragma once
#include <QString>
#include <string>
#include <algorithm>
#include <cctype>

inline QString qs(const std::string& s) { return QString::fromUtf8(s.c_str()); }
inline std::string ss(const QString& s) { return s.toUtf8().constData(); }

inline std::string formatName(const std::string& name) {
    if (name.empty()) return name;
    std::string result = name;
    std::ranges::transform(result, result.begin(), ::tolower);
    if (!result.empty()) {
        result[0] = std::toupper(result[0]);
    }
    return result;
}
