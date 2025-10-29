#pragma once
#include <QString>
#include <string>

inline QString qs(const std::string& s) { return QString::fromUtf8(s.c_str()); }
inline std::string ss(const QString& s) { return s.toUtf8().constData(); }
