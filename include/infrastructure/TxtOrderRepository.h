#pragma once
#include <string>
#include "include/core/IRepository.h"

class TxtOrderRepository : public IRepository {
private:
    std::string file_;
public:
    explicit TxtOrderRepository(std::string f) : file_(std::move(f)) {}
    void save(const std::vector<Order>& data) override;
    std::vector<Order> load() override;
};

