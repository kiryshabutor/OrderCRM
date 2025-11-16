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
    
    clientFilterEdit_ = new QLineEdit(this);
    clientFilterEdit_->setPlaceholderText("Enter client name");
    filterForm->addRow("Client:", clientFilterEdit_);

    statusFilterCombo_ = new QComboBox(this);
    statusFilterCombo_->addItems({"Any", "new", "in_progress", "done", "canceled"});
    filterForm->addRow("Status:", statusFilterCombo_);

    QRegularExpression intRe("^[0-9]*$");
    QRegularExpression dblRe("^[0-9]+([\\.,][0-9]+)?$");

    minTotalEdit_ = new QLineEdit(this);
    minTotalEdit_->setPlaceholderText("min total");
    minTotalEdit_->setValidator(new QRegularExpressionValidator(dblRe, this));
    filterForm->addRow("Min total:", minTotalEdit_);

    maxTotalEdit_ = new QLineEdit(this);
    maxTotalEdit_->setPlaceholderText("max total");
    maxTotalEdit_->setValidator(new QRegularExpressionValidator(dblRe, this));
    filterForm->addRow("Max total:", maxTotalEdit_);

    minIdEdit_ = new QLineEdit(this);
    minIdEdit_->setPlaceholderText("min id");
    minIdEdit_->setValidator(new QRegularExpressionValidator(intRe, this));
    filterForm->addRow("Min ID:", minIdEdit_);

    maxIdEdit_ = new QLineEdit(this);
    maxIdEdit_->setPlaceholderText("max id");
    maxIdEdit_->setValidator(new QRegularExpressionValidator(intRe, this));
    filterForm->addRow("Max ID:", maxIdEdit_);

    useFromCheck_ = new QCheckBox("From date:", this);
    fromDateEdit_ = new QDateTimeEdit(this);
    fromDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    fromDateEdit_->setCalendarPopup(true);
    fromDateEdit_->setDateTime(QDateTime::currentDateTime());
    fromDateEdit_->setEnabled(false);
    auto* fromLayout = new QHBoxLayout();
    fromLayout->addWidget(useFromCheck_);
    fromLayout->addWidget(fromDateEdit_);
    filterForm->addRow(fromLayout);

    useToCheck_ = new QCheckBox("To date:", this);
    toDateEdit_ = new QDateTimeEdit(this);
    toDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    toDateEdit_->setCalendarPopup(true);
    toDateEdit_->setDateTime(QDateTime::currentDateTime());
    toDateEdit_->setEnabled(false);
    auto* toLayout = new QHBoxLayout();
    toLayout->addWidget(useToCheck_);
    toLayout->addWidget(toDateEdit_);
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
    statsNewLabel_ = new QLabel("New: 0", this);
    statsInProgressLabel_ = new QLabel("In Progress: 0", this);
    statsDoneLabel_ = new QLabel("Done: 0", this);
    statsCanceledLabel_ = new QLabel("Canceled: 0", this);
    statsTotalRevenueLabel_ = new QLabel("Total Revenue: $0.00", this);
    statsTotalRevenueLabel_->setStyleSheet("font-weight: bold; margin-top: 5px;");
    
    statsLayout->addWidget(statsNewLabel_);
    statsLayout->addWidget(statsInProgressLabel_);
    statsLayout->addWidget(statsDoneLabel_);
    statsLayout->addWidget(statsCanceledLabel_);
    statsLayout->addWidget(statsTotalRevenueLabel_);
    
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
    
    productStatsLowStockLabel_ = new QLabel("Low Stock (Top 3):", this);
    productStatsHighStockLabel_ = new QLabel("High Stock (Top 3):", this);
    productStatsExpensiveLabel_ = new QLabel("Most Expensive (Top 3):", this);
    productStatsCheapLabel_ = new QLabel("Cheapest (Top 3):", this);
    productStatsTotalCountLabel_ = new QLabel("Total Products: 0", this);
    productStatsTotalValueLabel_ = new QLabel("Total Value: $0.00", this);
    
    productsRight->addWidget(productStatsLowStockLabel_);
    productsRight->addWidget(productStatsHighStockLabel_);
    productsRight->addWidget(productStatsExpensiveLabel_);
    productsRight->addWidget(productStatsCheapLabel_);
    productsRight->addSpacing(20);
    productsRight->addWidget(productStatsTotalCountLabel_);
    productsRight->addWidget(productStatsTotalValueLabel_);
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

    connect(clientFilterEdit_, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(statusFilterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onFilterChanged);
    connect(minTotalEdit_, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(maxTotalEdit_, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(minIdEdit_, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(maxIdEdit_, &QLineEdit::textChanged, this, &MainWindow::onFilterChanged);
    connect(useFromCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        fromDateEdit_->setEnabled(checked);
        onFilterChanged();
    });
    connect(useToCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        toDateEdit_->setEnabled(checked);
        onFilterChanged();
    });
    connect(fromDateEdit_, &QDateTimeEdit::dateTimeChanged, this, &MainWindow::onFilterChanged);
    connect(toDateEdit_, &QDateTimeEdit::dateTimeChanged, this, &MainWindow::onFilterChanged);

    setupCompleters();
    showMaximized();
    refreshTable();
    refreshProducts();
    updateStatistics();
    updateProductStatistics();
    QTimer::singleShot(0, this, [this] { resizeEvent(nullptr); });
}

QList<const Order*> MainWindow::currentFilteredRows() const {
    const auto& all = svc_.all();
    QList<const Order*> rows;
    for (const auto& o : all) {
        bool ok = true;
        if (!activeClientFilter_.isEmpty()) {
            QString c = qs(o.client);
            ok = ok && c.toLower().contains(activeClientFilter_.toLower());
        }
        if (!activeStatusFilter_.isEmpty()) {
            ok = ok && (qs(o.status).compare(activeStatusFilter_, Qt::CaseInsensitive) == 0);
        }
        if (!minTotalText_.isEmpty()) {
            QString t = minTotalText_; t.replace(',', '.');
            bool b = false; double v = t.toDouble(&b);
            if (b) ok = ok && (o.total >= v);
        }
        if (!maxTotalText_.isEmpty()) {
            QString t = maxTotalText_; t.replace(',', '.');
            bool b = false; double v = t.toDouble(&b);
            if (b) ok = ok && (o.total <= v);
        }
        if (!minIdText_.isEmpty()) {
            bool b = false; int v = minIdText_.toInt(&b);
            if (b) ok = ok && (o.id >= v);
        }
        if (!maxIdText_.isEmpty()) {
            bool b = false; int v = maxIdText_.toInt(&b);
            if (b) ok = ok && (o.id <= v);
        }
        if (useFrom_ || useTo_) {
            QDateTime created = QDateTime::fromString(qs(o.createdAt), Qt::ISODate);
            if (useFrom_) ok = ok && (created >= fromDate_);
            if (useTo_) ok = ok && (created <= toDate_);
        }
        if (ok) rows.push_back(&o);
    }
    return rows;
}


void MainWindow::refreshTable() {
    table_->setSortingEnabled(false);
    table_->clearSpans();
    table_->clearContents();

    QList<const Order*> rows = currentFilteredRows();

    if (rows.isEmpty()) {
        table_->setRowCount(1);
        table_->setSpan(0, 0, 1, table_->columnCount());
        auto* item = new QTableWidgetItem("Not found");
        item->setTextAlignment(Qt::AlignCenter);
        QFont f = item->font();
        f.setItalic(true);
        item->setFont(f);
        table_->setItem(0, 0, item);
    } else {
        table_->setRowCount(rows.size());
        int r = 0;
        for (auto* op : rows) {
            const auto& o = *op;
            QString itemsStr;
            bool first = true;
            for (const auto& [itemKey, qty] : o.items) {
                auto pit = svc_.price().find(itemKey);
                QString priceText = (pit != svc_.price().end())
                    ? QString::number(pit->second, 'f', 2)
                    : QString("n/a");
                if (!first) itemsStr += "\n";
                itemsStr += QString("%1 ×%2 (%3)")
                    .arg(qs(itemKey))
                    .arg(qty)
                    .arg(priceText);
                first = false;
            }
            auto* idCell = new NumericItem(o.id, QString::number(o.id));
            idCell->setTextAlignment(Qt::AlignCenter);
            
            auto* clientCell = new QTableWidgetItem(qs(o.client));
            clientCell->setTextAlignment(Qt::AlignCenter);
            
            auto* itemCell = new QTableWidgetItem(itemsStr);
            itemCell->setTextAlignment(Qt::AlignCenter);
            
            auto* statusCell = new QTableWidgetItem(qs(o.status));
            statusCell->setTextAlignment(Qt::AlignCenter);
            if (o.status == "new") { statusCell->setBackground(QColor("#388E3C")); statusCell->setForeground(QBrush(Qt::white)); }
            else if (o.status == "in_progress") { statusCell->setBackground(QColor("#FBC02D")); statusCell->setForeground(QBrush(Qt::black)); }
            else if (o.status == "done") { statusCell->setBackground(QColor("#1976D2")); statusCell->setForeground(QBrush(Qt::white)); }
            else if (o.status == "canceled") { statusCell->setBackground(QColor("#D32F2F")); statusCell->setForeground(QBrush(Qt::white)); }
            
            auto* totalCell = new NumericItem(o.total, QString::number(o.total, 'f', 2));
            totalCell->setTextAlignment(Qt::AlignCenter);
            
            QString createdAtStr = qs(o.createdAt);
            createdAtStr.replace("T", " ");
            auto* createdCell = new QTableWidgetItem(createdAtStr);
            createdCell->setTextAlignment(Qt::AlignCenter);
            
            auto* editBtn = new QPushButton("⚙️", this);
            editBtn->setStyleSheet(
                "QPushButton {"
                "background-color: #2196F3;"
                "color: white;"
                "border: none;"
                "padding: 4px 8px;"
                "border-radius: 4px;"
                "font-size: 14px;"
                "}"
                "QPushButton:hover {"
                "background-color: #1976D2;"
                "}"
                "QPushButton:pressed {"
                "background-color: #0D47A1;"
                "}"
            );
            editBtn->setToolTip("Edit order");
            editBtn->setFixedSize(35, 25);
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
            
            table_->setItem(r, 0, idCell);
            table_->setItem(r, 1, clientCell);
            table_->setItem(r, 2, itemCell);
            table_->setItem(r, 3, statusCell);
            table_->setItem(r, 4, totalCell);
            table_->setItem(r, 5, createdCell);
            table_->setCellWidget(r, 6, widgetContainer);
            ++r;
        }
        table_->resizeRowsToContents();
        for (int row = 0; row < table_->rowCount(); ++row) {
            int h = table_->rowHeight(row);
            table_->setRowHeight(row, h + 8);
        }
    }

    bool filterActive = !activeClientFilter_.isEmpty() || !activeStatusFilter_.isEmpty()
                        || !minTotalText_.isEmpty() || !maxTotalText_.isEmpty()
                        || !minIdText_.isEmpty() || !maxIdText_.isEmpty()
                        || useFrom_ || useTo_;

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
    bool filterActive = !activeClientFilter_.isEmpty() || !activeStatusFilter_.isEmpty()
                        || !minTotalText_.isEmpty() || !maxTotalText_.isEmpty()
                        || !minIdText_.isEmpty() || !maxIdText_.isEmpty()
                        || useFrom_ || useTo_;
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
    filterInfo.clientFilter = activeClientFilter_;
    filterInfo.statusFilter = activeStatusFilter_;
    filterInfo.minTotal = minTotalText_;
    filterInfo.maxTotal = maxTotalText_;
    filterInfo.minId = minIdText_;
    filterInfo.maxId = maxIdText_;
    filterInfo.fromDate = fromDate_;
    filterInfo.toDate = toDate_;
    filterInfo.useFrom = useFrom_;
    filterInfo.useTo = useTo_;

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

static double parsePrice(const QString& input) {
    QString s = input.trimmed();
    if (s.isEmpty()) throw ValidationException("price cannot be empty");
    s.replace(',', '.');
    bool ok = false;
    double price = s.toDouble(&ok);
    if (!ok || price <= 0.0) throw ValidationException("invalid price");
    double cents = std::round(price * 100.0);
    if (std::fabs(price * 100.0 - cents) > 1e-9) throw ValidationException("price must have max 2 decimals");
    price = cents / 100.0;
    return price;
}

void MainWindow::refreshProducts() {
    productTable_->setSortingEnabled(false);
    const auto& p = productSvc_.all();
    productTable_->clearContents();
    productTable_->setRowCount((int)p.size());
    int i = 0;
    for (const auto& [productKey, prod] : p) {
        
        auto* nameCell = new QTableWidgetItem(qs(prod.name));
        nameCell->setTextAlignment(Qt::AlignCenter);
        auto* priceCell = new NumericItem(prod.price, QString::number(prod.price, 'f', 2));
        priceCell->setTextAlignment(Qt::AlignCenter);
        auto* stockCell = new QTableWidgetItem(QString::number(prod.stock));
        stockCell->setTextAlignment(Qt::AlignCenter);
        
        productTable_->setItem(i, 0, nameCell);
        productTable_->setItem(i, 1, priceCell);
        productTable_->setItem(i, 2, stockCell);
        
        auto* editBtn = new QPushButton("⚙️", this);
        editBtn->setStyleSheet(
            "QPushButton {"
            "background-color: #2196F3;"
            "color: white;"
            "border: none;"
            "padding: 4px 8px;"
            "border-radius: 4px;"
            "font-size: 14px;"
            "}"
            "QPushButton:hover {"
            "background-color: #1976D2;"
            "}"
            "QPushButton:pressed {"
            "background-color: #0D47A1;"
            "}"
        );
        editBtn->setToolTip("Edit product");
        editBtn->setFixedSize(35, 25);
        connect(editBtn, &QPushButton::clicked, this, [this, productKey, productName = prod.name]() {
            onEditProduct(productKey, productName);
        });
        
        auto* deleteBtn = new QPushButton("❌", this);
        deleteBtn->setStyleSheet(
            "QPushButton {"
            "background-color: #F44336;"
            "color: white;"
            "border: none;"
            "padding: 4px 8px;"
            "border-radius: 4px;"
            "font-size: 14px;"
            "}"
            "QPushButton:hover {"
            "background-color: #D32F2F;"
            "}"
            "QPushButton:pressed {"
            "background-color: #B71C1C;"
            "}"
        );
        deleteBtn->setToolTip("Delete product");
        deleteBtn->setFixedSize(35, 25);
        connect(deleteBtn, &QPushButton::clicked, this, [this, productKey, productName = prod.name]() {
            onDeleteProduct(productKey, productName);
        });
        
        auto* editWidget = new QWidget(this);
        auto* editLayout = new QHBoxLayout(editWidget);
        editLayout->setAlignment(Qt::AlignCenter);
        editLayout->setContentsMargins(0, 0, 0, 0);
        editLayout->addWidget(editBtn);
        productTable_->setCellWidget(i, 3, editWidget);
        
        auto* deleteWidget = new QWidget(this);
        auto* deleteLayout = new QHBoxLayout(deleteWidget);
        deleteLayout->setAlignment(Qt::AlignCenter);
        deleteLayout->setContentsMargins(0, 0, 0, 0);
        deleteLayout->addWidget(deleteBtn);
        productTable_->setCellWidget(i, 4, deleteWidget);
        
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

void MainWindow::onEditProduct(const std::string& productKey, const std::string& productName) {
    const Product* product = productSvc_.findProduct(productName);
    if (!product) {
        QMessageBox::warning(this, "error", "product not found");
        return;
    }
    
    QDialog* editDialog = new QDialog(this);
    editDialog->setWindowTitle("Edit Product");
    editDialog->setModal(true);
    
    auto* layout = new QVBoxLayout(editDialog);
    auto* form = new QFormLayout();
    
    auto* nameEdit = new QLineEdit(editDialog);
    nameEdit->setText(qs(product->name));
    form->addRow("Product name:", nameEdit);
    
    auto* priceEdit = new QLineEdit(editDialog);
    priceEdit->setText(QString::number(product->price, 'f', 2));
    form->addRow("Price:", priceEdit);
    
    auto* stockEdit = new QLineEdit(editDialog);
    stockEdit->setText(QString::number(product->stock));
    stockEdit->setValidator(new QIntValidator(0, 1000000000, editDialog));
    form->addRow("Stock:", stockEdit);
    
    layout->addLayout(form);
    
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, editDialog);
    buttons->button(QDialogButtonBox::Ok)->setText("Save");
    layout->addWidget(buttons);
    
    connect(buttons, &QDialogButtonBox::accepted, editDialog, [this, editDialog, nameEdit, priceEdit, stockEdit, oldName = productName]() {
        try {
            std::string newName = formatName(ss(nameEdit->text()));
            if (newName.empty()) {
                QMessageBox::warning(editDialog, "error", "Product name cannot be empty");
                return;
            }
            
            double oldPrice = 0.0;
            const Product* oldProduct = productSvc_.findProduct(oldName);
            if (oldProduct) {
                oldPrice = oldProduct->price;
            }
            
            double price = parsePrice(priceEdit->text());
            bool ok = false;
            int stock = stockEdit->text().toInt(&ok);
            if (!ok || stock < 0) {
                QMessageBox::warning(editDialog, "error", "Invalid stock value");
                return;
            }
            
            productSvc_.updateProduct(oldName, newName, price, stock);
            productSvc_.save();
            
            svc_.setPrices(productSvc_.all());
            bool priceChanged = (oldProduct && std::abs(oldPrice - price) > 0.01);
            bool nameChanged = (oldName != newName);
            
            if (priceChanged || nameChanged) {
                if (nameChanged) {
                    std::string oldKey = oldName;
                    std::ranges::transform(oldKey, oldKey.begin(), ::tolower);
                    svc_.recalculateOrdersWithProduct(oldKey);
                }
                std::string newKey = newName;
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
    });
    
    connect(buttons, &QDialogButtonBox::rejected, editDialog, &QDialog::reject);
    
    editDialog->resize(400, 150);
    editDialog->exec();
    delete editDialog;
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
    productStatsLowStockLabel_->setText(lowStock);
    
    std::ranges::sort(productsVec, 
        [](const auto& a, const auto& b) { return a.second.stock > b.second.stock; });
    
    QString highStock = "High Stock (Top 3):\n";
    for (size_t i = 0; i < std::min(3UL, productsVec.size()); ++i) {
        highStock += QString("  • %1: %2\n").arg(qs(productsVec[i].second.name)).arg(productsVec[i].second.stock);
    }
    productStatsHighStockLabel_->setText(highStock);
    
    std::ranges::sort(productsVec, 
        [](const auto& a, const auto& b) { return a.second.price > b.second.price; });
    
    QString expensive = "Most Expensive (Top 3):\n";
    for (size_t i = 0; i < std::min(3UL, productsVec.size()); ++i) {
        expensive += QString("  • %1: $%2\n").arg(qs(productsVec[i].second.name))
                    .arg(QString::number(productsVec[i].second.price, 'f', 2));
    }
    productStatsExpensiveLabel_->setText(expensive);
    
    std::ranges::sort(productsVec, 
        [](const auto& a, const auto& b) { return a.second.price < b.second.price; });
    
    QString cheap = "Cheapest (Top 3):\n";
    for (size_t i = 0; i < std::min(3UL, productsVec.size()); ++i) {
        cheap += QString("  • %1: $%2\n").arg(qs(productsVec[i].second.name))
                .arg(QString::number(productsVec[i].second.price, 'f', 2));
    }
    productStatsCheapLabel_->setText(cheap);
    
    int totalCount = products.size();
    double totalValue = 0.0;
    for (const auto& [key, product] : products) {
        (void)key; // unused
        totalValue += product.price * product.stock;
    }
    
    productStatsTotalCountLabel_->setText(QString("Total Products: %1").arg(totalCount));
    productStatsTotalValueLabel_->setText(QString("Total Value: $%1").arg(QString::number(totalValue, 'f', 2)));
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
    
    statsNewLabel_->setText(QString("New: %1").arg(newCount));
    statsInProgressLabel_->setText(QString("In Progress: %1").arg(inProgressCount));
    statsDoneLabel_->setText(QString("Done: %1").arg(doneCount));
    statsCanceledLabel_->setText(QString("Canceled: %1").arg(canceledCount));
    statsTotalRevenueLabel_->setText(QString("Total Revenue: $%1").arg(QString::number(totalRevenue, 'f', 2)));
}

void MainWindow::applyFilters() {
    activeClientFilter_ = clientFilterEdit_->text().trimmed();
    activeStatusFilter_ = statusFilterCombo_->currentIndex() == 0 ? QString() : statusFilterCombo_->currentText();
    minTotalText_ = minTotalEdit_->text().trimmed();
    maxTotalText_ = maxTotalEdit_->text().trimmed();
    minIdText_ = minIdEdit_->text().trimmed();
    maxIdText_ = maxIdEdit_->text().trimmed();
    useFrom_ = useFromCheck_->isChecked();
    useTo_ = useToCheck_->isChecked();
    fromDate_ = fromDateEdit_->dateTime();
    toDate_ = toDateEdit_->dateTime();
    refreshTable();
}

void MainWindow::onFilterChanged() {
    applyFilters();
}

void MainWindow::onClearFilter() {
    clientFilterEdit_->clear();
    statusFilterCombo_->setCurrentIndex(0);
    minTotalEdit_->clear();
    maxTotalEdit_->clear();
    minIdEdit_->clear();
    maxIdEdit_->clear();
    useFromCheck_->setChecked(false);
    useToCheck_->setChecked(false);
    fromDateEdit_->setDateTime(QDateTime::currentDateTime());
    toDateEdit_->setDateTime(QDateTime::currentDateTime());
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
        clientFilterEdit_->setCompleter(clientFilterCompleter_);
    } else {
        clientFilterModel_->setStringList(clientNames);
    }
}
