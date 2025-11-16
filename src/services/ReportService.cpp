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

        QString itemsStr;
        bool firstItem = true;
        for (const auto& [key, value] : o.items) {
            if (!firstItem) itemsStr += "; ";
            auto pit = orderService.price().find(key);
            QString priceText = (pit != orderService.price().end())
                ? QString::number(pit->second, 'f', 2)
                : QString("n/a");
            itemsStr += QString("%1 x%2 (@%3)").arg(qs(key)).arg(value).arg(priceText);
            firstItem = false;
        }
        if (itemsStr.isEmpty()) itemsStr = "-";

        QString client = escapeCsvField(qs(o.client));
        QString status = qs(o.status);
        QString createdAt = qs(o.createdAt);
        createdAt.replace("T", " ");
        QString itemsStrEscaped = escapeCsvField(itemsStr);

        out << o.id << ","
            << client << ","
            << status << ","
            << QString::number(o.total, 'f', 2) << ","
            << createdAt << ","
            << itemsStrEscaped << "\n";
    }

    out << "\n";
    if (includeSummarySection) {
        out << "Summary:\n";
        out << "Total Orders," << orders.size() << "\n";
        out << "Total Revenue," << QString::number(totalSum, 'f', 2) << "\n";
    }

    f.close();
    return fileName;
}

