#pragma once
#include <vector>
#include "include/core/Order.h"

struct IRepository {
    virtual ~IRepository() = default;
    virtual void save(const std::vector<Order>& data) = 0;
    virtual std::vector<Order> load() = 0;
};
