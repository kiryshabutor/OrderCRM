#include "include/ui/UtilsQt.h"
#include "include/ui/MainWindow.h"
#include "include/ui/FilterDialog.h"
#include "include/ui/NumericItem.h"
#include "include/infrastructure/TxtProductRepository.h"
#include <QVBoxLayout>
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
#include <algorithm>
#include <cctype>
#include <cmath>

MainWindow::MainWindow(OrderService& svc, ProductService& productSvc, QWidget* parent)
    : QMainWindow(parent), svc_(svc), productSvc_(productSvc) {
    auto* central = new QWidget(this);
    auto* root = new QHBoxLayout(central);

    auto* left = new QVBoxLayout();

    auto* topRow = new QHBoxLayout();
    titleLabel_ = new QLabel("Main Table", this);
    titleLabel_->setStyleSheet("font-weight: bold; font-size: 16px;");
    topRow->addWidget(titleLabel_);
    topRow->addStretch();
    left->addLayout(topRow);

    auto* filterRow = new QHBoxLayout();
    filterBtn_ = new QPushButton("Filter", this);
    clearFilterBtn_ = new QPushButton("Clear filter", this);
    filterRow->addWidget(filterBtn_);
    filterRow->addWidget(clearFilterBtn_);
    filterRow->addStretch();
    left->addLayout(filterRow);

    table_ = new QTableWidget(this);
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels({"ID","Client","Items","Status","Total","Created At"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setWordWrap(true);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSortingEnabled(true);
    left->addWidget(table_);

    orderControls_ = new QWidget(this);
    auto* orderLayout = new QVBoxLayout(orderControls_);
    orderLayout->setContentsMargins(0,0,0,0);

    auto* row1 = new QHBoxLayout();
    clientEdit_ = new QLineEdit(this);
    clientEdit_->setPlaceholderText("client name");
    addOrderBtn_ = new QPushButton("Add order", this);
    row1->addWidget(clientEdit_);
    row1->addWidget(addOrderBtn_);
    orderLayout->addLayout(row1);

    auto* row2 = new QHBoxLayout();
    orderIdEdit_ = new QLineEdit(this);
    orderIdEdit_->setPlaceholderText("order id");
    itemNameEdit_ = new QLineEdit(this);
    itemNameEdit_->setPlaceholderText("item name");
    qtyEdit_ = new QLineEdit(this);
    qtyEdit_->setPlaceholderText("qty (pcs)");
    addItemBtn_ = new QPushButton("Add item", this);
    removeItemBtn_ = new QPushButton("Remove item", this);
    row2->addWidget(orderIdEdit_);
    row2->addWidget(itemNameEdit_);
    row2->addWidget(qtyEdit_);
    row2->addWidget(addItemBtn_);
    row2->addWidget(removeItemBtn_);
    orderLayout->addLayout(row2);

    auto* row3 = new QHBoxLayout();
    statusCombo_ = new QComboBox(this);
    statusCombo_->addItems({"new","in_progress","done","canceled"});
    setStatusBtn_ = new QPushButton("Set status", this);
    row3->addWidget(statusCombo_);
    row3->addWidget(setStatusBtn_);
    orderLayout->addLayout(row3);

    auto* row4 = new QHBoxLayout();
    saveBtn_ = new QPushButton("Save", this);
    loadBtn_ = new QPushButton("Load", this);
    row4->addWidget(saveBtn_);
    row4->addWidget(loadBtn_);
    orderLayout->addLayout(row4);

    left->addWidget(orderControls_);

    reportControls_ = new QWidget(this);
    auto* reportLayout = new QHBoxLayout(reportControls_);
    reportLayout->setContentsMargins(0,0,0,0);
    reportNameEdit_ = new QLineEdit(this);
    reportNameEdit_->setPlaceholderText("report name");
    generateReportBtn_ = new QPushButton("Generate report", this);
    reportLayout->addWidget(reportNameEdit_);
    reportLayout->addWidget(generateReportBtn_);
    left->addWidget(reportControls_);
    reportControls_->setVisible(false);

    auto* right = new QVBoxLayout();
    productTable_ = new QTableWidget(this);
    productTable_->setColumnCount(2);
    productTable_->setHorizontalHeaderLabels({"Product","Price"});
    productTable_->horizontalHeader()->setStretchLastSection(true);
    productTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    productTable_->setSortingEnabled(true);
    right->addWidget(productTable_);

    auto* prodInputs = new QHBoxLayout();
    productNameEdit_ = new QLineEdit(this);
    productNameEdit_->setPlaceholderText("product name");
    productPriceEdit_ = new QLineEdit(this);
    productPriceEdit_->setPlaceholderText("price");
    prodInputs->addWidget(productNameEdit_);
    prodInputs->addWidget(productPriceEdit_);

    auto* prodButtons = new QHBoxLayout();
    addProductBtn_ = new QPushButton("Add", this);
    updateProductBtn_ = new QPushButton("Update", this);
    removeProductBtn_ = new QPushButton("Remove", this);
    prodButtons->addWidget(addProductBtn_);
    prodButtons->addWidget(updateProductBtn_);
    prodButtons->addWidget(removeProductBtn_);

    right->addLayout(prodInputs);
    right->addLayout(prodButtons);

    root->addLayout(left, 2);
    root->addLayout(right, 1);
    setCentralWidget(central);

    connect(addOrderBtn_, &QPushButton::clicked, this, &MainWindow::onAddOrder);
    connect(addItemBtn_, &QPushButton::clicked, this, &MainWindow::onAddItem);
    connect(removeItemBtn_, &QPushButton::clicked, this, &MainWindow::onRemoveItem);
    connect(setStatusBtn_, &QPushButton::clicked, this, &MainWindow::onSetStatus);
    connect(saveBtn_, &QPushButton::clicked, this, &MainWindow::onSave);
    connect(loadBtn_, &QPushButton::clicked, this, &MainWindow::onLoad);
    connect(addProductBtn_, &QPushButton::clicked, this, &MainWindow::onAddProduct);
    connect(updateProductBtn_, &QPushButton::clicked, this, &MainWindow::onUpdateProduct);
    connect(removeProductBtn_, &QPushButton::clicked, this, &MainWindow::onRemoveProduct);
    connect(filterBtn_, &QPushButton::clicked, this, &MainWindow::onOpenFilter);
    connect(clearFilterBtn_, &QPushButton::clicked, this, &MainWindow::onClearFilter);
    connect(generateReportBtn_, &QPushButton::clicked, this, &MainWindow::onGenerateReport);

    this->resize(1200, 560);
    refreshTable();
    refreshProducts();
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

QString MainWindow::sanitizedReportBaseName() const {
    QString base = reportNameEdit_ ? reportNameEdit_->text().trimmed() : QString();
    if (base.isEmpty()) base = "Report";
    QString res;
    for (QChar ch : base) {
        if (ch.isLetterOrNumber() || ch == ' ' || ch == '_' || ch == '-' ) res.append(ch);
        else res.append('_');
    }
    res = res.simplified();
    res.replace(' ', '_');
    return res;
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
            for (auto& kv : o.items) {
                auto pit = svc_.price().find(kv.first);
                QString priceText = (pit != svc_.price().end())
                    ? QString::number(pit->second, 'f', 2)
                    : QString("n/a");
                if (!first) itemsStr += "\n";
                itemsStr += QString("%1 ×%2 (%3)")
                    .arg(qs(kv.first))
                    .arg(kv.second)
                    .arg(priceText);
                first = false;
            }
            auto* idCell = new NumericItem(o.id, QString::number(o.id));
            auto* clientCell = new QTableWidgetItem(qs(o.client));
            auto* itemCell = new QTableWidgetItem(itemsStr);
            itemCell->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
            auto* statusCell = new QTableWidgetItem(qs(o.status));
            if (o.status == "new") { statusCell->setBackground(QColor("#388E3C")); statusCell->setForeground(QBrush(Qt::white)); }
            else if (o.status == "in_progress") { statusCell->setBackground(QColor("#FBC02D")); statusCell->setForeground(QBrush(Qt::black)); }
            else if (o.status == "done") { statusCell->setBackground(QColor("#1976D2")); statusCell->setForeground(QBrush(Qt::white)); }
            else if (o.status == "canceled") { statusCell->setBackground(QColor("#D32F2F")); statusCell->setForeground(QBrush(Qt::white)); }
            auto* totalCell = new NumericItem(o.total, QString::number(o.total, 'f', 2));
            auto* createdCell = new QTableWidgetItem(qs(o.createdAt));
            table_->setItem(r, 0, idCell);
            table_->setItem(r, 1, clientCell);
            table_->setItem(r, 2, itemCell);
            table_->setItem(r, 3, statusCell);
            table_->setItem(r, 4, totalCell);
            table_->setItem(r, 5, createdCell);
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
        filterBtn_->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
        titleLabel_->setText(QString("Filtered Table (%1 orders)").arg(foundCount));
        orderControls_->setVisible(false);
        saveBtn_->setVisible(false);
        loadBtn_->setVisible(false);
        reportControls_->setVisible(true);
    } else {
        filterBtn_->setStyleSheet("");
        titleLabel_->setText(QString("Main Table (%1 orders)").arg((int)svc_.all().size()));
        orderControls_->setVisible(true);
        saveBtn_->setVisible(true);
        loadBtn_->setVisible(true);
        reportControls_->setVisible(false);
    }

    table_->setSortingEnabled(true);
    filterBtn_->update();
    titleLabel_->update();
}

void MainWindow::refreshProducts() {
    productTable_->setSortingEnabled(false);
    const auto& p = productSvc_.all();
    productTable_->clearContents();
    productTable_->setRowCount((int)p.size());
    int i = 0;
    for (auto& kv : p) {
        productTable_->setItem(i, 0, new QTableWidgetItem(qs(kv.first)));
        productTable_->setItem(i, 1, new NumericItem(kv.second, QString::number(kv.second, 'f', 2)));
        ++i;
    }
    productTable_->setSortingEnabled(true);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (table_) {
        int totalWidth = table_->viewport()->width();
        table_->setColumnWidth(0, totalWidth * 0.08);
        table_->setColumnWidth(1, totalWidth * 0.18);
        table_->setColumnWidth(2, totalWidth * 0.30);
        table_->setColumnWidth(3, totalWidth * 0.14);
        table_->setColumnWidth(4, totalWidth * 0.10);
        table_->setColumnWidth(5, totalWidth * 0.20);
        table_->resizeRowsToContents();
        for (int row = 0; row < table_->rowCount(); ++row) {
            int h = table_->rowHeight(row);
            table_->setRowHeight(row, h + 8);
        }
    }
    if (productTable_) {
        int pw = productTable_->viewport()->width();
        productTable_->setColumnWidth(0, pw * 0.70);
        productTable_->setColumnWidth(1, pw * 0.30);
    }
}

Order* MainWindow::orderByIdFromInput() {
    bool ok = false;
    int id = orderIdEdit_->text().toInt(&ok);
    if (!ok || id <= 0) throw ValidationException("invalid id");
    Order* o = svc_.findById(id);
    if (!o) throw NotFoundException("order not found");
    return o;
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

void MainWindow::onAddOrder() { try {
    std::string client = ss(clientEdit_->text());
    V_.validate_client_name(client);
    Order& o = svc_.create(client);
    o.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
    clientEdit_->clear();
    svc_.sortById();
    refreshTable();
} catch (const CustomException& e) { QMessageBox::warning(this, "error", qs(e.what())); }}

void MainWindow::onAddItem() { try {
    Order* o = orderByIdFromInput();
    std::string name = ss(itemNameEdit_->text());
    V_.validate_item_name(name);
    bool ok = false;
    int q = qtyEdit_->text().toInt(&ok);
    if (!ok) throw ValidationException("invalid qty");
    V_.validate_qty(q);
    svc_.addItem(*o, name, q);
    itemNameEdit_->clear();
    qtyEdit_->clear();
    refreshTable();
} catch (const CustomException& e) { QMessageBox::warning(this, "error", qs(e.what())); }}

void MainWindow::onRemoveItem() { try {
    Order* o = orderByIdFromInput();
    std::string name = ss(itemNameEdit_->text());
    if (name.empty()) throw ValidationException("enter item name to remove");
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){ return std::tolower(c); });
    auto it = o->items.find(name);
    if (it == o->items.end()) throw NotFoundException("item not found in this order");
    o->items.erase(it);
    o->total = o->calcTotal(svc_.price());
    o->total = std::round(o->total * 100.0) / 100.0;
    refreshTable();
} catch (const CustomException& e) { QMessageBox::warning(this, "error", qs(e.what())); }}

void MainWindow::onSetStatus() { try {
    Order* o = orderByIdFromInput();
    std::string st = ss(statusCombo_->currentText());
    V_.validate_status(st);
    svc_.setStatus(*o, st);
    refreshTable();
} catch (const CustomException& e) { QMessageBox::warning(this, "error", qs(e.what())); }}

void MainWindow::onSave() { try {
    svc_.save();
    QMessageBox::information(this, "ok", "saved");
} catch (const CustomException& e) { QMessageBox::warning(this, "error", qs(e.what())); }}

void MainWindow::onLoad() { try {
    productSvc_.load();
    svc_.setPrices(productSvc_.all());
    svc_.load();
    svc_.sortById();
    refreshProducts();
    refreshTable();
    QMessageBox::information(this, "ok", "loaded");
} catch (const CustomException& e) { QMessageBox::warning(this, "error", qs(e.what())); }}

void MainWindow::onAddProduct() { try {
    std::string name = ss(productNameEdit_->text());
    double price = parsePrice(productPriceEdit_->text());
    productSvc_.addProduct(name, price);
    productSvc_.save();
    svc_.setPrices(productSvc_.all());
    refreshProducts();
    productNameEdit_->clear();
    productPriceEdit_->clear();
    QMessageBox::information(this, "ok", "product added");
} catch (const CustomException& e) { QMessageBox::warning(this, "error", qs(e.what())); }}

void MainWindow::onUpdateProduct() { try {
    std::string name = ss(productNameEdit_->text());
    double price = parsePrice(productPriceEdit_->text());
    productSvc_.updateProduct(name, name, price);
    productSvc_.save();
    svc_.setPrices(productSvc_.all());
    refreshProducts();
    refreshTable();
    productNameEdit_->clear();
    productPriceEdit_->clear();
    QMessageBox::information(this, "ok", "product updated");
} catch (const CustomException& e) { QMessageBox::warning(this, "error", qs(e.what())); }}

void MainWindow::onRemoveProduct() { try {
    std::string name = ss(productNameEdit_->text());
    if (name.empty()) throw ValidationException("enter product name to remove");
    productSvc_.removeProduct(name);
    productSvc_.save();
    svc_.setPrices(productSvc_.all());
    refreshProducts();
    refreshTable();
    productNameEdit_->clear();
    productPriceEdit_->clear();
    QMessageBox::information(this, "ok", "product removed");
} catch (const CustomException& e) { QMessageBox::warning(this, "error", qs(e.what())); }}

void MainWindow::onOpenFilter() {
    FilterDialog dlg(activeClientFilter_, activeStatusFilter_,
                     minTotalText_, maxTotalText_, minIdText_, maxIdText_,
                     fromDate_, useFrom_, toDate_, useTo_, this);
    if (dlg.exec() == QDialog::Accepted) {
        activeClientFilter_ = dlg.clientFilter();
        activeStatusFilter_ = dlg.statusFilter();
        minTotalText_ = dlg.minTotalText();
        maxTotalText_ = dlg.maxTotalText();
        minIdText_ = dlg.minIdText();
        maxIdText_ = dlg.maxIdText();
        useFrom_ = dlg.useFrom();
        useTo_ = dlg.useTo();
        fromDate_ = dlg.fromDate();
        toDate_ = dlg.toDate();
        refreshTable();
    }
}

void MainWindow::onClearFilter() {
    activeClientFilter_.clear();
    activeStatusFilter_.clear();
    minTotalText_.clear();
    maxTotalText_.clear();
    minIdText_.clear();
    maxIdText_.clear();
    useFrom_ = false;
    useTo_ = false;
    fromDate_ = QDateTime();
    toDate_ = QDateTime();
    refreshTable();
}

void MainWindow::onGenerateReport() {
    QList<const Order*> rows = currentFilteredRows();
    if (rows.isEmpty()) {
        QMessageBox::information(this, "report", "nothing to report");
        return;
    }
    QDir().mkpath("../reports/");
    QString base = sanitizedReportBaseName();
    QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString fileName = QString("../reports/%1_%2.txt").arg(base, ts);
    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "error", "cannot create report file");
        return;
    }
    QTextStream out(&f);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(2);
    out << "Order Report: " << base << "  [" << ts << "]\n";
    out << "Filters:\n";
    out << "  Client: " << (activeClientFilter_.isEmpty() ? "-" : activeClientFilter_) << "\n";
    out << "  Status: " << (activeStatusFilter_.isEmpty() ? "-" : activeStatusFilter_) << "\n";
    out << "  Total min: " << (minTotalText_.isEmpty() ? "-" : minTotalText_) << "\n";
    out << "  Total max: " << (maxTotalText_.isEmpty() ? "-" : maxTotalText_) << "\n";
    out << "  ID min: " << (minIdText_.isEmpty() ? "-" : minIdText_) << "\n";
    out << "  ID max: " << (maxIdText_.isEmpty() ? "-" : maxIdText_) << "\n";
    out << "  From: " << (useFrom_ ? fromDate_.toString("yyyy-MM-dd HH:mm:ss") : "-") << "\n";
    out << "  To: " << (useTo_ ? toDate_.toString("yyyy-MM-dd HH:mm:ss") : "-") << "\n";
    out << "\n";
    out << "Orders: " << rows.size() << "\n";
    out << "----------------------------------------\n";
    for (const Order* op : rows) {
        const Order& o = *op;
        out << "ID: " << o.id << " | Client: " << qs(o.client) << " | Status: " << qs(o.status)
            << " | Total: " << QString::number(o.total, 'f', 2) << " | Created: " << qs(o.createdAt) << "\n";
        if (!o.items.empty()) {
            out << "  Items:\n";
            for (auto& kv : o.items) {
                auto pit = svc_.price().find(kv.first);
                QString priceText = (pit != svc_.price().end())
                    ? QString::number(pit->second, 'f', 2)
                    : QString("n/a");
                out << "    - " << qs(kv.first) << " x" << kv.second << " @ " << priceText << "\n";
            }
        } else {
            out << "  Items: -\n";
        }
        out << "----------------------------------------\n";
    }
    f.close();
    QMessageBox::information(this, "report", QString("report created: %1").arg(fileName));
}
