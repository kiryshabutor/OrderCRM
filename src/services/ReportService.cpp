#include "include/services/ReportService.h"
#include "include/services/OrderService.h"
#include "include/ui/UtilsQt.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDateTime>
#include <QStringConverter>
#include <QMap>

QString ReportService::sanitizedBaseName(const QString& raw) {
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

QString ReportService::escapeCsvField(const QString& field) {
    QString result = field;
    result.replace("\"", "\"\"");
    if (result.contains(',') || result.contains('"') || result.contains('\n')) {
        result = "\"" + result + "\"";
    }
    return result;
}

static QString escapeCsvField(const QString& field) {
    QString result = field;
    result.replace("\"", "\"\"");
    if (result.contains(',') || result.contains('"') || result.contains('\n')) {
        result = "\"" + result + "\"";
    }
    return result;
}

static void writeFiltersHeader(QTextStream& out, const ReportFilterInfo& filterInfo) {
    out << "\n";
    out << "Filters:\n";
    out << "Client," << (filterInfo.clientFilter.isEmpty() ? "-" : filterInfo.clientFilter) << "\n";
    out << "Status," << (filterInfo.statusFilter.isEmpty() ? "-" : filterInfo.statusFilter) << "\n";
    out << "Total min," << (filterInfo.minTotal.isEmpty() ? "-" : filterInfo.minTotal) << "\n";
    out << "Total max," << (filterInfo.maxTotal.isEmpty() ? "-" : filterInfo.maxTotal) << "\n";
    out << "ID min," << (filterInfo.minId.isEmpty() ? "-" : filterInfo.minId) << "\n";
    out << "ID max," << (filterInfo.maxId.isEmpty() ? "-" : filterInfo.maxId) << "\n";
    out << "From," << (filterInfo.useFrom ? filterInfo.fromDate.toString("yyyy-MM-dd HH:mm:ss") : "-") << "\n";
    out << "To," << (filterInfo.useTo ? filterInfo.toDate.toString("yyyy-MM-dd HH:mm:ss") : "-") << "\n";
}

static QString formatOrderItems(const Order& order, const OrderService& orderService) {
    QString itemsStr;
    bool firstItem = true;
    for (const auto& [key, value] : order.items) {
        if (!firstItem) itemsStr += "; ";
        auto pit = orderService.price().find(key);
        QString priceText = (pit != orderService.price().end())
            ? QString::number(pit->second, 'f', 2)
            : QString("n/a");
        itemsStr += QString("%1 x%2 (@%3)").arg(qs(key)).arg(value).arg(priceText);
        firstItem = false;
    }
    return itemsStr.isEmpty() ? "-" : itemsStr;
}

static void writeOrderRow(QTextStream& out, const Order& order, const OrderService& orderService) {
    QString itemsStr = formatOrderItems(order, orderService);
    QString client = escapeCsvField(qs(order.client));
    QString status = qs(order.status);
    QString createdAt = qs(order.createdAt);
    createdAt.replace("T", " ");
    QString itemsStrEscaped = escapeCsvField(itemsStr);

    out << order.id << ","
        << client << ","
        << status << ","
        << QString::number(order.total, 'f', 2) << ","
        << createdAt << ","
        << itemsStrEscaped << "\n";
}

static void writeSummarySection(QTextStream& out, int orderCount, double totalSum) {
    out << "\n";
    out << "Summary:\n";
    out << "Total Orders," << orderCount << "\n";
    out << "Total Revenue," << QString::number(totalSum, 'f', 2) << "\n";
}

QString ReportService::generateReport(
    const QList<const Order*>& orders,
    const QString& reportName,
    bool scopeFiltered,
    bool includeFiltersHeader,
    bool includeSummarySection,
    const ReportFilterInfo& filterInfo,
    const OrderService& orderService
) {
    if (orders.isEmpty()) {
        return QString();
    }

    QString baseDir = QCoreApplication::applicationDirPath();
    QDir reportsDir(baseDir + "/reports");
    reportsDir.mkpath(".");
    QString base = sanitizedBaseName(reportName);
    QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString fileName = reportsDir.filePath(QString("%1_%2.csv").arg(base, ts));
    
    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QString();
    }
    
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Encoding::Utf8);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(2);

    out << "Order Report: " << base << " [" << ts << "]\n";
    out << "Scope: " << (scopeFiltered ? "Current filter" : "All orders") << "\n";
    
    if (includeFiltersHeader) {
        writeFiltersHeader(out, filterInfo);
    }
    
    out << "\n";
    out << "Orders: " << orders.size() << "\n";
    out << "\n";

    out << "Order ID,Client,Status,Total,Created At,Items\n";

    double totalSum = 0.0;
    QMap<QString, QPair<int,double>> statusAgg;

    for (const Order* op : orders) {
        const Order& o = *op;
        totalSum += o.total;
        statusAgg[qs(o.status)].first += 1;
        statusAgg[qs(o.status)].second += o.total;
        writeOrderRow(out, o, orderService);
    }

    if (includeSummarySection) {
        writeSummarySection(out, orders.size(), totalSum);
    }

    f.close();
    return fileName;
}

