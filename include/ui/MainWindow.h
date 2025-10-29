#pragma once
#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QDateTime>
#include <QList>
#include "include/services/OrderService.h"
#include "include/services/ProductService.h"
#include "include/utils/validation_utils.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
private:
    OrderService& svc_;
    ProductService& productSvc_;
    ValidationService V_;

    QTableWidget* table_;
    QTableWidget* productTable_;
    QLineEdit* clientEdit_;
    QLineEdit* orderIdEdit_;
    QLineEdit* itemNameEdit_;
    QLineEdit* qtyEdit_;
    QPushButton* addOrderBtn_;
    QPushButton* addItemBtn_;
    QPushButton* removeItemBtn_;
    QPushButton* setStatusBtn_;
    QPushButton* saveBtn_;
    QPushButton* loadBtn_;
    QComboBox* statusCombo_;

    QLineEdit* productNameEdit_;
    QLineEdit* productPriceEdit_;
    QPushButton* addProductBtn_;
    QPushButton* updateProductBtn_;
    QPushButton* removeProductBtn_;

    QPushButton* filterBtn_;
    QPushButton* clearFilterBtn_;
    QLabel* titleLabel_;
    QWidget* orderControls_;

    QWidget* reportControls_;
    QLineEdit* reportNameEdit_;
    QPushButton* generateReportBtn_;

    QString activeClientFilter_;
    QString activeStatusFilter_;
    QString minTotalText_;
    QString maxTotalText_;
    QString minIdText_;
    QString maxIdText_;
    QDateTime fromDate_;
    QDateTime toDate_;
    bool useFrom_{false};
    bool useTo_{false};

    Order* orderByIdFromInput();
    QList<const Order*> currentFilteredRows() const;
    QString sanitizedReportBaseName() const;

private slots:
    void onAddOrder();
    void onAddItem();
    void onRemoveItem();
    void onSetStatus();
    void onSave();
    void onLoad();
    void onAddProduct();
    void onUpdateProduct();
    void onRemoveProduct();
    void onOpenFilter();
    void onClearFilter();
    void onGenerateReport();

public:
    void refreshTable();
    void refreshProducts();
    explicit MainWindow(OrderService& svc, ProductService& productSvc, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;
};
