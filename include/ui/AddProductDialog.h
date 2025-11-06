#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <string>
#include "include/services/ProductService.h"

class AddProductDialog : public QDialog {
    Q_OBJECT
private:
    ProductService& productSvc_;
    QLineEdit* nameEdit_;
    QLineEdit* priceEdit_;
    QLineEdit* stockEdit_;
    QDialogButtonBox* buttons_;
    std::string addedProductName_;
    
private slots:
    void onAdd();
    
public:
    explicit AddProductDialog(ProductService& productSvc, QWidget* parent = nullptr);
    std::string addedProductName() const { return addedProductName_; }
};

