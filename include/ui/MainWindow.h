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
#include "include/services/OrderService.h"
#include "include/services/ProductService.h"
#include "include/utils/validation_utils.h"

class ProductWindow;
class StatisticsWindow;
class QDateTimeEdit;
class QCheckBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
private:
    OrderService& svc_;
    ProductService& productSvc_;
    ValidationService V_;

    QTableWidget* table_;
    QPushButton* addOrderBtn_;
    QPushButton* reportBtn_;
    QPushButton* productsBtn_;

    QPushButton* clearFilterBtn_;
    QLabel* titleLabel_;
    
    // Statistics elements
    QLabel* statsNewLabel_;
    QLabel* statsInProgressLabel_;
    QLabel* statsDoneLabel_;
    QLabel* statsCanceledLabel_;
    QLabel* statsTotalRevenueLabel_;
    QPushButton* openChartsBtn_;

    // Filter elements
    QLineEdit* clientFilterEdit_;
    QComboBox* statusFilterCombo_;
    QLineEdit* minTotalEdit_;
    QLineEdit* maxTotalEdit_;
    QLineEdit* minIdEdit_;
    QLineEdit* maxIdEdit_;
    QCheckBox* useFromCheck_;
    QDateTimeEdit* fromDateEdit_;
    QCheckBox* useToCheck_;
    QDateTimeEdit* toDateEdit_;

    ProductWindow* productWindow_;
    StatisticsWindow* statisticsWindow_;
    QCompleter* clientFilterCompleter_;
    QStringListModel* clientFilterModel_;

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

    QList<const Order*> currentFilteredRows() const;
    static QString sanitizedBaseName(const QString& raw);
    void applyFilters();
    void setupCompleters();

private slots:
    void onAddOrder();
    void onOpenReportDialog();
    void onOpenProducts();
    void onOpenStatistics();
    void onClearFilter();
    void onFilterChanged();
    void updateStatistics();

public:
    void refreshTable();
    explicit MainWindow(OrderService& svc, ProductService& productSvc, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;
};
