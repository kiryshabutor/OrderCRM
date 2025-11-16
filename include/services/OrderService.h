#pragma once
#include <map>
#include <string>
#include <functional>
#include <algorithm>
#include "include/core/Order.h"
#include "include/core/IRepository.h"
#include "include/core/Product.h"
#include "include/Errors/CustomExceptions.h"
#include "include/utils/SimpleList.h"

class ProductService;

class OrderService {
private:
    SimpleList<Order> data_;
    std::map<std::string, double, std::less<>> price_;
    int nextId_{1};
    IRepository& repo_;
    ProductService* productService_{nullptr};
    void persist();
    void returnItemsToStock(Order& o);
    void removeItemsFromStock(Order& o);
public:
    explicit OrderService(IRepository& repo) : repo_(repo) {}

    void setProductService(ProductService* ps) { productService_ = ps; }
    void setPrices(const std::map<std::string, Product, std::less<>>& products);
    const std::map<std::string, double, std::less<>>& price() const { return price_; }

    Order& create(const std::string& client);
    void addItem(Order& o, const std::string& name, int qty);
    void removeItem(Order& o, const std::string& name);
    void setStatus(Order& o, const std::string& s);

    Order* findById(int id);
    const Order* findById(int id) const;

    void sortById();
    double revenue() const;
    void recalculateOrdersWithProduct(const std::string& productKey);

    void save();
    void load();

    const SimpleList<Order>& all() const { return data_; }
    int& nextIdRef() { return nextId_; }
};
