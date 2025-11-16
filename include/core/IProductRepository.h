#pragma once
#include <map>
#include <string>
#include <functional>
#include "include/core/Product.h"

class IProductRepository {
public:
    virtual ~IProductRepository() = default;
    virtual void save(const std::map<std::string, Product, std::less<>>& data) = 0;
    virtual std::map<std::string, Product, std::less<>> load() = 0;
};
