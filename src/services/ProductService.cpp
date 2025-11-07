#include "include/services/ProductService.h"
#include <algorithm>
#include <cmath>

ProductService::ProductService(IProductRepository& repo) : repo_(repo) {}

const std::map<std::string, Product>& ProductService::all() const {
    return products_;
}

Product* ProductService::findProduct(const std::string& name) {
    std::string key = name;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    auto it = products_.find(key);
    return it != products_.end() ? &it->second : nullptr;
}

const Product* ProductService::findProduct(const std::string& name) const {
    std::string key = name;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    auto it = products_.find(key);
    return it != products_.end() ? &it->second : nullptr;
}

void ProductService::load() {
    products_ = repo_.load();
    std::map<std::string, Product> normalized;
    for (auto& kv : products_) {
        Product p = kv.second;
        p.price = std::round(p.price * 100.0) / 100.0;
        if (p.price > 0.0) {
            if (p.stock < 0) p.stock = 0;
            normalized[kv.first] = p;
        }
    }
    products_.swap(normalized);
}

void ProductService::save() {
    repo_.save(products_);
}

void ProductService::addProduct(const std::string& name, double price, int stock) {
    V_.validate_item_name(name);
    V_.validate_price(price);
    if (stock < 0) throw ValidationException("stock cannot be negative");
    std::string key = name;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    if (products_.contains(key))
        throw ValidationException("product already exists");
    double v = V_.normalize_money(price);
    if (v <= 0.0) throw ValidationException("price must be positive");
    products_[key] = Product(name, v, stock);
}

void ProductService::removeProduct(const std::string& name) {
    std::string key = name;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    auto it = products_.find(key);
    if (it == products_.end())
        throw NotFoundException("product not found");
    products_.erase(it);
}

void ProductService::updateProduct(const std::string& oldName, const std::string& newName, double newPrice, int stock) {
    std::string oldKey = oldName, newKey = newName;
    std::transform(oldKey.begin(), oldKey.end(), oldKey.begin(), ::tolower);
    std::transform(newKey.begin(), newKey.end(), newKey.begin(), ::tolower);
    auto it = products_.find(oldKey);
    if (it == products_.end())
        throw NotFoundException("product not found");
    V_.validate_item_name(newName);
    V_.validate_price(newPrice);
    double v = V_.normalize_money(newPrice);
    Product p = it->second;
    p.name = newName;
    p.price = v;
    if (stock >= 0) p.stock = stock;
    products_.erase(it);
    products_[newKey] = p;
}

void ProductService::decreaseStock(const std::string& name, int qty) {
    Product* p = findProduct(name);
    if (!p) throw NotFoundException("product not found");
    if (p->stock < qty) throw ValidationException("not enough stock");
    p->stock -= qty;
}

void ProductService::increaseStock(const std::string& name, int qty) {
    Product* p = findProduct(name);
    if (!p) throw NotFoundException("product not found");
    p->stock += qty;
}

bool ProductService::hasEnoughStock(const std::string& name, int qty) const {
    const Product* p = findProduct(name);
    if (!p) return false;
    return p->stock >= qty;
}

int ProductService::getStock(const std::string& name) const {
    const Product* p = findProduct(name);
    if (!p) return 0;
    return p->stock;
}
