#pragma once
#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QCompleter>
#include <string_view>
#include "include/services/ProductService.h"
#include "include/services/OrderService.h"

class ProductWindow : public QMainWindow {
    Q_OBJECT
private:
    ProductService& productSvc_;
    OrderService& orderSvc_;

    QTableWidget* productTable_;
    QPushButton* addProductBtn_;
    QCompleter* productNameCompleter_{nullptr};

    void refreshProducts();
    void setupCompleters() const;
    void onEditProduct([[maybe_unused]] std::string_view productKey, std::string_view productName);
    void onDeleteProduct(const std::string& productKey, const std::string& productName);
    bool isProductUsedInActiveOrders(const std::string& productKey, QList<int>& affectedOrderIds) const;
    void cancelOrderSafely(int orderId);
    void handleProductEditSave(const QLineEdit* nameEdit, const QLineEdit* priceEdit, const QLineEdit* stockEdit, const std::string& oldName, QDialog* editDialog);

private slots:
    void onAddProduct();

signals:
    void ordersChanged();

public:
    explicit ProductWindow(ProductService& productSvc, OrderService& orderSvc, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
};

