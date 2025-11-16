#pragma once
#include <string>
#include <map>
#include <functional>
#include <ostream>
#include <optional>
#include <compare>
#include <iomanip>

class Order {
public:
    int id;
    std::string client;
    std::string status;
    std::map<std::string, int, std::less<>> items;
    double total;
    std::string createdAt;

    double calcTotal(const std::map<std::string, double, std::less<>>& priceList) const;

    auto operator<=>(const Order& other) const { return id <=> other.id; }
    bool operator==(const Order& other) const { return id == other.id; }

    std::string toLine() const;
    static std::optional<Order> fromLine(const std::string& line);

    friend std::ostream& operator<<(std::ostream& os, const Order& o) {
        os << "Order #" << o.id << " (" << o.client << ") [" << o.status << "]\n";
        for (const auto& [itemKey, qty] : o.items)
            os << "  " << itemKey << " x" << qty << "\n";
        os.setf(std::ios::fixed);
        os << std::setprecision(2);
        os << "Total: " << o.total << "\n";
        os << "CreatedAt: " << o.createdAt << "\n";
        return os;
    }
};
