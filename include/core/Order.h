#pragma once
#include <string>
#include <map>
#include <ostream>
#include <optional>

class Order {
public:
    int id;
    std::string client;
    std::string status;
    std::map<std::string, int> items;
    double total;
    std::string createdAt;

    double calcTotal(const std::map<std::string, double>& priceList) const;

    bool operator<(const Order& other) const { return id < other.id; }
    bool operator==(const Order& other) const { return id == other.id; }

    std::string toLine() const;
    static std::optional<Order> fromLine(const std::string& line);

    friend std::ostream& operator<<(std::ostream& os, const Order& o);
};
