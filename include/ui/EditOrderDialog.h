#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QCompleter>
#include <QTableWidget>
#include "include/services/OrderService.h"
#include "include/services/ProductService.h"

class EditOrderDialog : public QDialog {
    Q_OBJECT
private:
    OrderService& svc_;
    ProductService* productSvc_{nullptr};
    int orderId_;
    QLineEdit* idEdit_;
    QComboBox* statusCombo_;
    QTableWidget* itemsTable_;
    QLineEdit* addItemName_;
    QLineEdit* addQty_;
    QPushButton* addItemBtn_;
    QCompleter* addItemCompleter_{nullptr};

    Order* orderOrWarn();
    void setupCompleters();
    void refreshItemsTable();
    void onEditItem(const std::string& itemKey, int currentQty);
    void onDeleteItem(const std::string& itemKey);
    void updateItemQuantity(Order* order, const std::string& itemKey, int newQty, int currentQty);

private slots:
    void onApplyStatus();
    void onAddItem();

public:
    explicit EditOrderDialog(OrderService& svc, int orderId, QWidget* parent = nullptr);
    void setProductService(ProductService* productSvc);
    signals:
        void dataChanged();
};
