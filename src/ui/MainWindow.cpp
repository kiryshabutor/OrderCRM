#include "include/ui/UtilsQt.h"
#include "include/ui/MainWindow.h"
#include "include/ui/ProductWindow.h"
#include "include/ui/StatisticsWindow.h"
#include "include/ui/NumericItem.h"
#include "include/ui/AddOrderDialog.h"
#include "include/ui/EditOrderDialog.h"
#include "include/ui/ReportDialog.h"
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
    : QMainWindow(parent), svc_(svc), productSvc_(productSvc), productWindow_(nullptr), statisticsWindow_(nullptr), clientFilterCompleter_(nullptr), clientFilterModel_(nullptr) {
    auto* central = new QWidget(this);
    auto* root = new QHBoxLayout(central);

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
    table_->setWordWrap(true);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionMode(QAbstractItemView::NoSelection); // Запрещаем выделение строк
    table_->setSortingEnabled(true);
    left->addWidget(table_);

    auto* actionRow = new QHBoxLayout();
    addOrderBtn_ = new QPushButton("Add order", this);
    reportBtn_ = new QPushButton("Report", this);
    productsBtn_ = new QPushButton("Products", this);
    actionRow->addWidget(addOrderBtn_);
    actionRow->addWidget(reportBtn_);
    actionRow->addWidget(productsBtn_);
    actionRow->addStretch();
    left->addLayout(actionRow);
    
    // Визуальное разделение между таблицей и фильтрами
    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet("background-color: #000000; max-width: 2px;");
    root->addWidget(separator);

    // Right panel - Filters
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
    clearFilterBtn_->setEnabled(false); // По умолчанию неактивна
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
    
    // Отступ между фильтрами и статистикой
    right->addSpacing(20);
    
    // Statistics panel
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

    root->addLayout(left, 3);
    root->addLayout(right, 1);
    setCentralWidget(central);

    // Connect signals
    connect(addOrderBtn_, &QPushButton::clicked, this, &MainWindow::onAddOrder);
    connect(reportBtn_, &QPushButton::clicked, this, &MainWindow::onOpenReportDialog);
    connect(productsBtn_, &QPushButton::clicked, this, &MainWindow::onOpenProducts);
    connect(openChartsBtn_, &QPushButton::clicked, this, &MainWindow::onOpenStatistics);
    connect(clearFilterBtn_, &QPushButton::clicked, this, &MainWindow::onClearFilter);

    // Connect filter controls
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
    this->resize(1200, 600);
    refreshTable();
    updateStatistics();
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

QString MainWindow::sanitizedBaseName(const QString& raw) {
    QString base = raw.trimmed();
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
                // Используем сохраненную цену, если она есть, иначе текущую цену
                double itemPrice = 0.0;
                bool priceFound = false;
                
                if (!o.itemPrices.empty()) {
                    auto savedPriceIt = o.itemPrices.find(kv.first);
                    if (savedPriceIt != o.itemPrices.end()) {
                        itemPrice = savedPriceIt->second;
                        priceFound = true;
                    }
                }
                
                if (!priceFound) {
                    auto pit = svc_.price().find(kv.first);
                    if (pit != svc_.price().end()) {
                        itemPrice = pit->second;
                        priceFound = true;
                    }
                }
                
                // Форматируем цену с точностью до 2 знаков после запятой
                // Используем 'g' формат с максимальной точностью, чтобы избежать потери дробной части
                QString priceText = priceFound ? QString::number(itemPrice, 'f', 2) : QString("n/a");
                if (!first) itemsStr += "\n";
                itemsStr += QString("%1 ×%2 (%3)")
                    .arg(qs(kv.first))
                    .arg(kv.second)
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
            
            // Кнопка редактирования в ячейке таблицы (обёрнута в виджет для центрирования)
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
                const Order* order = svc_.findById(orderId);
                if (!order) {
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
            
            // Обёртка для центрирования кнопки
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
        // Активируем кнопку и делаем её красной когда фильтры есть
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
        // Деактивируем кнопку когда фильтров нет
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
    
    // Обновляем окно статистики, если оно открыто
    if (statisticsWindow_ && statisticsWindow_->isVisible()) {
        statisticsWindow_->refreshStatistics();
    }
    
    // Обновляем автодополнение клиентов (может появиться новый клиент)
    setupCompleters();
}


void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (table_) {
        int totalWidth = table_->viewport()->width();
        table_->setColumnWidth(0, totalWidth * 0.075);
        table_->setColumnWidth(1, totalWidth * 0.17);
        table_->setColumnWidth(2, totalWidth * 0.28);
        table_->setColumnWidth(3, totalWidth * 0.13);
        table_->setColumnWidth(4, totalWidth * 0.095);
        table_->setColumnWidth(5, totalWidth * 0.19);
        table_->setColumnWidth(6, 50); // Фиксированная ширина для колонки с кнопкой редактирования
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

    QString baseDir = QCoreApplication::applicationDirPath();
    QDir reportsDir(baseDir + "/reports");
    reportsDir.mkpath(".");
    QString base = sanitizedBaseName(dlg.reportName());
    QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString fileName = reportsDir.filePath(QString("%1_%2.csv").arg(base, ts));
    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "error", "cannot create report file");
        return;
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Encoding::Utf8);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(2);

    // Заголовок отчета
    out << "Order Report: " << base << " [" << ts << "]\n";
    out << "Scope: " << (dlg.scopeFiltered() ? "Current filter" : "All orders") << "\n";
    
    if (dlg.includeFiltersHeader()) {
        out << "\n";
        out << "Filters:\n";
        out << "Client," << (activeClientFilter_.isEmpty() ? "-" : activeClientFilter_) << "\n";
        out << "Status," << (activeStatusFilter_.isEmpty() ? "-" : activeStatusFilter_) << "\n";
        out << "Total min," << (minTotalText_.isEmpty() ? "-" : minTotalText_) << "\n";
        out << "Total max," << (maxTotalText_.isEmpty() ? "-" : maxTotalText_) << "\n";
        out << "ID min," << (minIdText_.isEmpty() ? "-" : minIdText_) << "\n";
        out << "ID max," << (maxIdText_.isEmpty() ? "-" : maxIdText_) << "\n";
        out << "From," << (useFrom_ ? fromDate_.toString("yyyy-MM-dd HH:mm:ss") : "-") << "\n";
        out << "To," << (useTo_ ? toDate_.toString("yyyy-MM-dd HH:mm:ss") : "-") << "\n";
    }
    
    out << "\n";
    out << "Orders: " << rows.size() << "\n";
    out << "\n";

    // Заголовки таблицы заказов
    out << "Order ID,Client,Status,Total,Created At,Items\n";

    double totalSum = 0.0;
    QMap<QString, QPair<int,double>> statusAgg;

    for (const Order* op : rows) {
        const Order& o = *op;
        totalSum += o.total;
        statusAgg[qs(o.status)].first += 1;
        statusAgg[qs(o.status)].second += o.total;

        // Формируем строку с товарами
        QString itemsStr;
        bool firstItem = true;
        for (auto& kv : o.items) {
            if (!firstItem) itemsStr += "; ";
            
            // Используем сохраненную цену, если она есть, иначе текущую цену
            double itemPrice = 0.0;
            bool priceFound = false;
            
            if (!o.itemPrices.empty()) {
                auto savedPriceIt = o.itemPrices.find(kv.first);
                if (savedPriceIt != o.itemPrices.end()) {
                    itemPrice = savedPriceIt->second;
                    priceFound = true;
                }
            }
            
            if (!priceFound) {
                auto pit = svc_.price().find(kv.first);
                if (pit != svc_.price().end()) {
                    itemPrice = pit->second;
                    priceFound = true;
                }
            }
            
            QString priceText = priceFound ? QString::number(itemPrice, 'f', 2) : QString("n/a");
            itemsStr += QString("%1 x%2 (@%3)").arg(qs(kv.first)).arg(kv.second).arg(priceText);
            firstItem = false;
        }
        if (itemsStr.isEmpty()) itemsStr = "-";

        // Экранируем кавычки и запятые в CSV
        QString client = qs(o.client);
        client.replace("\"", "\"\""); // Экранируем двойные кавычки
        if (client.contains(',') || client.contains('"') || client.contains('\n')) {
            client = "\"" + client + "\"";
        }

        QString status = qs(o.status);
        QString createdAt = qs(o.createdAt);
        createdAt.replace("T", " "); // Заменяем T на пробел для читаемости

        itemsStr.replace("\"", "\"\""); // Экранируем двойные кавычки
        if (itemsStr.contains(',') || itemsStr.contains('"') || itemsStr.contains('\n')) {
            itemsStr = "\"" + itemsStr + "\"";
        }

        out << o.id << ","
            << client << ","
            << status << ","
            << QString::number(o.total, 'f', 2) << ","
            << createdAt << ","
            << itemsStr << "\n";
    }

    // Сводка
    out << "\n";
    if (dlg.includeSummarySection()) {
        out << "Summary:\n";
        out << "Total Orders," << rows.size() << "\n";
        out << "Total Revenue," << QString::number(totalSum, 'f', 2) << "\n";
    }

    f.close();
    QMessageBox::information(this, "report", QString("Excel report created: %1").arg(fileName));
}

void MainWindow::onOpenProducts() {
    if (!productWindow_) {
        productWindow_ = new ProductWindow(productSvc_, svc_, this);
        connect(productWindow_, &ProductWindow::ordersChanged, this, &MainWindow::refreshTable);
        connect(productWindow_, &ProductWindow::productsChanged, this, &MainWindow::refreshTable);
    }
    productWindow_->show();
    productWindow_->raise();
    productWindow_->activateWindow();
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
    // Получаем список всех уникальных клиентов
    QSet<QString> clientSet;
    const auto& orders = svc_.all();
    for (const auto& order : orders) {
        clientSet.insert(qs(order.client));
    }
    
    QStringList clientNames = clientSet.values();
    clientNames.sort(Qt::CaseInsensitive);
    
    // Создаем или обновляем модель и completer для фильтра клиента
    if (!clientFilterModel_) {
        // Первый раз создаем модель и completer
        clientFilterModel_ = new QStringListModel(clientNames, this);
        clientFilterCompleter_ = new QCompleter(clientFilterModel_, this);
        clientFilterCompleter_->setCaseSensitivity(Qt::CaseInsensitive);
        clientFilterCompleter_->setCompletionMode(QCompleter::PopupCompletion);
        clientFilterCompleter_->setFilterMode(Qt::MatchContains);
        clientFilterEdit_->setCompleter(clientFilterCompleter_);
    } else {
        // Обновляем существующую модель данными
        clientFilterModel_->setStringList(clientNames);
    }
}
