#pragma once
#include "include/core/IProductRepository.h"
#include <map>
#include <string>
#include <functional>

class TxtProductRepository : public IProductRepository {
private:
    std::string file_;
public:
    explicit TxtProductRepository(std::string f) : file_(std::move(f)) {}

    std::map<std::string, Product, std::less<>> load() override;
    void save(const std::map<std::string, Product, std::less<>>& data) override;
};
