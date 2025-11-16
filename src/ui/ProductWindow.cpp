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
        auto rowData = createProductTableRow(productTable_, i, prod, this);
        connect(rowData.editBtn, &QPushButton::clicked, this, [this, productKey, productName = prod.name]() {
            onEditProduct(productKey, productName);
        });
        connect(rowData.deleteBtn, &QPushButton::clicked, this, [this, productKey, productName = prod.name]() {
            onDeleteProduct(productKey, productName);
        });
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
            auto dialogResult = showDeleteProductDialog(this, qs(productName), affectedOrderIds.size());
            if (dialogResult.shouldCancel) {
                return;
            }
            
            if (dialogResult.shouldCancelOrders) {
                orderSvc_.setProductService(&productSvc_);
                for (int orderId : affectedOrderIds) {
                    cancelOrderSafely(orderId);
                }
                orderSvc_.save();
                emit ordersChanged();
            }
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
    auto dialogData = createProductEditDialog(this, product);
    
    if (!dialogData.dialog) {
        return;
    }
    
    connect(dialogData.buttons, &QDialogButtonBox::accepted, dialogData.dialog, [this, dialogData, oldName = ss(dialogData.oldName)]() {
        handleProductEditSave(dialogData.fields.nameEdit, dialogData.fields.priceEdit, dialogData.fields.stockEdit, oldName, dialogData.dialog);
    });
    
    connect(dialogData.buttons, &QDialogButtonBox::rejected, dialogData.dialog, &QDialog::reject);
    
    dialogData.dialog->exec();
    delete dialogData.dialog;
}

void ProductWindow::handleProductEditSave(const QLineEdit* nameEdit, const QLineEdit* priceEdit, const QLineEdit* stockEdit, const std::string& oldName, QDialog* editDialog) {
    try {
        auto validation = validateProductEditInputs(nameEdit, priceEdit, stockEdit);
        if (!validation.isValid) {
            QMessageBox::warning(editDialog, "error", validation.errorMessage);
            return;
        }
        
        double oldPrice = 0.0;
        const Product* oldProduct = productSvc_.findProduct(oldName);
        if (oldProduct) {
            oldPrice = oldProduct->price;
        }
        
        productSvc_.updateProduct(oldName, validation.newName, validation.price, validation.stock);
        productSvc_.save();
        
        orderSvc_.setPrices(productSvc_.all());
        bool priceChanged = (oldProduct && std::abs(oldPrice - validation.price) > 0.01);
        bool nameChanged = (oldName != validation.newName);
        
        if (priceChanged || nameChanged) {
            if (nameChanged) {
                std::string oldKey = oldName;
                std::ranges::transform(oldKey, oldKey.begin(), ::tolower);
                orderSvc_.recalculateOrdersWithProduct(oldKey);
            }
            std::string newKey = validation.newName;
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

