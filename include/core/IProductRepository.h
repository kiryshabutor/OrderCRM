#pragma once
#include <map>
#include <string>

struct IProductRepository {
    virtual ~IProductRepository() = default;
    virtual void save(const std::map<std::string, double>& data) = 0;
    virtual std::map<std::string, double> load() = 0;
};
