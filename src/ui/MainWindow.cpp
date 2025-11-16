#include "include/ui/UtilsQt.h"
#include "include/ui/MainWindow.h"
#include "include/ui/StatisticsWindow.h"
#include "include/ui/NumericItem.h"
#include "include/ui/AddOrderDialog.h"
#include "include/ui/EditOrderDialog.h"
#include "include/ui/ReportDialog.h"
#include "include/ui/AddProductDialog.h"
#include "include/core/Product.h"
#include "include/core/Order.h"
#include "include/Errors/CustomExceptions.h"
#include "include/services/ReportService.h"
#include <QVBoxLayout>
#include <QIntValidator>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <vector>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QHeaderView>
#include <QTimer>
#include <QResizeEvent>
#include <QLabel>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QInputDialog>
#include <QCoreApplication>
#include <QDateTimeEdit>
#include <QCheckBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QFormLayout>
#include <QStringConverter>
#include <QFrame>
#include <QSet>
#include <QStringListModel>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string_view>

MainWindow::MainWindow(OrderService& svc, ProductService& productSvc, QWidget* parent)
    : QMainWindow(parent), svc_(svc), productSvc_(productSvc) {
    setWindowTitle("Order Management System");
    
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    
    tabs_ = new QTabWidget(this);
    root->addWidget(tabs_);
    
    auto* ordersTab = new QWidget(this);
    auto* ordersLayout = new QHBoxLayout(ordersTab);

    auto* left = new QVBoxLayout();

    auto* topRow = new QHBoxLayout();
    titleLabel_ = new QLabel("Main Table", this);
    titleLabel_->setStyleSheet("font-weight: bold; font-size: 16px;");
    
    topRow->addWidget(titleLabel_);
    topRow->addStretch();
    left->addLayout(topRow);

    table_ = new QTableWidget(this);
    table_->setColumnCount(7);
    table_->setHorizontalHeaderLabels({"ID","Client","Items","Status","Total","Created At",""});
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Fixed);
    table_->setColumnWidth(6, 40);
    table_->setWordWrap(true);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionMode(QAbstractItemView::NoSelection);
    table_->setSortingEnabled(true);
    left->addWidget(table_);

    auto* actionRow = new QHBoxLayout();
    addOrderBtn_ = new QPushButton("Add order", this);
    reportBtn_ = new QPushButton("Report", this);
    actionRow->addWidget(addOrderBtn_);
    actionRow->addWidget(reportBtn_);
    actionRow->addStretch();
    left->addLayout(actionRow);

    auto* right = new QVBoxLayout();
    auto* filterLabel = new QLabel("Filters", this);
    filterLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    right->addWidget(filterLabel);

    auto* filterForm = new QFormLayout();
    
    filterWidgets_.clientEdit_ = new QLineEdit(this);
    filterWidgets_.clientEdit_->setPlaceholderText("Enter client name");
    filterForm->addRow("Client:", filterWidgets_.clientEdit_);

    filterWidgets_.statusCombo_ = new QComboBox(this);
    filterWidgets_.statusCombo_->addItems({"Any", "new", "in_progress", "done", "canceled"});
    filterForm->addRow("Status:", filterWidgets_.statusCombo_);

    QRegularExpression intRe("^[0-9]*$");
    QRegularExpression dblRe("^[0-9]+([\\.,][0-9]+)?$");

    filterWidgets_.minTotalEdit_ = new QLineEdit(this);
    filterWidgets_.minTotalEdit_->setPlaceholderText("min total");
    filterWidgets_.minTotalEdit_->setValidator(new QRegularExpressionValidator(dblRe, this));
    filterForm->addRow("Min total:", filterWidgets_.minTotalEdit_);

    filterWidgets_.maxTotalEdit_ = new QLineEdit(this);
    filterWidgets_.maxTotalEdit_->setPlaceholderText("max total");
    filterWidgets_.maxTotalEdit_->setValidator(new QRegularExpressionValidator(dblRe, this));
    filterForm->addRow("Max total:", filterWidgets_.maxTotalEdit_);

    filterWidgets_.minIdEdit_ = new QLineEdit(this);
    filterWidgets_.minIdEdit_->setPlaceholderText("min id");
    filterWidgets_.minIdEdit_->setValidator(new QRegularExpressionValidator(intRe, this));
    filterForm->addRow("Min ID:", filterWidgets_.minIdEdit_);

    filterWidgets_.maxIdEdit_ = new QLineEdit(this);
    filterWidgets_.maxIdEdit_->setPlaceholderText("max id");
    filterWidgets_.maxIdEdit_->setValidator(new QRegularExpressionValidator(intRe, this));
    filterForm->addRow("Max ID:", filterWidgets_.maxIdEdit_);

    filterWidgets_.useFromCheck_ = new QCheckBox("From date:", this);
    filterWidgets_.fromDateEdit_ = new QDateTimeEdit(this);
    filterWidgets_.fromDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    filterWidgets_.fromDateEdit_->setCalendarPopup(true);
    filterWidgets_.fromDateEdit_->setDateTime(QDateTime::currentDateTime());
    filterWidgets_.fromDateEdit_->setEnabled(false);
    auto* fromLayout = new QHBoxLayout();
    fromLayout->addWidget(filterWidgets_.useFromCheck_);
    fromLayout->addWidget(filterWidgets_.fromDateEdit_);
    filterForm->addRow(fromLayout);

    filterWidgets_.useToCheck_ = new QCheckBox("To date:", this);
    filterWidgets_.toDateEdit_ = new QDateTimeEdit(this);
    filterWidgets_.toDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    filterWidgets_.toDateEdit_->setCalendarPopup(true);
    filterWidgets_.toDateEdit_->setDateTime(QDateTime::currentDateTime());
    filterWidgets_.toDateEdit_->setEnabled(false);
    auto* toLayout = new QHBoxLayout();
    toLayout->addWidget(filterWidgets_.useToCheck_);
    toLayout->addWidget(filterWidgets_.toDateEdit_);
    filterForm->addRow(toLayout);

    right->addLayout(filterForm);
    
    clearFilterBtn_ = new QPushButton("Clear all filters", this);
    clearFilterBtn_->setEnabled(false);
    clearFilterBtn_->setStyleSheet(
        "QPushButton:disabled {"
        "background-color: #9E9E9E;"
        "color: #E0E0E0;"
        "border: none;"
        "padding: 5px;"
        "border-radius: 3px;"
        "}"
    );
    right->addWidget(clearFilterBtn_);
    
    right->addSpacing(20);
    
    auto* statsLabel = new QLabel("Statistics", this);
    statsLabel->setStyleSheet("font-weight: bold; font-size: 14px; margin-top: 10px;");
    right->addWidget(statsLabel);
    
    auto* statsLayout = new QVBoxLayout();
    orderStats_.newLabel_ = new QLabel("New: 0", this);
    orderStats_.inProgressLabel_ = new QLabel("In Progress: 0", this);
    orderStats_.doneLabel_ = new QLabel("Done: 0", this);
    orderStats_.canceledLabel_ = new QLabel("Canceled: 0", this);
    orderStats_.totalRevenueLabel_ = new QLabel("Total Revenue: $0.00", this);
    orderStats_.totalRevenueLabel_->setStyleSheet("font-weight: bold; margin-top: 5px;");
    
    statsLayout->addWidget(orderStats_.newLabel_);
    statsLayout->addWidget(orderStats_.inProgressLabel_);
    statsLayout->addWidget(orderStats_.doneLabel_);
    statsLayout->addWidget(orderStats_.canceledLabel_);
    statsLayout->addWidget(orderStats_.totalRevenueLabel_);
    
    openChartsBtn_ = new QPushButton("Open Charts", this);
    statsLayout->addWidget(openChartsBtn_);
    
    right->addLayout(statsLayout);
    right->addStretch();

    ordersLayout->addLayout(left, 3);
    ordersLayout->addLayout(right, 1);
    tabs_->addTab(ordersTab, "Orders");
    
    auto* productsTab = new QWidget(this);
    auto* productsLayout = new QHBoxLayout(productsTab);
    
    auto* productsLeft = new QVBoxLayout();
    productTable_ = new QTableWidget(this);
    productTable_->setColumnCount(5);
    productTable_->setHorizontalHeaderLabels({"Product","Price","Stock","",""});
    productTable_->horizontalHeader()->setStretchLastSection(false);
    productTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    productTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    productTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    productTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    productTable_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    productTable_->setColumnWidth(3, 40);
    productTable_->setColumnWidth(4, 40);
    productTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    productTable_->setSelectionMode(QAbstractItemView::NoSelection);
    productTable_->setSortingEnabled(true);
    productsLeft->addWidget(productTable_);
    
    auto* prodButtons = new QHBoxLayout();
    addProductBtn_ = new QPushButton("Add product", this);
    prodButtons->addWidget(addProductBtn_);
    prodButtons->addStretch();
    productsLeft->addLayout(prodButtons);
    
    auto* productsRight = new QVBoxLayout();
    auto* productStatsLabel = new QLabel("Product Statistics", this);
    productStatsLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    productsRight->addWidget(productStatsLabel);
    
    productStats_.lowStockLabel_ = new QLabel("Low Stock (Top 3):", this);
    productStats_.highStockLabel_ = new QLabel("High Stock (Top 3):", this);
    productStats_.expensiveLabel_ = new QLabel("Most Expensive (Top 3):", this);
    productStats_.cheapLabel_ = new QLabel("Cheapest (Top 3):", this);
    productStats_.totalCountLabel_ = new QLabel("Total Products: 0", this);
    productStats_.totalValueLabel_ = new QLabel("Total Value: $0.00", this);
    
    productsRight->addWidget(productStats_.lowStockLabel_);
    productsRight->addWidget(productStats_.highStockLabel_);
    productsRight->addWidget(productStats_.expensiveLabel_);
    productsRight->addWidget(productStats_.cheapLabel_);
    productsRight->addSpacing(20);
    productsRight->addWidget(productStats_.totalCountLabel_);
    productsRight->addWidget(productStats_.totalValueLabel_);
    productsRight->addStretch();
    
    productsLayout->addLayout(productsLeft, 3);
    productsLayout->addLayout(productsRight, 1);
    tabs_->addTab(productsTab, "Products");
    
    setCentralWidget(central);

    connect(addOrderBtn_, &QPushButton::clicked, this, &MainWindow::onAddOrder);
    connect(reportBtn_, &QPushButton::clicked, this, &MainWindow::onOpenReportDialog);
    connect(openChartsBtn_, &QPushButton::clicked, this, &MainWindow::onOpenStatistics);
    connect(clearFilterBtn_, &QPushButton::clicked, this, &MainWindow::onClearFilter);
    connect(addProductBtn_, &QPushButton::clicked, this, &MainWindow::onAddProduct);

    connect(filterWidgets_.clientEdit_, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(filterWidgets_.statusCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterChanged);
    connect(filterWidgets_.minTotalEdit_, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(filterWidgets_.maxTotalEdit_, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(filterWidgets_.minIdEdit_, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(filterWidgets_.maxIdEdit_, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(filterWidgets_.useFromCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        filterWidgets_.fromDateEdit_->setEnabled(checked);
        onFilterChanged();
    });
    connect(filterWidgets_.useToCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        filterWidgets_.toDateEdit_->setEnabled(checked);
        onFilterChanged();
    });
    connect(filterWidgets_.fromDateEdit_, &QDateTimeEdit::dateTimeChanged, this, &MainWindow::onFilterChanged);
    connect(filterWidgets_.toDateEdit_, &QDateTimeEdit::dateTimeChanged, this, &MainWindow::onFilterChanged);

    setupCompleters();
    showMaximized();
    refreshTable();
    refreshProducts();
    updateStatistics();
    updateProductStatistics();
    QTimer::singleShot(0, this, [this] { resizeEvent(nullptr); });
}

bool MainWindow::matchesClientFilter(const Order& o) const {
    if (filterState_.activeClientFilter_.isEmpty()) return true;
    QString c = qs(o.client);
    return c.toLower().contains(filterState_.activeClientFilter_.toLower());
}

bool MainWindow::matchesStatusFilter(const Order& o) const {
    if (filterState_.activeStatusFilter_.isEmpty()) return true;
    return qs(o.status).compare(filterState_.activeStatusFilter_, Qt::CaseInsensitive) == 0;
}

bool MainWindow::matchesTotalFilter(const Order& o) const {
    if (!filterState_.minTotalText_.isEmpty()) {
        QString t = filterState_.minTotalText_;
        t.replace(',', '.');
        bool b = false;
        const double v = t.toDouble(&b);
        if (b && o.total < v) return false;
    }
    if (!filterState_.maxTotalText_.isEmpty()) {
        QString t = filterState_.maxTotalText_;
        t.replace(',', '.');
        bool b = false;
        const double v = t.toDouble(&b);
        if (b && o.total > v) return false;
    }
    return true;
}

bool MainWindow::matchesIdFilter(const Order& o) const {
    if (!filterState_.minIdText_.isEmpty()) {
        bool b = false;
        const int v = filterState_.minIdText_.toInt(&b);
        if (b && o.id < v) return false;
    }
    if (!filterState_.maxIdText_.isEmpty()) {
        bool b = false;
        const int v = filterState_.maxIdText_.toInt(&b);
        if (b && o.id > v) return false;
    }
    return true;
}

bool MainWindow::matchesDateFilter(const Order& o) const {
    if (!filterState_.useFrom_ && !filterState_.useTo_) return true;
    QDateTime created = QDateTime::fromString(qs(o.createdAt), Qt::ISODate);
    if (filterState_.useFrom_ && created < filterState_.fromDate_) return false;
    if (filterState_.useTo_ && created > filterState_.toDate_) return false;
    return true;
}

QList<const Order*> MainWindow::currentFilteredRows() const {
    const auto& all = svc_.all();
    QList<const Order*> rows;
    for (const auto& o : all) {
        if (matchesClientFilter(o) && matchesStatusFilter(o) && matchesTotalFilter(o) && 
            matchesIdFilter(o) && matchesDateFilter(o)) {
            rows.push_back(&o);
        }
    }
    return rows;
}


void MainWindow::setupEmptyTableRow() {
    table_->setRowCount(1);
    table_->setSpan(0, 0, 1, table_->columnCount());
    auto* item = new QTableWidgetItem("Not found");
    item->setTextAlignment(Qt::AlignCenter);
    QFont f = item->font();
    f.setItalic(true);
    item->setFont(f);
    table_->setItem(0, 0, item);
}

QString MainWindow::formatOrderItems(const Order& o) const {
    QString itemsStr;
    bool first = true;
    for (const auto& [itemKey, qty] : o.items) {
        const auto pit = svc_.price().find(itemKey);
        const QString priceText = (pit != svc_.price().end())
            ? QString::number(pit->second, 'f', 2)
            : QString("n/a");
        if (!first) itemsStr += "\n";
        itemsStr += QString("%1 ×%2 (%3)")
            .arg(qs(itemKey))
            .arg(qty)
            .arg(priceText);
        first = false;
    }
    return itemsStr;
}

QTableWidgetItem* MainWindow::createStatusCell(const Order& o) const {
    auto* statusCell = new QTableWidgetItem(qs(o.status));
    statusCell->setTextAlignment(Qt::AlignCenter);
    if (o.status == "new") {
        statusCell->setBackground(QColor("#388E3C"));
        statusCell->setForeground(QBrush(Qt::white));
    } else if (o.status == "in_progress") {
        statusCell->setBackground(QColor("#FBC02D"));
        statusCell->setForeground(QBrush(Qt::black));
    } else if (o.status == "done") {
        statusCell->setBackground(QColor("#1976D2"));
        statusCell->setForeground(QBrush(Qt::white));
    } else if (o.status == "canceled") {
        statusCell->setBackground(QColor("#D32F2F"));
        statusCell->setForeground(QBrush(Qt::white));
    }
    return statusCell;
}

void MainWindow::populateTableRow(int row, const Order& o) {
    auto* idCell = new NumericItem(o.id, QString::number(o.id));
    idCell->setTextAlignment(Qt::AlignCenter);
    
    auto* clientCell = new QTableWidgetItem(qs(o.client));
    clientCell->setTextAlignment(Qt::AlignCenter);
    
    const QString itemsStr = formatOrderItems(o);
    auto* itemCell = new QTableWidgetItem(itemsStr);
    itemCell->setTextAlignment(Qt::AlignCenter);
    
    auto* statusCell = createStatusCell(o);
    
    auto* totalCell = new NumericItem(o.total, QString::number(o.total, 'f', 2));
    totalCell->setTextAlignment(Qt::AlignCenter);
    
    QString createdAtStr = qs(o.createdAt);
    createdAtStr.replace("T", " ");
    auto* createdCell = new QTableWidgetItem(createdAtStr);
    createdCell->setTextAlignment(Qt::AlignCenter);
    
    auto* editBtn = createEditButton(this, "Edit order");
    connect(editBtn, &QPushButton::clicked, this, [this, orderId = o.id]() {
        if (const Order* order = svc_.findById(orderId); !order) {
            QMessageBox::warning(this, "error", "order not found");
            return;
        }
        EditOrderDialog editDlg(svc_, orderId, this);
        editDlg.setProductService(&productSvc_);
        connect(&editDlg, &EditOrderDialog::dataChanged, this, &MainWindow::refreshTable);
        editDlg.exec();
        refreshTable();
        updateStatistics();
    });
    
    auto* widgetContainer = new QWidget(this);
    auto* layout = new QHBoxLayout(widgetContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch();
    layout->addWidget(editBtn);
    layout->addStretch();
    layout->setAlignment(Qt::AlignCenter);
    
    table_->setItem(row, 0, idCell);
    table_->setItem(row, 1, clientCell);
    table_->setItem(row, 2, itemCell);
    table_->setItem(row, 3, statusCell);
    table_->setItem(row, 4, totalCell);
    table_->setItem(row, 5, createdCell);
    table_->setCellWidget(row, 6, widgetContainer);
}

void MainWindow::refreshTable() {
    table_->setSortingEnabled(false);
    table_->clearSpans();
    table_->clearContents();

    const QList<const Order*> rows = currentFilteredRows();

    if (rows.isEmpty()) {
        setupEmptyTableRow();
    } else {
        table_->setRowCount(rows.size());
        int r = 0;
        for (const auto* op : rows) {
            populateTableRow(r, *op);
            ++r;
        }
        table_->resizeRowsToContents();
        for (int row = 0; row < table_->rowCount(); ++row) {
            const int h = table_->rowHeight(row);
            table_->setRowHeight(row, h + 8);
        }
    }

    bool filterActive = !filterState_.activeClientFilter_.isEmpty() || !filterState_.activeStatusFilter_.isEmpty()
                        || !filterState_.minTotalText_.isEmpty() || !filterState_.maxTotalText_.isEmpty()
                        || !filterState_.minIdText_.isEmpty() || !filterState_.maxIdText_.isEmpty()
                        || filterState_.useFrom_ || filterState_.useTo_;

    int foundCount = rows.size();
    if (filterActive) {
        titleLabel_->setText(QString("Filtered Table (%1 orders)").arg(foundCount));
        clearFilterBtn_->setEnabled(true);
        clearFilterBtn_->setStyleSheet(
            "QPushButton {"
            "background-color: #F44336;"
            "color: white;"
            "border: none;"
            "padding: 5px;"
            "border-radius: 3px;"
            "font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "background-color: #D32F2F;"
            "}"
            "QPushButton:pressed {"
            "background-color: #B71C1C;"
            "}"
        );
    } else {
        titleLabel_->setText(QString("Main Table (%1 orders)").arg((int)svc_.all().size()));
        clearFilterBtn_->setEnabled(false);
        clearFilterBtn_->setStyleSheet(
            "QPushButton:disabled {"
            "background-color: #9E9E9E;"
            "color: #E0E0E0;"
            "border: none;"
            "padding: 5px;"
            "border-radius: 3px;"
            "}"
        );
    }

    table_->setSortingEnabled(!rows.isEmpty());
    titleLabel_->update();
    updateStatistics();
    
    if (statisticsWindow_ && statisticsWindow_->isVisible()) {
        statisticsWindow_->refreshStatistics();
    }
    
    setupCompleters();
}


void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (table_) {
        table_->resizeRowsToContents();
        for (int row = 0; row < table_->rowCount(); ++row) {
            int h = table_->rowHeight(row);
            table_->setRowHeight(row, h + 8);
        }
    }
}


void MainWindow::onAddOrder() {
    AddOrderDialog dlg(svc_, this);
    if (dlg.exec() == QDialog::Accepted) {
        int createdId = dlg.createdOrderId();
        refreshTable();
        EditOrderDialog editDlg(svc_, createdId, this);
        editDlg.setProductService(&productSvc_);
        connect(&editDlg, &EditOrderDialog::dataChanged, this, &MainWindow::refreshTable);
        editDlg.exec();
        refreshTable();
        updateStatistics();
    }
}


void MainWindow::onOpenReportDialog() {
    bool filterActive = !filterState_.activeClientFilter_.isEmpty() || !filterState_.activeStatusFilter_.isEmpty()
                        || !filterState_.minTotalText_.isEmpty() || !filterState_.maxTotalText_.isEmpty()
                        || !filterState_.minIdText_.isEmpty() || !filterState_.maxIdText_.isEmpty()
                        || filterState_.useFrom_ || filterState_.useTo_;
    ReportDialog dlg(filterActive, this);
    if (dlg.exec() != QDialog::Accepted) return;

    QList<const Order*> rows = dlg.scopeFiltered() ? currentFilteredRows() : QList<const Order*>{};
    if (!dlg.scopeFiltered()) {
        for (const auto& o : svc_.all()) rows.push_back(&o);
    }
    if (rows.isEmpty()) {
        QMessageBox::information(this, "report", "nothing to report");
        return;
    }

    ReportFilterInfo filterInfo;
    filterInfo.clientFilter = filterState_.activeClientFilter_;
    filterInfo.statusFilter = filterState_.activeStatusFilter_;
    filterInfo.minTotal = filterState_.minTotalText_;
    filterInfo.maxTotal = filterState_.maxTotalText_;
    filterInfo.minId = filterState_.minIdText_;
    filterInfo.maxId = filterState_.maxIdText_;
    filterInfo.fromDate = filterState_.fromDate_;
    filterInfo.toDate = filterState_.toDate_;
    filterInfo.useFrom = filterState_.useFrom_;
    filterInfo.useTo = filterState_.useTo_;

    QString fileName = ReportService::generateReport(
        rows,
        dlg.reportName(),
        dlg.scopeFiltered(),
        dlg.includeFiltersHeader(),
        dlg.includeSummarySection(),
        filterInfo,
        svc_
    );

    if (fileName.isEmpty()) {
        QMessageBox::warning(this, "error", "cannot create report file");
        return;
    }

    QMessageBox::information(this, "report", QString("Excel report created: %1").arg(fileName));
}


void MainWindow::refreshProducts() {
    productTable_->setSortingEnabled(false);
    const auto& p = productSvc_.all();
    productTable_->clearContents();
    productTable_->setRowCount((int)p.size());
    int i = 0;
    for (const auto& [productKey, prod] : p) {
        auto rowData = createProductTableRow(productTable_, i, prod, this, false);
        connect(rowData.editBtn, &QPushButton::clicked, this, [this, productKey, productName = prod.name]() {
            onEditProduct(productKey, productName);
        });
        connect(rowData.deleteBtn, &QPushButton::clicked, this, [this, productKey, productName = prod.name]() {
            onDeleteProduct(productKey, productName);
        });
        ++i;
    }
    productTable_->setSortingEnabled(true);
    
    updateProductStatistics();
}

void MainWindow::onAddProduct() {
    AddProductDialog dlg(productSvc_, this);
    if (dlg.exec() == QDialog::Accepted) {
        if (std::string addedName = dlg.addedProductName(); !addedName.empty()) {
            svc_.setPrices(productSvc_.all());
            std::string key = addedName;
            std::ranges::transform(key, key.begin(), ::tolower);
            svc_.recalculateOrdersWithProduct(key);
            svc_.save();
            refreshTable();
        }
        refreshProducts();
    }
}

bool MainWindow::isProductUsedInActiveOrders(const std::string& productKey, QList<int>& affectedOrderIds) const {
    affectedOrderIds.clear();
    const auto& orders = svc_.all();
    for (const auto& order : orders) {
        if ((order.status == "new" || order.status == "in_progress") && order.items.contains(productKey)) {
            affectedOrderIds.append(order.id);
        }
    }
    return !affectedOrderIds.isEmpty();
}

void MainWindow::cancelOrderSafely(int orderId) {
    Order* order = svc_.findById(orderId);
    if (!order) {
        return;
    }
    try {
        svc_.setStatus(*order, "canceled");
    } catch (const CustomException& e) {
        QMessageBox::warning(this, "error", QString("Failed to cancel order %1: %2")
                            .arg(orderId)
                            .arg(qs(e.what())));
    }
}

void MainWindow::onDeleteProduct(const std::string& productKey, const std::string& productName) {
    try {
        QList<int> affectedOrderIds;
        bool isUsed = isProductUsedInActiveOrders(productKey, affectedOrderIds);
        
        if (isUsed) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Delete Product");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(QString("Product '%1' is used in %2 active order(s) (new or in_progress).")
                          .arg(qs(productName))
                          .arg(affectedOrderIds.size()));
            msgBox.setInformativeText("What would you like to do?");
            
            QPushButton* cancelBtn = msgBox.addButton("Cancel", QMessageBox::RejectRole);
            msgBox.addButton("Cancel All Orders", QMessageBox::AcceptRole);
            
            msgBox.setDefaultButton(cancelBtn);
            msgBox.exec();
            
            if (msgBox.clickedButton() == cancelBtn) {
                return;
            }
            
            svc_.setProductService(&productSvc_);
            
            for (int orderId : affectedOrderIds) {
                cancelOrderSafely(orderId);
            }
            svc_.save();
        }
        
        productSvc_.removeProduct(productName);
        productSvc_.save();
        svc_.setPrices(productSvc_.all());
        svc_.save();
        refreshProducts();
        refreshTable();
        
        if (isUsed) {
            QMessageBox::information(this, "ok", QString("Product removed. %1 order(s) were canceled.")
                                    .arg(affectedOrderIds.size()));
        } else {
            QMessageBox::information(this, "ok", "product removed");
        }
    } catch (const CustomException& e) { 
        QMessageBox::warning(this, "error", qs(e.what())); 
    }
}

void MainWindow::onEditProduct([[maybe_unused]] std::string_view productKey, std::string_view productName) {
    const Product* product = productSvc_.findProduct(std::string(productName));
    if (!product) {
        QMessageBox::warning(this, "error", "product not found");
        return;
    }
    
    auto* editDialog = new QDialog(this);
    editDialog->setWindowTitle("Edit Product");
    editDialog->setModal(true);
    
    auto fields = createProductEditDialogFields(editDialog, qs(product->name), product->price, product->stock);
    
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, editDialog);
    buttons->button(QDialogButtonBox::Ok)->setText("Save");
    qobject_cast<QVBoxLayout*>(editDialog->layout())->addWidget(buttons);
    
    connect(buttons, &QDialogButtonBox::accepted, editDialog, [this, editDialog, nameEdit = fields.nameEdit, priceEdit = fields.priceEdit, stockEdit = fields.stockEdit, oldName = std::string(productName)]() {
        handleProductEditSave(nameEdit, priceEdit, stockEdit, oldName, editDialog);
    });
    
    connect(buttons, &QDialogButtonBox::rejected, editDialog, &QDialog::reject);
    
    editDialog->resize(400, 150);
    editDialog->exec();
    delete editDialog;
}

void MainWindow::handleProductEditSave(const QLineEdit* nameEdit, const QLineEdit* priceEdit, const QLineEdit* stockEdit, const std::string& oldName, QDialog* editDialog) {
    try {
        auto validation = validateProductEditInputs(nameEdit, priceEdit, stockEdit);
        if (!validation.isValid) {
            QMessageBox::warning(editDialog, "error", validation.errorMessage);
            return;
        }
        
        double oldPrice = 0.0;
        const Product* oldProduct = productSvc_.findProduct(oldName);
        if (oldProduct) {
            oldPrice = oldProduct->price;
        }
        
        productSvc_.updateProduct(oldName, validation.newName, validation.price, validation.stock);
        productSvc_.save();
        
        svc_.setPrices(productSvc_.all());
        bool priceChanged = (oldProduct && std::abs(oldPrice - validation.price) > 0.01);
        
        if (bool nameChanged = (oldName != validation.newName); priceChanged || nameChanged) {
            if (nameChanged) {
                std::string oldKey = oldName;
                std::ranges::transform(oldKey, oldKey.begin(), ::tolower);
                svc_.recalculateOrdersWithProduct(oldKey);
            }
            std::string newKey = validation.newName;
            std::ranges::transform(newKey, newKey.begin(), ::tolower);
            svc_.recalculateOrdersWithProduct(newKey);
        }
        svc_.save();
        
        refreshProducts();
        refreshTable();
        
        editDialog->accept();
        QMessageBox::information(this, "ok", "product updated");
    } catch (const CustomException& e) {
        QMessageBox::warning(editDialog, "error", qs(e.what()));
    }
}

void MainWindow::updateProductStatistics() {
    const auto& products = productSvc_.all();
    
    std::vector<std::pair<std::string, Product>> productsVec;
    for (const auto& [key, product] : products) {
        productsVec.emplace_back(key, product);
    }
    
    std::ranges::sort(productsVec, 
        [](const auto& a, const auto& b) { return a.second.stock < b.second.stock; });
    
    QString lowStock = "Low Stock (Top 3):\n";
    for (size_t i = 0; i < std::min(3UL, productsVec.size()); ++i) {
        lowStock += QString("  • %1: %2\n").arg(qs(productsVec[i].second.name)).arg(productsVec[i].second.stock);
    }
    productStats_.lowStockLabel_->setText(lowStock);
    
    std::ranges::sort(productsVec, 
        [](const auto& a, const auto& b) { return a.second.stock > b.second.stock; });
    
    QString highStock = "High Stock (Top 3):\n";
    for (size_t i = 0; i < std::min(3UL, productsVec.size()); ++i) {
        highStock += QString("  • %1: %2\n").arg(qs(productsVec[i].second.name)).arg(productsVec[i].second.stock);
    }
    productStats_.highStockLabel_->setText(highStock);
    
    std::ranges::sort(productsVec, 
        [](const auto& a, const auto& b) { return a.second.price > b.second.price; });
    
    QString expensive = "Most Expensive (Top 3):\n";
    for (size_t i = 0; i < std::min(3UL, productsVec.size()); ++i) {
        expensive += QString("  • %1: $%2\n").arg(qs(productsVec[i].second.name))
                    .arg(QString::number(productsVec[i].second.price, 'f', 2));
    }
    productStats_.expensiveLabel_->setText(expensive);
    
    std::ranges::sort(productsVec, 
        [](const auto& a, const auto& b) { return a.second.price < b.second.price; });
    
    QString cheap = "Cheapest (Top 3):\n";
    for (size_t i = 0; i < std::min(3UL, productsVec.size()); ++i) {
        cheap += QString("  • %1: $%2\n").arg(qs(productsVec[i].second.name))
                .arg(QString::number(productsVec[i].second.price, 'f', 2));
    }
    productStats_.cheapLabel_->setText(cheap);
    
    int totalCount = products.size();
    double totalValue = 0.0;
    for (const auto& [key, product] : products) {
        (void)key; // unused
        totalValue += product.price * product.stock;
    }
    
    productStats_.totalCountLabel_->setText(QString("Total Products: %1").arg(totalCount));
    productStats_.totalValueLabel_->setText(QString("Total Value: $%1").arg(QString::number(totalValue, 'f', 2)));
}

void MainWindow::onOpenStatistics() {
    if (!statisticsWindow_) {
        statisticsWindow_ = new StatisticsWindow(svc_, this);
    }
    statisticsWindow_->show();
    statisticsWindow_->raise();
    statisticsWindow_->activateWindow();
}

void MainWindow::updateStatistics() {
    int newCount = 0;
    int inProgressCount = 0;
    int doneCount = 0;
    int canceledCount = 0;
    double totalRevenue = 0.0;
    
    const auto& all = svc_.all();
    for (const auto& o : all) {
        if (o.status == "new") newCount++;
        else if (o.status == "in_progress") inProgressCount++;
        else if (o.status == "done") doneCount++;
        else if (o.status == "canceled") canceledCount++;
        totalRevenue += o.total;
    }
    
    orderStats_.newLabel_->setText(QString("New: %1").arg(newCount));
    orderStats_.inProgressLabel_->setText(QString("In Progress: %1").arg(inProgressCount));
    orderStats_.doneLabel_->setText(QString("Done: %1").arg(doneCount));
    orderStats_.canceledLabel_->setText(QString("Canceled: %1").arg(canceledCount));
    orderStats_.totalRevenueLabel_->setText(QString("Total Revenue: $%1").arg(QString::number(totalRevenue, 'f', 2)));
}

void MainWindow::applyFilters() {
    filterState_.activeClientFilter_ = filterWidgets_.clientEdit_->text().trimmed();
    filterState_.activeStatusFilter_ = filterWidgets_.statusCombo_->currentIndex() == 0 ? QString() : filterWidgets_.statusCombo_->currentText();
    filterState_.minTotalText_ = filterWidgets_.minTotalEdit_->text().trimmed();
    filterState_.maxTotalText_ = filterWidgets_.maxTotalEdit_->text().trimmed();
    filterState_.minIdText_ = filterWidgets_.minIdEdit_->text().trimmed();
    filterState_.maxIdText_ = filterWidgets_.maxIdEdit_->text().trimmed();
    filterState_.useFrom_ = filterWidgets_.useFromCheck_->isChecked();
    filterState_.useTo_ = filterWidgets_.useToCheck_->isChecked();
    filterState_.fromDate_ = filterWidgets_.fromDateEdit_->dateTime();
    filterState_.toDate_ = filterWidgets_.toDateEdit_->dateTime();
    refreshTable();
}

void MainWindow::onFilterChanged() {
    applyFilters();
}

void MainWindow::onClearFilter() {
    filterWidgets_.clientEdit_->clear();
    filterWidgets_.statusCombo_->setCurrentIndex(0);
    filterWidgets_.minTotalEdit_->clear();
    filterWidgets_.maxTotalEdit_->clear();
    filterWidgets_.minIdEdit_->clear();
    filterWidgets_.maxIdEdit_->clear();
    filterWidgets_.useFromCheck_->setChecked(false);
    filterWidgets_.useToCheck_->setChecked(false);
    filterWidgets_.fromDateEdit_->setDateTime(QDateTime::currentDateTime());
    filterWidgets_.toDateEdit_->setDateTime(QDateTime::currentDateTime());
    applyFilters();
}

void MainWindow::setupCompleters() {
    QSet<QString> clientSet;
    const auto& orders = svc_.all();
    for (const auto& order : orders) {
        clientSet.insert(qs(order.client));
    }
    
    QStringList clientNames = clientSet.values();
    clientNames.sort(Qt::CaseInsensitive);
    
    if (!clientFilterModel_) {
        clientFilterModel_ = new QStringListModel(clientNames, this);
        clientFilterCompleter_ = new QCompleter(clientFilterModel_, this);
        clientFilterCompleter_->setCaseSensitivity(Qt::CaseInsensitive);
        clientFilterCompleter_->setCompletionMode(QCompleter::PopupCompletion);
        clientFilterCompleter_->setFilterMode(Qt::MatchContains);
        filterWidgets_.clientEdit_->setCompleter(clientFilterCompleter_);
    } else {
        clientFilterModel_->setStringList(clientNames);
    }
}
