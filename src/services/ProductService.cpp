#include "include/services/ProductService.h"
#include <algorithm>
#include <cmath>

ProductService::ProductService(IProductRepository& repo) : repo_(repo) {}

const std::map<std::string, double>& ProductService::all() const {
    return products_;
}

void ProductService::load() {
    products_ = repo_.load();
    std::map<std::string, double> normalized;
    for (auto& kv : products_) {
        double v = std::round(kv.second * 100.0) / 100.0;
        if (v > 0.0) normalized[kv.first] = v;
    }
    products_.swap(normalized);
}

void ProductService::save() {
    repo_.save(products_);
}

void ProductService::addProduct(const std::string& name, double price) {
    V_.validate_item_name(name);
    V_.validate_price(price);
    std::string key = name;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    if (products_.contains(key))
        throw ValidationException("product already exists");
    double v = V_.normalize_money(price);
    if (v <= 0.0) throw ValidationException("price must be positive");
    products_[key] = v;
}

void ProductService::removeProduct(const std::string& name) {
    std::string key = name;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    auto it = products_.find(key);
    if (it == products_.end())
        throw NotFoundException("product not found");
    products_.erase(it);
}

void ProductService::updateProduct(const std::string& oldName, const std::string& newName, double newPrice) {
    std::string oldKey = oldName, newKey = newName;
    std::transform(oldKey.begin(), oldKey.end(), oldKey.begin(), ::tolower);
    std::transform(newKey.begin(), newKey.end(), newKey.begin(), ::tolower);
    auto it = products_.find(oldKey);
    if (it == products_.end())
        throw NotFoundException("product not found");
    V_.validate_item_name(newName);
    V_.validate_price(newPrice);
    double v = V_.normalize_money(newPrice);
    products_.erase(it);
    products_[newKey] = v;
}
