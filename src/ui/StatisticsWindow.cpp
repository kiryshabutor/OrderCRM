#include "include/ui/StatisticsWindow.h"
#include "include/ui/UtilsQt.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QRect>
#include <QColor>
#include <QScrollArea>
#include <QTimer>
#include <QLinearGradient>
#include <algorithm>
#include <cmath>

StatisticsWindow::StatisticsWindow(OrderService& svc, QWidget* parent)
    : QMainWindow(parent), svc_(svc) {
    setWindowTitle("Statistics and Charts");
    
    auto* central = new QWidget(this);
    new QVBoxLayout(central); // Layout is managed by Qt's parent-child relationship

    updateStatistics();
    
    animationTimer_ = new QTimer(this);
    connect(animationTimer_, &QTimer::timeout, this, &StatisticsWindow::onAnimationTick);

    setCentralWidget(central);
    setMinimumSize(800, 600);
    resize(900, 700);
}

void StatisticsWindow::updateStatistics() {
    stats_ = StatusStats();
    const auto& all = svc_.all();
    
    for (const auto& o : all) {
        if (o.status == "new") {
            stats_.newCount++;
            stats_.newRevenue += o.total;
        } else if (o.status == "in_progress") {
            stats_.inProgressCount++;
            stats_.inProgressRevenue += o.total;
        } else if (o.status == "done") {
            stats_.doneCount++;
            stats_.doneRevenue += o.total;
        } else if (o.status == "canceled") {
            stats_.canceledCount++;
            stats_.canceledRevenue += o.total;
        }
    }
}


void StatisticsWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    refreshStatistics();
    
    animationProgress_ = 0.0;
    animationTimer_->start(16);
}

void StatisticsWindow::refreshStatistics() {
    updateStatistics();
    update();
}

void StatisticsWindow::onAnimationTick() {
    animationProgress_ += 0.05;
    if (animationProgress_ >= 1.0) {
        animationProgress_ = 1.0;
        animationTimer_->stop();
    }
    update();
}

void StatisticsWindow::drawBarWithGradient(QPainter& painter, const QRect& rect, const QColor& baseColor, bool withShadow) const {
    if (rect.isEmpty()) return;
    
    if (withShadow) {
        QRect shadowRect = rect.translated(3, 3);
        QColor shadowColor(0, 0, 0, 60);
        painter.fillRect(shadowRect, shadowColor);
    }
    
    QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
    QColor lightColor = baseColor.lighter(120);
    QColor darkColor = baseColor.darker(110);
    gradient.setColorAt(0.0, lightColor);
    gradient.setColorAt(0.5, baseColor);
    gradient.setColorAt(1.0, darkColor);
    
    painter.setBrush(gradient);
    painter.setPen(QPen(baseColor.darker(150), 1));
    
    int radius = 5;
    painter.drawRoundedRect(rect, radius, radius);
    
    QLinearGradient highlight(rect.topLeft(), QPoint(rect.left(), rect.top() + rect.height() / 3));
    highlight.setColorAt(0.0, QColor(255, 255, 255, 100));
    highlight.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.setBrush(highlight);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRect(rect.left(), rect.top(), rect.width(), rect.height() / 3), radius, radius);
}

void StatisticsWindow::paintEvent(QPaintEvent* event) {
    QMainWindow::paintEvent(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    if (int totalOrders = stats_.newCount + stats_.inProgressCount + stats_.doneCount + stats_.canceledCount; totalOrders == 0) {
        painter.setPen(QPen(Qt::white));
        painter.setFont(QFont("Arial", 14, QFont::Normal));
        painter.drawText(rect(), Qt::AlignCenter, "No orders to display");
        return;
    }
    
    int margin = 50;
    int chartWidth = width() - 2 * margin;
    int chartHeight = 220;
    int chartY = 40;
    
    QColor newColor("#4CAF50");
    QColor inProgressColor("#FFC107");
    QColor doneColor("#2196F3");
    QColor canceledColor("#F44336");
    
    int barWidth = (chartWidth - 60) / 4;
    int maxCount = std::max({stats_.newCount, stats_.inProgressCount, stats_.doneCount, stats_.canceledCount, 1});
    
    int x = margin + 15;
    int baseY = chartY + chartHeight;
    
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    
    auto drawAnimatedBar = [&](int count, const QColor& color, const QString& label) {
        if (count < 0) return;
        int targetHeight = maxCount > 0 ? (count * chartHeight) / maxCount : 0;
        auto animatedHeight = static_cast<int>(targetHeight * animationProgress_);
        QRect barRect(x, baseY - animatedHeight, barWidth, animatedHeight);
        
        if (!barRect.isEmpty()) {
            drawBarWithGradient(painter, barRect, color);
        }
        
        painter.setPen(QPen(Qt::white));
        painter.setFont(QFont("Arial", 9, QFont::Bold));
        painter.drawText(QRect(x, baseY + 10, barWidth, 40), Qt::AlignCenter | Qt::TextWordWrap, label);
        
        if (animatedHeight > 25) {
            painter.setPen(QPen(Qt::white));
            painter.setFont(QFont("Arial", 10, QFont::Bold));
            painter.drawText(barRect, Qt::AlignCenter, QString::number(count));
        }
    };
    
    drawAnimatedBar(stats_.newCount, newColor, QString("New\n%1").arg(stats_.newCount));
    x += barWidth + 15;
    
    drawAnimatedBar(stats_.inProgressCount, inProgressColor, QString("In Progress\n%1").arg(stats_.inProgressCount));
    x += barWidth + 15;
    
    drawAnimatedBar(stats_.doneCount, doneColor, QString("Done\n%1").arg(stats_.doneCount));
    x += barWidth + 15;
    
    drawAnimatedBar(stats_.canceledCount, canceledColor, QString("Canceled\n%1").arg(stats_.canceledCount));
    
    int revenueChartY = chartY + chartHeight + 100;
    double maxRevenue = std::max({stats_.newRevenue, stats_.inProgressRevenue, stats_.doneRevenue, stats_.canceledRevenue, 1.0});
    
    x = margin + 15;
    baseY = revenueChartY + chartHeight;
    
    drawAnimatedRevenueBar(painter, stats_.newRevenue, newColor, "New", x, baseY, barWidth, chartHeight, maxRevenue);
    x += barWidth + 15;
    
    drawAnimatedRevenueBar(painter, stats_.inProgressRevenue, inProgressColor, "In Progress", x, baseY, barWidth, chartHeight, maxRevenue);
    x += barWidth + 15;
    
    drawAnimatedRevenueBar(painter, stats_.doneRevenue, doneColor, "Done", x, baseY, barWidth, chartHeight, maxRevenue);
    x += barWidth + 15;
    
    drawAnimatedRevenueBar(painter, stats_.canceledRevenue, canceledColor, "Canceled", x, baseY, barWidth, chartHeight, maxRevenue);
    
    painter.setFont(QFont("Arial", 16, QFont::Bold));
    painter.setPen(QPen(Qt::white));
    QRect titleRect1(margin, chartY - 40, chartWidth, 30);
    painter.drawText(titleRect1, Qt::AlignLeft | Qt::AlignVCenter, "Orders by Status");
    
    QRect titleRect2(margin, revenueChartY - 40, chartWidth, 30);
    painter.drawText(titleRect2, Qt::AlignLeft | Qt::AlignVCenter, "Revenue by Status");
    
    painter.setPen(QPen(Qt::white, 2));
    painter.drawLine(margin, chartY - 10, margin + chartWidth, chartY - 10);
    painter.drawLine(margin, revenueChartY - 10, margin + chartWidth, revenueChartY - 10);
    
    int legendY = revenueChartY + chartHeight + 80;
    int legendX = margin + (chartWidth - 400) / 2;
    int legendSpacing = 100;
    
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.setPen(QPen(Qt::white));
    
    QRect legendRect1(legendX, legendY, 15, 15);
    painter.fillRect(legendRect1, newColor);
    painter.setPen(QPen(Qt::white, 1));
    painter.drawRect(legendRect1);
    painter.setPen(QPen(Qt::white));
    painter.drawText(QRect(legendX + 20, legendY, 80, 15), Qt::AlignLeft | Qt::AlignVCenter, "New");
    
    QRect legendRect2(legendX + legendSpacing, legendY, 15, 15);
    painter.fillRect(legendRect2, inProgressColor);
    painter.setPen(QPen(Qt::white, 1));
    painter.drawRect(legendRect2);
    painter.setPen(QPen(Qt::white));
    painter.drawText(QRect(legendX + legendSpacing + 20, legendY, 80, 15), Qt::AlignLeft | Qt::AlignVCenter, "In Progress");
    
    QRect legendRect3(legendX + 2 * legendSpacing, legendY, 15, 15);
    painter.fillRect(legendRect3, doneColor);
    painter.setPen(QPen(Qt::white, 1));
    painter.drawRect(legendRect3);
    painter.setPen(QPen(Qt::white));
    painter.drawText(QRect(legendX + 2 * legendSpacing + 20, legendY, 80, 15), Qt::AlignLeft | Qt::AlignVCenter, "Done");
    
    QRect legendRect4(legendX + 3 * legendSpacing, legendY, 15, 15);
    painter.fillRect(legendRect4, canceledColor);
    painter.setPen(QPen(Qt::white, 1));
    painter.drawRect(legendRect4);
    painter.setPen(QPen(Qt::white));
    painter.drawText(QRect(legendX + 3 * legendSpacing + 20, legendY, 80, 15), Qt::AlignLeft | Qt::AlignVCenter, "Canceled");
}

void StatisticsWindow::drawAnimatedRevenueBar(QPainter& painter, double revenue, const QColor& color, const QString& statusName,
                                               int x, int baseY, int barWidth, int chartHeight, double maxRevenue) const {
    if (revenue < 0) return;
    const int targetHeight = maxRevenue > 0 ? static_cast<int>((revenue * chartHeight) / maxRevenue) : 0;
    const auto animatedHeight = static_cast<int>(targetHeight * animationProgress_);
    QRect barRect(x, baseY - animatedHeight, barWidth, animatedHeight);
    
    if (!barRect.isEmpty()) {
        drawBarWithGradient(painter, barRect, color);
    }
    
    painter.setPen(QPen(Qt::white));
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    painter.drawText(QRect(x, baseY + 10, barWidth, 30), Qt::AlignCenter, statusName);
    
    const QString revenueText = "$" + QString::number(revenue, 'f', 2);
    QFontMetrics fm(QFont("Arial", 9, QFont::Bold));
    const int textWidth = fm.horizontalAdvance(revenueText);
    
    painter.setPen(QPen(Qt::white));
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    
    if (animatedHeight > 30 && textWidth < barWidth - 4) {
        painter.drawText(barRect, Qt::AlignCenter, revenueText);
    } else {
        const int textY = baseY - animatedHeight - 5;
        painter.drawText(QRect(x, textY - 15, barWidth, 15), Qt::AlignCenter, revenueText);
    }
}

