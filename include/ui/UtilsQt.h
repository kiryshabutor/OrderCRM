#pragma once
#include <QString>
#include <string>
#include <algorithm>
#include <cctype>

inline QString qs(const std::string& s) { return QString::fromUtf8(s.c_str()); }
inline std::string ss(const QString& s) { return s.toUtf8().constData(); }

// Форматирует строку: первая буква первого слова заглавная, остальные в нижнем регистре
inline std::string formatName(const std::string& name) {
    if (name.empty()) return name;
    std::string result = name;
    // Преобразуем все в нижний регистр
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    // Первая буква первого слова - заглавная
    if (!result.empty()) {
        result[0] = std::toupper(result[0]);
    }
    return result;
}
