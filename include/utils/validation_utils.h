#pragma once
#include <regex>
#include <string>
#include <cmath>
#include "include/Errors/CustomExceptions.h"

class ValidationService {
private:
    std::regex re;
    static inline bool money_precision_ok(double v) {
        double cents = std::round(v * 100.0);
        return std::fabs(v * 100.0 - cents) < 1e-9;
    }
public:
    void validate_client_name(const std::string& name) {
        re = std::regex(R"(^[A-Za-zА-Яа-яЁё0-9]+(?:[ .-][A-Za-zА-Яа-яЁё0-9]+)*$)");
        if (!std::regex_match(name, re)) throw ValidationException("invalid client name");
    }
    void validate_item_name(const std::string& name) {
        re = std::regex(R"(^[^\s|:;][^|:;]*$)");
        if (!std::regex_match(name, re)) throw ValidationException("invalid item name");
    }
    void validate_qty(int qty) const {
        if (qty <= 0) throw ValidationException("qty must be positive");
    }
    void validate_status(const std::string& s) {
        if (s != "new" && s != "in_progress" && s != "done" && s != "canceled")
            throw ValidationException("invalid status");
    }
    void validate_id(int id) const {
        if (id <= 0) throw ValidationException("id must be positive");
    }
    void validate_price(double price) const {
        if (!(price > 0.0)) throw ValidationException("price must be positive");
        if (!money_precision_ok(price)) throw ValidationException("price must have max 2 decimals");
    }
    double normalize_money(double v) const {
        if (!(v >= 0.0)) throw ValidationException("money must be non-negative");
        double r = std::round(v * 100.0) / 100.0;
        return r;
    }
};

