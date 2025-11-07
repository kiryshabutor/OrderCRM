#pragma once
#include <vector>
#include "include/core/Order.h"

class IRepository {
public:
    virtual ~IRepository() = default;
    virtual void save(const std::vector<Order>& data) = 0;
    virtual std::vector<Order> load() = 0;
};
