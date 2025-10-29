#include "include/infrastructure/TxtProductRepository.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <locale>
#include <iomanip>
#include <cmath>
#include "include/Errors/CustomExceptions.h"

static inline void trim(std::string& s) {
    while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && std::isspace((unsigned char)s.back()))  s.pop_back();
}

static inline std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

static inline double parse_price(std::string s) {
    trim(s);
    std::replace(s.begin(), s.end(), ',', '.');
    s.erase(std::remove_if(s.begin(), s.end(),
                           [](unsigned char c){ return std::isspace(c); }),
            s.end());
    std::stringstream ss(s);
    ss.imbue(std::locale::classic());
    double v = 0.0;
    ss >> v;
    v = std::round(v * 100.0) / 100.0;
    return v;
}

std::map<std::string, double> TxtProductRepository::load() {
    std::map<std::string, double> result;
    std::ifstream in(file_);
    if (!in) throw IoException("cannot open products file: " + file_);

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::string name, priceStr;
        std::stringstream ss(line);
        std::getline(ss, name, ';');
        std::getline(ss, priceStr, ';');
        trim(name);
        if (name.empty()) continue;
        name = toLower(name);
        result[name] = parse_price(priceStr);
    }
    return result;
}

void TxtProductRepository::save(const std::map<std::string, double>& data) {
    std::ofstream out(file_);
    if (!out) throw IoException("cannot open products file for write: " + file_);
    out.setf(std::ios::fixed);
    out << std::setprecision(2);
    for (auto& kv : data)
        out << kv.first << ";" << kv.second << "\n";
}
