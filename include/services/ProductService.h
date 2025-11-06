#pragma once
#include <map>
#include <string>
#include "include/core/IProductRepository.h"
#include "include/core/Product.h"
#include "include/Errors/CustomExceptions.h"
#include "include/utils/validation_utils.h"

class ProductService {
private:
    std::map<std::string, Product> products_;
    IProductRepository& repo_;
    ValidationService V_;

public:
    explicit ProductService(IProductRepository& repo);

    const std::map<std::string, Product>& all() const;
    Product* findProduct(const std::string& name);
    const Product* findProduct(const std::string& name) const;

    void load();
    void save();

    void addProduct(const std::string& name, double price, int stock = 0);
    void removeProduct(const std::string& name);
    void updateProduct(const std::string& oldName, const std::string& newName, double newPrice, int stock = -1);
    
    // Методы для работы с количеством на складе
    void decreaseStock(const std::string& name, int qty);
    void increaseStock(const std::string& name, int qty);
    bool hasEnoughStock(const std::string& name, int qty) const;
    int getStock(const std::string& name) const;
};