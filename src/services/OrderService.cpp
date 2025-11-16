#include "include/services/OrderService.h"
#include "include/services/ProductService.h"
#include "include/utils/validation_utils.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cmath>
#include <format>
#include <ranges>

static std::string now_iso8601_srv() {
    auto tp = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm lt{};
#if defined(_WIN32)
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    std::ostringstream os;
    os << std::put_time(&lt, "%Y-%m-%dT%H:%M:%S");
    return os.str();
}

void OrderService::persist() {
    save();
}

Order& OrderService::create(const std::string& client) {
    ValidationService V;
    V.validate_client_name(client);
    Order o;
    o.id = nextId_++;
    o.client = client;
    o.status = "new";
    o.total = 0;
    o.createdAt = now_iso8601_srv();
    data_.push_back(o);
    persist();
    return data_[data_.size() - 1];
}

void OrderService::setPrices(const std::map<std::string, Product, std::less<>>& products) {
    price_.clear();
    for (const auto& [key, product] : products) {
        price_[key] = product.price;
    }
}

void OrderService::addItem(Order& o, const std::string& item, int qty) {
    if (qty <= 0) throw ValidationException("qty must be positive");
    std::string key = item;
    std::ranges::transform(key, key.begin(), [](unsigned char c){ return std::tolower(c); });
    if (auto it = price_.find(key); it == price_.end()) {
        throw NotFoundException("item not found in product base");
    }
    
    if (o.status != "canceled") {
        if (!productService_) {
            throw ValidationException("product service not initialized");
        }
        
        if (int availableStock = productService_->getStock(key); availableStock < qty) {
            throw ValidationException(std::format("not enough stock. Available: {}, needed: {}", availableStock, qty));
        }
        
        try {
            productService_->decreaseStock(key, qty);
            productService_->save();
        } catch (const NotFoundException&) {
            throw ValidationException("product not found: " + key);
        }
    }
    
    o.items[key] += qty;
    o.total = o.calcTotal(price_);
    o.total = std::round(o.total * 100.0) / 100.0;
    persist();
}

void OrderService::removeItem(Order& o, const std::string& name) {
    std::string key = name;
    std::ranges::transform(key, key.begin(), [](unsigned char c){ return std::tolower(c); });
    const auto it = o.items.find(key);
    if (it == o.items.end()) {
        throw NotFoundException("item not found in this order");
    }
    const int qty = it->second;
    o.items.erase(it);
    
    if (o.status != "canceled" && productService_) {
        productService_->increaseStock(key, qty);
        productService_->save();
    }
    
    o.total = o.calcTotal(price_);
    o.total = std::round(o.total * 100.0) / 100.0;
    persist();
}

void OrderService::returnItemsToStock(const Order& o) {
    if (!productService_) return;
    for (const auto& [itemKey, qty] : o.items) {
        productService_->increaseStock(itemKey, qty);
    }
    productService_->save();
}

void OrderService::removeItemsFromStock(const Order& o) {
    if (!productService_) return;
    for (const auto& [itemKey, qty] : o.items) {
        if (!productService_->hasEnoughStock(itemKey, qty)) {
            const int available = productService_->getStock(itemKey);
            throw ValidationException(std::format("not enough stock for {}. Available: {}, needed: {}", itemKey, available, qty));
        }
        productService_->decreaseStock(itemKey, qty);
    }
    productService_->save();
}

void OrderService::setStatus(Order& o, const std::string& s) {
    ValidationService V;
    V.validate_status(s);
    
    std::string oldStatus = o.status;
    o.status = s;
    
    if (productService_) {
        if (oldStatus == "canceled" && s != "canceled") {
            try {
                removeItemsFromStock(o);
            } catch (const ValidationException&) {
                o.status = oldStatus;
                throw;
            }
        } else if (oldStatus != "canceled" && s == "canceled") {
            returnItemsToStock(o);
        }
    }
    
    persist();
}

Order* OrderService::findById(int id) {
    return find_if(data_, [id](const Order& o) { return o.id == id; });
}

const Order* OrderService::findById(int id) const {
    return find_if(data_, [id](const Order& o) { return o.id == id; });
}

void OrderService::sortById() {
    std::ranges::sort(data_, [](const Order& a, const Order& b) {
        return a.id < b.id;
    });
}

double OrderService::revenue() const {
    double s = 0;
    for (const auto& o : data_) s += o.total;
    return std::round(s * 100.0) / 100.0;
}

void OrderService::recalculateOrdersWithProduct(const std::string& productKey) {
    std::string key = productKey;
    std::ranges::transform(key, key.begin(), [](unsigned char c){ return std::tolower(c); });
    
    bool changed = false;
    for (auto& order : data_) {
        if (order.items.contains(key)) {
            double oldTotal = order.total;
            order.total = order.calcTotal(price_);
            order.total = std::round(order.total * 100.0) / 100.0;
            if (std::abs(oldTotal - order.total) > 0.01) {
                changed = true;
            }
        }
    }
    
    if (changed) {
        persist();
    }
}

void OrderService::save() {
    std::vector<Order> temp;
    for (const auto& o : data_) {
        Order c = o;
        c.total = c.calcTotal(price_);
        c.total = std::round(c.total * 100.0) / 100.0;
        temp.push_back(c);
    }
    repo_.save(temp);
}

void OrderService::load() {
    auto loaded = repo_.load();
    data_.clear();
    for (const auto& o : loaded) {
        Order c = o;
        c.total = c.calcTotal(price_);
        c.total = std::round(c.total * 100.0) / 100.0;
        
        data_.push_back(c);
        nextId_ = std::max(nextId_, c.id + 1);
    }
}
