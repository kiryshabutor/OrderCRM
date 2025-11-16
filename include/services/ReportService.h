#pragma once
#include <QString>
#include <QList>
#include <QDateTime>
#include "include/core/Order.h"

class OrderService;

struct ReportFilterInfo {
    QString clientFilter;
    QString statusFilter;
    QString minTotal;
    QString maxTotal;
    QString minId;
    QString maxId;
    QDateTime fromDate;
    QDateTime toDate;
    bool useFrom;
    bool useTo;
};

class ReportService {
public:
    static QString generateReport(
        const QList<const Order*>& orders,
        const QString& reportName,
        bool scopeFiltered,
        bool includeFiltersHeader,
        bool includeSummarySection,
        const ReportFilterInfo& filterInfo,
        const OrderService& orderService
    );
    
private:
    static QString sanitizedBaseName(const QString& raw);
    static QString escapeCsvField(const QString& field);
};

