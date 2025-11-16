#include "include/infrastructure/TxtProductRepository.h"
#include "include/core/Product.h"
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
    std::ranges::transform(s, s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

static inline double parse_price(std::string s) {
    trim(s);
    std::ranges::replace(s, ',', '.');
    std::erase_if(s, [](unsigned char c){ return std::isspace(c); });
    std::stringstream ss(s);
    ss.imbue(std::locale::classic());
    double v = 0.0;
    ss >> v;
    v = std::round(v * 100.0) / 100.0;
    return v;
}

static inline int parse_int(std::string s) {
    trim(s);
    std::erase_if(s, [](unsigned char c){ return std::isspace(c); });
    if (s.empty()) return 0;
    return std::stoi(s);
}

std::map<std::string, Product, std::less<>> TxtProductRepository::load() {
    std::map<std::string, Product, std::less<>> result;
    std::ifstream in(file_);
    if (!in) {
        return result;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::string name;
        std::string priceStr;
        std::string stockStr;
        std::stringstream ss(line);
        std::getline(ss, name, ';');
        std::getline(ss, priceStr, ';');
        std::getline(ss, stockStr, ';');
        trim(name);
        if (name.empty()) continue;
        std::string key = toLower(name);
        double price = parse_price(priceStr);
        int stock = stockStr.empty() ? 0 : parse_int(stockStr);
        result[key] = Product(name, price, stock);
    }
    return result;
}

void TxtProductRepository::save(const std::map<std::string, Product, std::less<>>& data) {
    std::ofstream out(file_);
    if (!out) throw IoException("cannot open products file for write: " + file_);
    out.setf(std::ios::fixed);
    out << std::setprecision(2);
    for (const auto& [key, p] : data) {
        (void)key; // unused
        out << p.name << ";" << p.price << ";" << p.stock << "\n";
    }
}
