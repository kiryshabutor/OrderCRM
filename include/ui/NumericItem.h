#pragma once
#include <QTableWidgetItem>

class NumericItem : public QTableWidgetItem {
    double v_;
public:
    explicit NumericItem(int v, const QString& display)
        : QTableWidgetItem(display), v_(static_cast<double>(v)) {}
    explicit NumericItem(double v, const QString& display)
        : QTableWidgetItem(display), v_(v) {}
    bool operator<(const QTableWidgetItem& other) const override {
        const auto* o = dynamic_cast<const NumericItem*>(&other);
        if (o) return v_ < o->v_;
        return QTableWidgetItem::operator<(other);
    }
};
