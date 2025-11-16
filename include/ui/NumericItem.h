#pragma once
#include <QTableWidgetItem>
#include <compare>

class NumericItem : public QTableWidgetItem {
    double v_;
public:
    explicit NumericItem(int v, const QString& display)
        : QTableWidgetItem(display), v_(static_cast<double>(v)) {}
    explicit NumericItem(double v, const QString& display)
        : QTableWidgetItem(display), v_(v) {}
    std::partial_ordering operator<=>(const QTableWidgetItem& other) const {
        if (const auto* o = dynamic_cast<const NumericItem*>(&other); o) {
            return v_ <=> o->v_;
        }
        return QTableWidgetItem::operator<(other) ? std::partial_ordering::less : std::partial_ordering::greater;
    }
    bool operator<(const QTableWidgetItem& other) const override {
        if (const auto* o = dynamic_cast<const NumericItem*>(&other); o) {
            return v_ < o->v_;
        }
        return QTableWidgetItem::operator<(other);
    }
};
