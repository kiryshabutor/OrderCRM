#pragma once
#include "include/core/IProductRepository.h"
#include <map>
#include <string>

class TxtProductRepository : public IProductRepository {
private:
    std::string file_;
public:
    explicit TxtProductRepository(std::string f) : file_(std::move(f)) {}

    std::map<std::string, Product> load() override;
    void save(const std::map<std::string, Product>& data) override;
};
