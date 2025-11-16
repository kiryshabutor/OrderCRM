#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QCompleter>
#include "include/services/OrderService.h"

class AddOrderDialog : public QDialog {
    Q_OBJECT
private:
    OrderService& svc_;
    QLineEdit* clientEdit_;
    QDialogButtonBox* buttons_;
    QCompleter* clientCompleter_{nullptr};
    int createdId_{-1};
    
    void setupCompleter();
    
private slots:
    void onAdd();
public:
    explicit AddOrderDialog(OrderService& svc, QWidget* parent = nullptr);
    int createdOrderId() const { return createdId_; }
};
