#pragma once
#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QDateTime>
#include <QList>
#include <QCompleter>
#include <QStringListModel>
#include <QTabWidget>
#include <string_view>
#include "include/services/OrderService.h"
#include "include/services/ProductService.h"
#include "include/utils/validation_utils.h"

class StatisticsWindow;
class QDateTimeEdit;
class QCheckBox;

struct OrderStatsLabels {
    QLabel* newLabel_{nullptr};
    QLabel* inProgressLabel_{nullptr};
    QLabel* doneLabel_{nullptr};
    QLabel* canceledLabel_{nullptr};
    QLabel* totalRevenueLabel_{nullptr};
};

struct ProductStatsLabels {
    QLabel* lowStockLabel_{nullptr};
    QLabel* highStockLabel_{nullptr};
    QLabel* expensiveLabel_{nullptr};
    QLabel* cheapLabel_{nullptr};
    QLabel* totalCountLabel_{nullptr};
    QLabel* totalValueLabel_{nullptr};
};

struct FilterWidgets {
    QLineEdit* clientEdit_{nullptr};
    QComboBox* statusCombo_{nullptr};
    QLineEdit* minTotalEdit_{nullptr};
    QLineEdit* maxTotalEdit_{nullptr};
    QLineEdit* minIdEdit_{nullptr};
    QLineEdit* maxIdEdit_{nullptr};
    QCheckBox* useFromCheck_{nullptr};
    QDateTimeEdit* fromDateEdit_{nullptr};
    QCheckBox* useToCheck_{nullptr};
    QDateTimeEdit* toDateEdit_{nullptr};
};

struct FilterState {
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
};

class MainWindow : public QMainWindow {
    Q_OBJECT
private:
    OrderService& svc_;
    ProductService& productSvc_;
    ValidationService V_;

    QTabWidget* tabs_;
    QTableWidget* table_;
    QPushButton* addOrderBtn_;
    QPushButton* reportBtn_;
    QPushButton* clearFilterBtn_;
    QLabel* titleLabel_;
    QPushButton* openChartsBtn_;
    
    OrderStatsLabels orderStats_;
    ProductStatsLabels productStats_;
    FilterWidgets filterWidgets_;
    FilterState filterState_;

    QTableWidget* productTable_;
    QPushButton* addProductBtn_;

    StatisticsWindow* statisticsWindow_{nullptr};
    QCompleter* clientFilterCompleter_{nullptr};
    QStringListModel* clientFilterModel_{nullptr};

    QList<const Order*> currentFilteredRows() const;
    void applyFilters();
    void setupCompleters();
    bool matchesClientFilter(const Order& o) const;
    bool matchesStatusFilter(const Order& o) const;
    bool matchesTotalFilter(const Order& o) const;
    bool matchesIdFilter(const Order& o) const;
    bool matchesDateFilter(const Order& o) const;
    void setupEmptyTableRow();
    void populateTableRow(int row, const Order& o);
    QString formatOrderItems(const Order& o) const;
    QTableWidgetItem* createStatusCell(const Order& o) const;

private slots:
    void onAddOrder();
    void onOpenReportDialog();
    void onOpenStatistics();
    void onClearFilter();
    void onFilterChanged();
    void updateStatistics();
    void onAddProduct();
    void refreshProducts();
    void updateProductStatistics();
    void onEditProduct([[maybe_unused]] std::string_view productKey, std::string_view productName);
    void onDeleteProduct(const std::string& productKey, const std::string& productName);
    bool isProductUsedInActiveOrders(const std::string& productKey, QList<int>& affectedOrderIds) const;
    void cancelOrderSafely(int orderId);
    void handleProductEditSave(const QLineEdit* nameEdit, const QLineEdit* priceEdit, const QLineEdit* stockEdit, const std::string& oldName, QDialog* editDialog);

public:
    void refreshTable();
    explicit MainWindow(OrderService& svc, ProductService& productSvc, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;
};
