#pragma once
#include <map>
#include <string>
#include "include/core/Product.h"

struct IProductRepository {
    virtual ~IProductRepository() = default;
    virtual void save(const std::map<std::string, Product>& data) = 0;
    virtual std::map<std::string, Product> load() = 0;
};
