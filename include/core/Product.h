#pragma once
#include <string>

struct Product {
    std::string name;
    double price;
    int stock;  // количество на складе

    Product() : price(0.0), stock(0) {}
    Product(const std::string& n, double p, int s = 0) : name(n), price(p), stock(s) {}
};

