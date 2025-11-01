#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include "include/services/OrderService.h"

class AddOrderDialog : public QDialog {
    Q_OBJECT
private:
    OrderService& svc_;
    QLineEdit* clientEdit_;
    QDialogButtonBox* buttons_;
    int createdId_{-1};
private slots:
    void onAdd();
public:
    explicit AddOrderDialog(OrderService& svc, QWidget* parent = nullptr);
    int createdOrderId() const { return createdId_; }
};
