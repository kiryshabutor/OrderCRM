#pragma once
#include <map>
#include <string>
#include "include/core/IProductRepository.h"
#include "include/Errors/CustomExceptions.h"
#include "include/utils/validation_utils.h"

class ProductService {
private:
    std::map<std::string, double> products_;
    IProductRepository& repo_;
    ValidationService V_;

public:
    explicit ProductService(IProductRepository& repo);

    const std::map<std::string, double>& all() const;

    void load();
    void save();

    void addProduct(const std::string& name, double price);
    void removeProduct(const std::string& name);
    void updateProduct(const std::string& oldName, const std::string& newName, double newPrice);
};