#pragma once
#include <QDialog>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include "include/services/OrderService.h"

class EditOrderDialog : public QDialog {
    Q_OBJECT
private:
    OrderService& svc_;
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

    QWidget* tabRemove_;
    QLineEdit* removeItemName_;
    QPushButton* removeItemBtn_;

    Order* orderOrWarn();

private slots:
    void onApplyStatus();
    void onAddItem();
    void onRemoveItem();

public:
    explicit EditOrderDialog(OrderService& svc, int orderId, QWidget* parent = nullptr);
    signals:
        void dataChanged();
};
