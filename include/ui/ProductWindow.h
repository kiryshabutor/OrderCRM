#pragma once
#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QCompleter>
#include "include/services/ProductService.h"
#include "include/services/OrderService.h"

class ProductWindow : public QMainWindow {
    Q_OBJECT
private:
    ProductService& productSvc_;
    OrderService& orderSvc_;

    QTableWidget* productTable_;
    QPushButton* addProductBtn_;
    QCompleter* productNameCompleter_;

    void refreshProducts();
    void setupCompleters();
    void onEditProduct(const std::string& productKey, const std::string& productName);
    void onDeleteProduct(const std::string& productKey, const std::string& productName);
    bool isProductUsedInActiveOrders(const std::string& productKey, QList<int>& affectedOrderIds);

private slots:
    void onAddProduct();

signals:
    void ordersChanged(); // Сигнал об изменении заказов

public:
    explicit ProductWindow(ProductService& productSvc, OrderService& orderSvc, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
};

