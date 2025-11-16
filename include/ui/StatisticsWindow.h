#pragma once
#include <QMainWindow>
#include <QWidget>
#include "include/services/OrderService.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
class QTimer;

class StatisticsWindow : public QMainWindow {
    Q_OBJECT
private:
    OrderService& svc_;
    QTimer* animationTimer_;
    double animationProgress_{0.0};

    void updateStatistics();

public:
    explicit StatisticsWindow(OrderService& svc, QWidget* parent = nullptr);
    void refreshStatistics();

protected:
    void showEvent(QShowEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onAnimationTick();

private:
    class StatusStats {
    public:
        int newCount = 0;
        int inProgressCount = 0;
        int doneCount = 0;
        int canceledCount = 0;
        double newRevenue = 0.0;
        double inProgressRevenue = 0.0;
        double doneRevenue = 0.0;
        double canceledRevenue = 0.0;
    };
    StatusStats stats_;
    
    void drawBarWithGradient(QPainter& painter, const QRect& rect, const QColor& baseColor, bool withShadow = true) const;
};

