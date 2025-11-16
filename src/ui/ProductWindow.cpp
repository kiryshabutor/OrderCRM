#include "include/ui/ProductWindow.h"
#include "include/ui/AddProductDialog.h"
#include "include/ui/UtilsQt.h"
#include "include/ui/NumericItem.h"
#include "include/Errors/CustomExceptions.h"
#include "include/core/Order.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHeaderView>
#include <QIntValidator>
#include <QShowEvent>
#include <QStringListModel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <algorithm>
#include <cmath>
#include <string_view>

static double parsePrice(const QString& input) {
    QString s = input.trimmed();
    if (s.isEmpty()) throw ValidationException("price cannot be empty");
    s.replace(',', '.');
    bool ok = false;
    double price = s.toDouble(&ok);
    if (!ok || price <= 0.0) throw ValidationException("invalid price");
    double cents = std::round(price * 100.0);
    if (std::fabs(price * 100.0 - cents) > 1e-9) throw ValidationException("price must have max 2 decimals");
    price = cents / 100.0;
    return price;
}

ProductWindow::ProductWindow(ProductService& productSvc, OrderService& orderSvc, QWidget* parent)
    : QMainWindow(parent), productSvc_(productSvc), orderSvc_(orderSvc) {
    setWindowTitle("Products");
    
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);

    productTable_ = new QTableWidget(this);
    productTable_->setColumnCount(5);
    productTable_->setHorizontalHeaderLabels({"Product","Price","Stock","",""});
    productTable_->horizontalHeader()->setStretchLastSection(false);
    productTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    productTable_->setSelectionMode(QAbstractItemView::NoSelection);
    productTable_->setSortingEnabled(true);
    root->addWidget(productTable_);

    auto* prodButtons = new QHBoxLayout();
    addProductBtn_ = new QPushButton("Add product", this);
    prodButtons->addWidget(addProductBtn_);
    prodButtons->addStretch();

    root->addLayout(prodButtons);
    setCentralWidget(central);

    connect(addProductBtn_, &QPushButton::clicked, this, &ProductWindow::onAddProduct);

    setupCompleters();
    resize(600, 400);
    refreshProducts();
}

void ProductWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    refreshProducts();
}

void ProductWindow::refreshProducts() {
    productTable_->setSortingEnabled(false);
    const auto& p = productSvc_.all();
    productTable_->clearContents();
    productTable_->setRowCount((int)p.size());
    int i = 0;
    for (const auto& [productKey, prod] : p) {
        
        auto* nameCell = new QTableWidgetItem(qs(prod.name));
        nameCell->setTextAlignment(Qt::AlignCenter);
        auto* priceCell = new NumericItem(prod.price, QString::number(prod.price, 'f', 2));
        priceCell->setTextAlignment(Qt::AlignCenter);
        auto* stockCell = new QTableWidgetItem(QString::number(prod.stock));
        stockCell->setTextAlignment(Qt::AlignCenter);
        
        productTable_->setItem(i, 0, nameCell);
        productTable_->setItem(i, 1, priceCell);
        productTable_->setItem(i, 2, stockCell);
        
        auto* editBtn = new QPushButton("⚙️", this);
        editBtn->setStyleSheet(
            "QPushButton {"
            "background-color: #2196F3;"
            "color: white;"
            "border: none;"
            "padding: 4px 8px;"
            "border-radius: 4px;"
            "font-size: 14px;"
            "}"
            "QPushButton:hover {"
            "background-color: #1976D2;"
            "}"
            "QPushButton:pressed {"
            "background-color: #0D47A1;"
            "}"
        );
        editBtn->setToolTip("Edit product");
        editBtn->setFixedSize(35, 25);
        connect(editBtn, &QPushButton::clicked, this, [this, productKey, productName = prod.name]() {
            onEditProduct(productKey, productName);
        });
        
        auto* deleteBtn = new QPushButton("❌", this);
        deleteBtn->setStyleSheet(
            "QPushButton {"
            "background-color: #F44336;"
            "color: white;"
            "border: none;"
            "padding: 4px 8px;"
            "border-radius: 4px;"
            "font-size: 14px;"
            "}"
            "QPushButton:hover {"
            "background-color: #D32F2F;"
            "}"
            "QPushButton:pressed {"
            "background-color: #B71C1C;"
            "}"
        );
        deleteBtn->setToolTip("Delete product");
        deleteBtn->setFixedSize(35, 25);
        connect(deleteBtn, &QPushButton::clicked, this, [this, productKey, productName = prod.name]() {
            onDeleteProduct(productKey, productName);
        });
        
        auto* editContainer = new QWidget(this);
        auto* editLayout = new QHBoxLayout(editContainer);
        editLayout->setContentsMargins(0, 0, 0, 0);
        editLayout->addStretch();
        editLayout->addWidget(editBtn);
        editLayout->addStretch();
        
        auto* deleteContainer = new QWidget(this);
        auto* deleteLayout = new QHBoxLayout(deleteContainer);
        deleteLayout->setContentsMargins(0, 0, 0, 0);
        deleteLayout->addStretch();
        deleteLayout->addWidget(deleteBtn);
        deleteLayout->addStretch();
        
        productTable_->setCellWidget(i, 3, editContainer);
        productTable_->setCellWidget(i, 4, deleteContainer);
        
        ++i;
    }
    productTable_->setSortingEnabled(true);
    
    int totalWidth = productTable_->viewport()->width();
    productTable_->setColumnWidth(0, totalWidth * 0.35);
    productTable_->setColumnWidth(1, totalWidth * 0.20);
    productTable_->setColumnWidth(2, totalWidth * 0.20);
    productTable_->setColumnWidth(3, 50);
    productTable_->setColumnWidth(4, 50);
    
    setupCompleters();
}

void ProductWindow::setupCompleters() const {
    // Completers are not needed for this window's current implementation
}

void ProductWindow::onAddProduct() {
    AddProductDialog dlg(productSvc_, this);
    if (dlg.exec() == QDialog::Accepted) {
        if (std::string addedName = dlg.addedProductName(); !addedName.empty()) {
            orderSvc_.setPrices(productSvc_.all());
            std::string key = addedName;
            std::ranges::transform(key, key.begin(), ::tolower);
            orderSvc_.recalculateOrdersWithProduct(key);
            orderSvc_.save();
            emit ordersChanged();
        }
        refreshProducts();
    }
}


bool ProductWindow::isProductUsedInActiveOrders(const std::string& productKey, QList<int>& affectedOrderIds) const {
    affectedOrderIds.clear();
    const auto& orders = orderSvc_.all();
    for (const auto& order : orders) {
        if ((order.status == "new" || order.status == "in_progress") && order.items.contains(productKey)) {
            affectedOrderIds.append(order.id);
        }
    }
    return !affectedOrderIds.isEmpty();
}

void ProductWindow::cancelOrderSafely(int orderId) {
    Order* order = orderSvc_.findById(orderId);
    if (!order) {
        return;
    }
    try {
        orderSvc_.setStatus(*order, "canceled");
        if (order->status != "canceled") {
            QMessageBox::warning(this, "error", QString("Failed to cancel order %1").arg(orderId));
        }
    } catch (const CustomException& e) {
        QMessageBox::warning(this, "error", QString("Failed to cancel order %1: %2")
                            .arg(orderId)
                            .arg(qs(e.what())));
    }
}

void ProductWindow::onDeleteProduct(const std::string& productKey, const std::string& productName) {
    try {
        QList<int> affectedOrderIds;
        bool isUsed = isProductUsedInActiveOrders(productKey, affectedOrderIds);
        
        if (isUsed) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Delete Product");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(QString("Product '%1' is used in %2 active order(s) (new or in_progress).")
                          .arg(qs(productName))
                          .arg(affectedOrderIds.size()));
            msgBox.setInformativeText("What would you like to do?");
            
            QPushButton* cancelBtn = msgBox.addButton("Cancel", QMessageBox::RejectRole);
            msgBox.addButton("Cancel All Orders", QMessageBox::AcceptRole);
            
            msgBox.setDefaultButton(cancelBtn);
            msgBox.exec();
            
            if (msgBox.clickedButton() == cancelBtn) {
                return;
            }
            
            orderSvc_.setProductService(&productSvc_);
            
            for (int orderId : affectedOrderIds) {
                cancelOrderSafely(orderId);
            }
            orderSvc_.save();
            
            emit ordersChanged();
        }
        
        productSvc_.removeProduct(productName);
        productSvc_.save();
        orderSvc_.setPrices(productSvc_.all());
        orderSvc_.save();
        refreshProducts();
        
        if (isUsed) {
            QMessageBox::information(this, "ok", QString("Product removed. %1 order(s) were canceled.")
                                    .arg(affectedOrderIds.size()));
        } else {
            QMessageBox::information(this, "ok", "product removed");
        }
    } catch (const CustomException& e) { 
        QMessageBox::warning(this, "error", qs(e.what())); 
    }
}

void ProductWindow::onEditProduct([[maybe_unused]] std::string_view productKey, std::string_view productName) {
    const Product* product = productSvc_.findProduct(std::string(productName));
    if (!product) {
        QMessageBox::warning(this, "error", "product not found");
        return;
    }
    
    auto* editDialog = new QDialog(this);
    editDialog->setWindowTitle("Edit Product");
    editDialog->setModal(true);
    
    auto* layout = new QVBoxLayout(editDialog);
    auto* form = new QFormLayout();
    
    auto* nameEdit = new QLineEdit(editDialog);
    nameEdit->setText(qs(product->name));
    form->addRow("Product name:", nameEdit);
    
    auto* priceEdit = new QLineEdit(editDialog);
    priceEdit->setText(QString::number(product->price, 'f', 2));
    form->addRow("Price:", priceEdit);
    
    auto* stockEdit = new QLineEdit(editDialog);
    stockEdit->setText(QString::number(product->stock));
    stockEdit->setValidator(new QIntValidator(0, 1000000000, editDialog));
    form->addRow("Stock:", stockEdit);
    
    layout->addLayout(form);
    
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, editDialog);
    buttons->button(QDialogButtonBox::Ok)->setText("Save");
    layout->addWidget(buttons);
    
        connect(buttons, &QDialogButtonBox::accepted, editDialog, [this, editDialog, nameEdit, priceEdit, stockEdit, oldName = std::string(productName)]() {
            handleProductEditSave(nameEdit, priceEdit, stockEdit, oldName, editDialog);
        });
    
    connect(buttons, &QDialogButtonBox::rejected, editDialog, &QDialog::reject);
    
    editDialog->resize(400, 150);
    editDialog->exec();
    delete editDialog;
}

void ProductWindow::handleProductEditSave(QLineEdit* nameEdit, QLineEdit* priceEdit, QLineEdit* stockEdit, const std::string& oldName, QDialog* editDialog) {
    try {
        std::string newName = formatName(ss(nameEdit->text()));
        if (newName.empty()) {
            QMessageBox::warning(editDialog, "error", "Product name cannot be empty");
            return;
        }
        
        double oldPrice = 0.0;
        const Product* oldProduct = productSvc_.findProduct(oldName);
        if (oldProduct) {
            oldPrice = oldProduct->price;
        }
        
        double price = parsePrice(priceEdit->text());
        bool ok = false;
        int stock = stockEdit->text().toInt(&ok);
        if (!ok || stock < 0) {
            QMessageBox::warning(editDialog, "error", "Invalid stock value");
            return;
        }
        
        productSvc_.updateProduct(oldName, newName, price, stock);
        productSvc_.save();
        
        orderSvc_.setPrices(productSvc_.all());
        bool priceChanged = (oldProduct && std::abs(oldPrice - price) > 0.01);
        bool nameChanged = (oldName != newName);
        
        if (priceChanged || nameChanged) {
            if (nameChanged) {
                std::string oldKey = oldName;
                std::ranges::transform(oldKey, oldKey.begin(), ::tolower);
                orderSvc_.recalculateOrdersWithProduct(oldKey);
            }
            std::string newKey = newName;
            std::ranges::transform(newKey, newKey.begin(), ::tolower);
            orderSvc_.recalculateOrdersWithProduct(newKey);
        }
        orderSvc_.save();
        
        refreshProducts();
        if (priceChanged || nameChanged) {
            emit ordersChanged();
        }
        
        editDialog->accept();
        QMessageBox::information(this, "ok", "product updated");
    } catch (const CustomException& e) {
        QMessageBox::warning(editDialog, "error", qs(e.what()));
    }
}

