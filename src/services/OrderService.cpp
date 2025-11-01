#include "include/services/OrderService.h"
#include "include/utils/validation_utils.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cmath>

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

void OrderService::addItem(Order& o, const std::string& item, int qty) {
    if (qty <= 0) throw ValidationException("qty must be positive");
    std::string key = item;
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return std::tolower(c); });
    auto it = price_.find(key);
    if (it == price_.end()) throw NotFoundException("item not found in product base");
    o.items[key] += qty;
    o.total = o.calcTotal(price_);
    o.total = std::round(o.total * 100.0) / 100.0;
    persist();
}

void OrderService::removeItem(Order& o, const std::string& name) {
    std::string key = name;
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return std::tolower(c); });
    auto it = o.items.find(key);
    if (it == o.items.end()) throw NotFoundException("item not found in this order");
    o.items.erase(it);
    o.total = o.calcTotal(price_);
    o.total = std::round(o.total * 100.0) / 100.0;
    persist();
}

void OrderService::setStatus(Order& o, const std::string& s) {
    ValidationService V;
    V.validate_status(s);
    o.status = s;
    persist();
}

Order* OrderService::findById(int id) {
    return find_if(data_, [id](Order& o) { return o.id == id; });
}

const Order* OrderService::findById(int id) const {
    return find_if(data_, [id](const Order& o) { return o.id == id; });
}

void OrderService::sortById() {
    std::sort(data_.begin(), data_.end(), [](const Order& a, const Order& b) {
        return a.id < b.id;
    });
}

double OrderService::revenue() const {
    double s = 0;
    for (auto& o : data_) s += o.total;
    return std::round(s * 100.0) / 100.0;
}

void OrderService::save() {
    std::vector<Order> temp;
    for (auto& o : data_) {
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
    for (auto& o : loaded) {
        Order c = o;
        c.total = c.calcTotal(price_);
        c.total = std::round(c.total * 100.0) / 100.0;
        data_.push_back(c);
        nextId_ = std::max(nextId_, c.id + 1);
    }
}
