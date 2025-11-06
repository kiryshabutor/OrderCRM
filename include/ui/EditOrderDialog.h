#pragma once
#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QCompleter>
#include "include/services/OrderService.h"
#include "include/services/ProductService.h"

class EditOrderDialog : public QDialog {
    Q_OBJECT
private:
    OrderService& svc_;
    ProductService* productSvc_;
    int orderId_;
    QLineEdit* idEdit_;
    QTabWidget* tabs_;

    QWidget* tabStatus_;
    QComboBox* statusCombo_;
    QPushButton* applyStatusBtn_;

    QWidget* tabAdd_;
    QLineEdit* addItemName_;
    QLineEdit* addQty_;
    QPushButton* addItemBtn_;
    QCompleter* addItemCompleter_;

    QWidget* tabRemove_;
    QLineEdit* removeItemName_;
    QPushButton* removeItemBtn_;
    QCompleter* removeItemCompleter_;

    Order* orderOrWarn();
    void setupCompleters();

private slots:
    void onApplyStatus();
    void onAddItem();
    void onRemoveItem();

public:
    explicit EditOrderDialog(OrderService& svc, int orderId, QWidget* parent = nullptr);
    void setProductService(ProductService* productSvc);
    signals:
        void dataChanged();
};
