#include "include/services/OrderService.h"
#include "include/services/ProductService.h"
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

void OrderService::setPrices(const std::map<std::string, Product>& products) {
    price_.clear();
    for (const auto& kv : products) {
        price_[kv.first] = kv.second.price;
    }
}

void OrderService::addItem(Order& o, const std::string& item, int qty) {
    if (qty <= 0) throw ValidationException("qty must be positive");
    std::string key = item;
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return std::tolower(c); });
    auto it = price_.find(key);
    if (it == price_.end()) throw NotFoundException("item not found in product base");
    
    // Всегда проверяем наличие на складе для неотмененных заказов
    if (o.status != "canceled") {
        if (!productService_) {
            throw ValidationException("product service not initialized");
        }
        
        // Проверяем только новое количество, которое добавляем
        // (если товар уже есть в заказе, его количество уже снято со склада)
        int availableStock = productService_->getStock(key);
        if (availableStock < qty) {
            throw ValidationException("not enough stock. Available: " + std::to_string(availableStock) + 
                                      ", needed: " + std::to_string(qty));
        }
        
        // Уменьшаем количество на складе (внутри decreaseStock есть дополнительная проверка)
        try {
            productService_->decreaseStock(key, qty);
            productService_->save();
        } catch (const ValidationException& e) {
            // Если не хватает товара (хотя мы проверили), выбрасываем исключение
            throw;
        } catch (const NotFoundException& e) {
            // Если товар не найден (хотя мы проверили), выбрасываем исключение
            throw ValidationException("product not found: " + key);
        }
    }
    
    o.items[key] += qty;
    
    // Сохраняем текущую цену товара для всех статусов
    // Для заказов "new" это нужно для последующего сохранения при смене статуса
    // Для заказов не в статусе "new" это нужно, если товар добавляется в уже существующий заказ
    // Пытаемся получить цену напрямую из ProductService для максимальной точности
    double priceToSave = it->second;
    if (productService_) {
        const Product* product = productService_->findProduct(key);
        if (product) {
            // Используем цену напрямую из Product для точности
            priceToSave = product->price;
        }
    }
    o.itemPrices[key] = priceToSave;
    
    // Пересчитываем total для всех заказов
    // Для заказов "new" используем текущие цены, для остальных - сохраненные
    o.total = o.calcTotal(price_);
    o.total = std::round(o.total * 100.0) / 100.0;
    
    persist();
}

void OrderService::removeItem(Order& o, const std::string& name) {
    std::string key = name;
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){ return std::tolower(c); });
    auto it = o.items.find(key);
    if (it == o.items.end()) throw NotFoundException("item not found in this order");
    
    int qty = it->second;
    o.items.erase(it);
    
    // Удаляем цену товара из сохраненных цен
    o.itemPrices.erase(key);
    
    // Возвращаем товар на склад только если заказ не отменен
    if (o.status != "canceled" && productService_) {
        productService_->increaseStock(key, qty);
        productService_->save();
    }
    
    // Пересчитываем total для всех заказов
    o.total = o.calcTotal(price_);
    o.total = std::round(o.total * 100.0) / 100.0;
    
    persist();
}

void OrderService::returnItemsToStock(Order& o) {
    if (!productService_) return;
    for (const auto& kv : o.items) {
        productService_->increaseStock(kv.first, kv.second);
    }
    productService_->save();
}

void OrderService::removeItemsFromStock(Order& o) {
    if (!productService_) return;
    for (const auto& kv : o.items) {
        if (!productService_->hasEnoughStock(kv.first, kv.second)) {
            int available = productService_->getStock(kv.first);
            throw ValidationException("not enough stock for " + kv.first + 
                                      ". Available: " + std::to_string(available) + 
                                      ", needed: " + std::to_string(kv.second));
        }
        productService_->decreaseStock(kv.first, kv.second);
    }
    productService_->save();
}

void OrderService::setStatus(Order& o, const std::string& s) {
    ValidationService V;
    V.validate_status(s);
    
    std::string oldStatus = o.status;
    o.status = s;
    
    // Если заказ переходит из статуса "new" в другой, сохраняем текущие цены товаров
    if (oldStatus == "new" && s != "new") {
        // Сохраняем цены всех товаров в заказе на момент изменения статуса
        // Используем цены из itemPrices, если они уже есть (для заказов new они должны быть сохранены при добавлении товаров)
        // Иначе пытаемся получить цену напрямую из ProductService для максимальной точности
        for (auto& kv : o.items) {
            // Сначала проверяем, есть ли уже сохраненная цена в itemPrices
            auto existingPriceIt = o.itemPrices.find(kv.first);
            if (existingPriceIt != o.itemPrices.end()) {
                // Цена уже сохранена, оставляем как есть
                continue;
            }
            // Иначе пытаемся получить цену напрямую из ProductService для точности
            double priceToSave = 0.0;
            if (productService_) {
                const Product* product = productService_->findProduct(kv.first);
                if (product) {
                    // Используем цену напрямую из Product, которая еще не округлена в price_
                    priceToSave = product->price;
                } else {
                    // Если продукт не найден, используем цену из price_
                    auto it = price_.find(kv.first);
                    if (it != price_.end()) {
                        priceToSave = it->second;
                    }
                }
            } else {
                // Если ProductService не доступен, используем цену из price_
                auto it = price_.find(kv.first);
                if (it != price_.end()) {
                    priceToSave = it->second;
                }
            }
            if (priceToSave > 0.0) {
                o.itemPrices[kv.first] = priceToSave;
            }
        }
        // Пересчитываем total по сохраненным ценам
        o.total = o.calcTotalFromSavedPrices();
    }
    // Если заказ переходит в статус "new" из другого статуса, обновляем цены по текущим
    else if (oldStatus != "new" && s == "new") {
        // Обновляем сохраненные цены по текущим ценам товаров
        for (auto& kv : o.items) {
            auto it = price_.find(kv.first);
            if (it != price_.end()) {
                o.itemPrices[kv.first] = it->second;
            }
        }
        // Пересчитываем total по текущим ценам
        o.total = o.calcTotal(price_);
        o.total = std::round(o.total * 100.0) / 100.0;
    }
    
    // Обработка изменения статуса для работы со складом
    if (productService_) {
        if (oldStatus == "canceled" && s != "canceled") {
            // Возврат заказа из отмены - снимаем товары со склада
            try {
                removeItemsFromStock(o);
            } catch (const ValidationException& e) {
                // Если не хватает товара, возвращаем статус обратно
                o.status = oldStatus;
                throw;
            }
        } else if (oldStatus != "canceled" && s == "canceled") {
            // Отмена заказа - возвращаем товары на склад
            returnItemsToStock(o);
        }
    }
    
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

void OrderService::updateProductPriceInNewOrders(const std::string& productKey, double newPrice) {
    for (auto& order : data_) {
        if (order.status == "new") {
            if (order.items.find(productKey) != order.items.end()) {
                // Обновляем сохраненную цену для этого товара
                order.itemPrices[productKey] = newPrice;
                // Пересчитываем total
                order.total = order.calcTotal(price_);
                order.total = std::round(order.total * 100.0) / 100.0;
            }
        }
    }
    persist();
}

void OrderService::updateProductNameInNewOrders(const std::string& oldKey, const std::string& newKey, double newPrice) {
    for (auto& order : data_) {
        if (order.status == "new") {
            if (order.items.find(oldKey) != order.items.end()) {
                // Переименовываем товар в заказе
                int qty = order.items[oldKey];
                order.items.erase(oldKey);
                order.items[newKey] = qty;
                // Обновляем цену
                order.itemPrices.erase(oldKey);
                order.itemPrices[newKey] = newPrice;
                // Пересчитываем total
                order.total = order.calcTotal(price_);
                order.total = std::round(order.total * 100.0) / 100.0;
            }
        }
    }
    persist();
}

void OrderService::save() {
    std::vector<Order> temp;
    for (auto& o : data_) {
        Order c = o;
        // Пересчитываем total только для заказов в статусе "new"
        // Для остальных используем сохраненные цены или уже вычисленный total
        if (c.status == "new") {
            c.total = c.calcTotal(price_);
            c.total = std::round(c.total * 100.0) / 100.0;
        } else if (!c.itemPrices.empty()) {
            // Используем сохраненные цены
            c.total = c.calcTotalFromSavedPrices();
        }
        // Если нет сохраненных цен и статус не "new", оставляем total как есть
        temp.push_back(c);
    }
    repo_.save(temp);
}

void OrderService::load() {
    auto loaded = repo_.load();
    data_.clear();
    for (auto& o : loaded) {
        Order c = o;
        
        // Пересчитываем total только для заказов в статусе "new"
        // Для остальных используем сохраненные цены или уже вычисленный total
        if (c.status == "new") {
            c.total = c.calcTotal(price_);
            c.total = std::round(c.total * 100.0) / 100.0;
        } else if (!c.itemPrices.empty()) {
            // Используем сохраненные цены
            c.total = c.calcTotalFromSavedPrices();
        }
        // Если нет сохраненных цен и статус не "new", оставляем total как есть
        
        // Синхронизируем склад: для неотмененных заказов товары должны быть сняты со склада
        // Если товаров нет на складе, но они есть в заказе - это нормально (они уже сняты)
        // Но мы не можем автоматически снять их, так как это может привести к отрицательным значениям
        // Поэтому просто проверяем, что если товар есть в заказе и заказ не отменен, 
        // то он должен быть снят со склада (но мы не делаем это автоматически при загрузке,
        // чтобы не сломать существующие данные)
        
        data_.push_back(c);
        nextId_ = std::max(nextId_, c.id + 1);
    }
}
