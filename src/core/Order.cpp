#include "include/core/Order.h"
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <cmath>

static std::string now_iso8601() {
    auto tp = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm lt{};
#if defined(_WIN32)
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    std::ostringstream os;
    os << std::put_time(&lt, "%Y-%m-%dT%H:%M:%S");
    return os.str();
}

double Order::calcTotal(const std::map<std::string, double>& priceList) const {
    // Для заказов в статусе "new" используем текущие цены из priceList
    // Для остальных статусов используем сохраненные цены из itemPrices
    if (status == "new") {
        // Для заказов "new" используем текущие цены, но если есть сохраненные - используем их
        double s = 0.0;
        for (auto& kv : items) {
            auto savedPriceIt = itemPrices.find(kv.first);
            if (savedPriceIt != itemPrices.end()) {
                s += savedPriceIt->second * kv.second;
            } else {
                auto it = priceList.find(kv.first);
                if (it != priceList.end())
                    s += it->second * kv.second;
            }
        }
        return std::round(s * 100.0) / 100.0;
    } else {
        // Для заказов не в статусе "new" используем сохраненные цены
        if (!itemPrices.empty()) {
            return calcTotalFromSavedPrices();
        }
        // Если нет сохраненных цен (старые заказы), используем текущие цены
        double s = 0.0;
        for (auto& kv : items) {
            auto it = priceList.find(kv.first);
            if (it != priceList.end())
                s += it->second * kv.second;
        }
        return std::round(s * 100.0) / 100.0;
    }
}

double Order::calcTotalFromSavedPrices() const {
    double s = 0.0;
    for (auto& kv : items) {
        auto it = itemPrices.find(kv.first);
        if (it != itemPrices.end())
            s += it->second * kv.second;
    }
    return std::round(s * 100.0) / 100.0;
}

std::string Order::toLine() const {
    std::ostringstream os;
    os.setf(std::ios::fixed);
    os << std::setprecision(2);
    os << id << ';' << client << ';' << status << ';' << total << ';' << createdAt << ';';
    bool first = true;
    for (auto& kv : items) {
        if (!first) os << ',';
        os << kv.first << ':' << kv.second;
        first = false;
    }
    // Сохраняем цены товаров, если они есть
    if (!itemPrices.empty()) {
        os << '|'; // Разделитель между items и prices
        first = true;
        for (auto& kv : itemPrices) {
            if (!first) os << ',';
            // Сохраняем цену с точностью до 2 знаков после запятой
            // НЕ округляем значение, только форматируем вывод
            // setprecision(2) уже установлен в начале функции
            os << kv.first << ':' << kv.second;
            first = false;
        }
    }
    return os.str();
}

std::optional<Order> Order::fromLine(const std::string& line) {
    std::istringstream ss(line);
    Order o;
    std::string idStr, clientStr, statusStr, totalStr, createdStr, itemsStr;

    if (!std::getline(ss, idStr, ';')) return std::nullopt;
    if (!std::getline(ss, clientStr, ';')) return std::nullopt;
    if (!std::getline(ss, statusStr, ';')) return std::nullopt;
    if (!std::getline(ss, totalStr, ';')) return std::nullopt;

    std::string rest;
    std::getline(ss, rest);
    std::istringstream restStream(rest);
    if (std::getline(restStream, createdStr, ';')) {
        std::getline(restStream, itemsStr);
    } else {
        createdStr.clear();
        itemsStr = rest;
    }

    o.id = std::stoi(idStr);
    o.client = clientStr;
    o.status = statusStr;
    {
        std::string t = totalStr;
        std::replace(t.begin(), t.end(), ',', '.');
        double v = std::stod(t);
        o.total = std::round(v * 100.0) / 100.0;
    }
    o.createdAt = createdStr.empty() ? now_iso8601() : createdStr;

    // Разделяем items и prices (разделитель |)
    std::string pricesStr;
    size_t pipePos = itemsStr.find('|');
    if (pipePos != std::string::npos) {
        pricesStr = itemsStr.substr(pipePos + 1);
        itemsStr = itemsStr.substr(0, pipePos);
    }

    // Загружаем items (нормализуем имена к lowercase для совместимости)
    std::istringstream itemStream(itemsStr);
    std::string pair;
    while (std::getline(itemStream, pair, ',')) {
        size_t pos = pair.find(':');
        if (pos != std::string::npos) {
            std::string name = pair.substr(0, pos);
            // Нормализуем имя к lowercase
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            int qty = std::stoi(pair.substr(pos + 1));
            o.items[name] = qty;
        }
    }
    
    // Загружаем prices, если они есть (нормализуем имена к lowercase)
    if (!pricesStr.empty()) {
        std::istringstream priceStream(pricesStr);
        while (std::getline(priceStream, pair, ',')) {
            size_t pos = pair.find(':');
            if (pos != std::string::npos) {
                std::string name = pair.substr(0, pos);
                // Нормализуем имя к lowercase
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                std::string priceStr = pair.substr(pos + 1);
                std::replace(priceStr.begin(), priceStr.end(), ',', '.');
                // Парсим цену как строку, чтобы сохранить точность
                double price = std::stod(priceStr);
                // Сохраняем цену БЕЗ округления, чтобы сохранить точность из файла
                // Округление будет происходить только при расчете total и при сохранении
                // Но само значение должно сохраняться с максимальной точностью
                o.itemPrices[name] = price;
            }
        }
    }
    
    return o;
}

std::ostream& operator<<(std::ostream& os, const Order& o) {
    os << "Order #" << o.id << " (" << o.client << ") [" << o.status << "]\n";
    for (auto& kv : o.items)
        os << "  " << kv.first << " x" << kv.second << "\n";
    os.setf(std::ios::fixed);
    os << std::setprecision(2);
    os << "Total: " << o.total << "\n";
    os << "CreatedAt: " << o.createdAt << "\n";
    return os;
}
