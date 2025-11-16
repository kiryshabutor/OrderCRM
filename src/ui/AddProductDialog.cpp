#include "include/ui/AddProductDialog.h"
#include "include/ui/UtilsQt.h"
#include "include/Errors/CustomExceptions.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QIntValidator>
#include <QPushButton>
#include <cmath>


AddProductDialog::AddProductDialog(ProductService& productSvc, QWidget* parent)
    : QDialog(parent), productSvc_(productSvc) {
    setWindowTitle("Add product");
    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    
    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText("product name");
    form->addRow("Product name:", nameEdit_);
    
    priceEdit_ = new QLineEdit(this);
    priceEdit_->setPlaceholderText("price");
    form->addRow("Price:", priceEdit_);
    
    stockEdit_ = new QLineEdit(this);
    stockEdit_->setPlaceholderText("stock");
    stockEdit_->setValidator(new QIntValidator(0, 1000000000, this));
    form->addRow("Stock:", stockEdit_);
    
    root->addLayout(form);
    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons_->button(QDialogButtonBox::Ok)->setText("Add");
    root->addWidget(buttons_);
    
    connect(buttons_, &QDialogButtonBox::accepted, this, &AddProductDialog::onAdd);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    resize(400, 150);
}

void AddProductDialog::onAdd() {
    try {
        std::string name = formatName(ss(nameEdit_->text()));
        double price = parsePrice(priceEdit_->text());
        bool ok = false;
        int stock = stockEdit_->text().toInt(&ok);
        if (!ok || stock < 0) stock = 0;
        
        productSvc_.addProduct(name, price, stock);
        productSvc_.save();
        addedProductName_ = name;
        accept();
    } catch (const CustomException& e) {
        QMessageBox::warning(this, "error", qs(e.what()));
    }
}

