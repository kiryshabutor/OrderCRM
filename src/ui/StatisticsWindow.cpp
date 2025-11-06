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
    : QMainWindow(parent), svc_(svc), animationProgress_(0.0) {
    setWindowTitle("Statistics and Charts");
    
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);

    updateStatistics();
    
    // Настройка анимации
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
    
    // Запуск анимации
    animationProgress_ = 0.0;
    animationTimer_->start(16); // ~60 FPS
}

void StatisticsWindow::refreshStatistics() {
    updateStatistics();
    update();
}

void StatisticsWindow::onAnimationTick() {
    animationProgress_ += 0.05; // Скорость анимации
    if (animationProgress_ >= 1.0) {
        animationProgress_ = 1.0;
        animationTimer_->stop();
    }
    update(); // Перерисовка окна
}

void StatisticsWindow::drawBarWithGradient(QPainter& painter, const QRect& rect, const QColor& baseColor, bool withShadow) {
    if (rect.isEmpty()) return;
    
    // Рисуем тень
    if (withShadow) {
        QRect shadowRect = rect.translated(3, 3);
        QColor shadowColor(0, 0, 0, 60);
        painter.fillRect(shadowRect, shadowColor);
    }
    
    // Создаем градиент для столбца
    QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
    QColor lightColor = baseColor.lighter(120);
    QColor darkColor = baseColor.darker(110);
    gradient.setColorAt(0.0, lightColor);
    gradient.setColorAt(0.5, baseColor);
    gradient.setColorAt(1.0, darkColor);
    
    // Рисуем столбец с градиентом
    painter.setBrush(gradient);
    painter.setPen(QPen(baseColor.darker(150), 1));
    
    // Скругленные углы
    int radius = 5;
    painter.drawRoundedRect(rect, radius, radius);
    
    // Добавляем блик (световую полоску сверху)
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
    
    int totalOrders = stats_.newCount + stats_.inProgressCount + stats_.doneCount + stats_.canceledCount;
    if (totalOrders == 0) {
        painter.setPen(QPen(Qt::white));
        painter.setFont(QFont("Arial", 14, QFont::Normal));
        painter.drawText(rect(), Qt::AlignCenter, "No orders to display");
        return;
    }
    
    int margin = 50;
    int chartWidth = width() - 2 * margin;
    int chartHeight = 220;
    int chartY = 40;
    
    // Цвета для статусов (более насыщенные)
    QColor newColor("#4CAF50");
    QColor inProgressColor("#FFC107");
    QColor doneColor("#2196F3");
    QColor canceledColor("#F44336");
    
    // Рисуем столбчатую диаграмму для количества
    int barWidth = (chartWidth - 60) / 4;
    int maxCount = std::max({stats_.newCount, stats_.inProgressCount, stats_.doneCount, stats_.canceledCount, 1});
    
    int x = margin + 15;
    int baseY = chartY + chartHeight;
    
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    
    // Функция для рисования столбца с анимацией
    auto drawAnimatedBar = [&](int count, const QColor& color, const QString& label) {
        if (count < 0) return;
        int targetHeight = maxCount > 0 ? (count * chartHeight) / maxCount : 0;
        int animatedHeight = (int)(targetHeight * animationProgress_);
        QRect barRect(x, baseY - animatedHeight, barWidth, animatedHeight);
        
        if (!barRect.isEmpty()) {
            drawBarWithGradient(painter, barRect, color);
        }
        
        // Подпись под столбцом
        painter.setPen(QPen(Qt::white));
        painter.setFont(QFont("Arial", 9, QFont::Bold));
        painter.drawText(QRect(x, baseY + 10, barWidth, 40), Qt::AlignCenter | Qt::TextWordWrap, label);
        
        // Значение на столбце (если есть место)
        if (animatedHeight > 25) {
            painter.setPen(QPen(Qt::white));
            painter.setFont(QFont("Arial", 10, QFont::Bold));
            painter.drawText(barRect, Qt::AlignCenter, QString::number(count));
        }
    };
    
    // New
    drawAnimatedBar(stats_.newCount, newColor, QString("New\n%1").arg(stats_.newCount));
    x += barWidth + 15;
    
    // In Progress
    drawAnimatedBar(stats_.inProgressCount, inProgressColor, QString("In Progress\n%1").arg(stats_.inProgressCount));
    x += barWidth + 15;
    
    // Done
    drawAnimatedBar(stats_.doneCount, doneColor, QString("Done\n%1").arg(stats_.doneCount));
    x += barWidth + 15;
    
    // Canceled
    drawAnimatedBar(stats_.canceledCount, canceledColor, QString("Canceled\n%1").arg(stats_.canceledCount));
    
    // Диаграмма выручки
    int revenueChartY = chartY + chartHeight + 100;
    double maxRevenue = std::max({stats_.newRevenue, stats_.inProgressRevenue, stats_.doneRevenue, stats_.canceledRevenue, 1.0});
    
    x = margin + 15;
    baseY = revenueChartY + chartHeight;
    
    // Функция для рисования столбца выручки с анимацией
    auto drawAnimatedRevenueBar = [&](double revenue, const QColor& color, const QString& statusName) {
        if (revenue < 0) return;
        int targetHeight = maxRevenue > 0 ? (int)((revenue * chartHeight) / maxRevenue) : 0;
        int animatedHeight = (int)(targetHeight * animationProgress_);
        QRect barRect(x, baseY - animatedHeight, barWidth, animatedHeight);
        
        if (!barRect.isEmpty()) {
            drawBarWithGradient(painter, barRect, color);
        }
        
        // Подпись под столбцом (статус)
        painter.setPen(QPen(Qt::white));
        painter.setFont(QFont("Arial", 9, QFont::Bold));
        painter.drawText(QRect(x, baseY + 10, barWidth, 30), Qt::AlignCenter, statusName);
        
        // Сумма выручки - показываем на столбце, если помещается, иначе выше столбца
        QString revenueText = "$" + QString::number(revenue, 'f', 2);
        QFontMetrics fm(QFont("Arial", 9, QFont::Bold));
        int textWidth = fm.horizontalAdvance(revenueText);
        
        painter.setPen(QPen(Qt::white));
        painter.setFont(QFont("Arial", 9, QFont::Bold));
        
        if (animatedHeight > 30 && textWidth < barWidth - 4) {
            // Помещается на столбце - рисуем на столбце
            painter.drawText(barRect, Qt::AlignCenter, revenueText);
        } else {
            // Не помещается или столбец маленький - рисуем выше столбца
            int textY = baseY - animatedHeight - 5;
            painter.drawText(QRect(x, textY - 15, barWidth, 15), Qt::AlignCenter, revenueText);
        }
    };
    
    // New Revenue
    drawAnimatedRevenueBar(stats_.newRevenue, newColor, "New");
    x += barWidth + 15;
    
    // In Progress Revenue
    drawAnimatedRevenueBar(stats_.inProgressRevenue, inProgressColor, "In Progress");
    x += barWidth + 15;
    
    // Done Revenue
    drawAnimatedRevenueBar(stats_.doneRevenue, doneColor, "Done");
    x += barWidth + 15;
    
    // Canceled Revenue
    drawAnimatedRevenueBar(stats_.canceledRevenue, canceledColor, "Canceled");
    
    // Заголовки белым шрифтом
    painter.setFont(QFont("Arial", 16, QFont::Bold));
    painter.setPen(QPen(Qt::white));
    QRect titleRect1(margin, chartY - 40, chartWidth, 30);
    painter.drawText(titleRect1, Qt::AlignLeft | Qt::AlignVCenter, "Orders by Status");
    
    QRect titleRect2(margin, revenueChartY - 40, chartWidth, 30);
    painter.drawText(titleRect2, Qt::AlignLeft | Qt::AlignVCenter, "Revenue by Status");
    
    // Линии под заголовками
    painter.setPen(QPen(Qt::white, 2));
    painter.drawLine(margin, chartY - 10, margin + chartWidth, chartY - 10);
    painter.drawLine(margin, revenueChartY - 10, margin + chartWidth, revenueChartY - 10);
    
    // Легенда статусов (цветные квадратики с подписями) - внизу окна
    int legendY = revenueChartY + chartHeight + 80;
    int legendX = margin + (chartWidth - 400) / 2; // Центрируем легенду
    int legendSpacing = 100;
    
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.setPen(QPen(Qt::white));
    
    // New
    QRect legendRect1(legendX, legendY, 15, 15);
    painter.fillRect(legendRect1, newColor);
    painter.setPen(QPen(Qt::white, 1));
    painter.drawRect(legendRect1);
    painter.setPen(QPen(Qt::white));
    painter.drawText(QRect(legendX + 20, legendY, 80, 15), Qt::AlignLeft | Qt::AlignVCenter, "New");
    
    // In Progress
    QRect legendRect2(legendX + legendSpacing, legendY, 15, 15);
    painter.fillRect(legendRect2, inProgressColor);
    painter.setPen(QPen(Qt::white, 1));
    painter.drawRect(legendRect2);
    painter.setPen(QPen(Qt::white));
    painter.drawText(QRect(legendX + legendSpacing + 20, legendY, 80, 15), Qt::AlignLeft | Qt::AlignVCenter, "In Progress");
    
    // Done
    QRect legendRect3(legendX + 2 * legendSpacing, legendY, 15, 15);
    painter.fillRect(legendRect3, doneColor);
    painter.setPen(QPen(Qt::white, 1));
    painter.drawRect(legendRect3);
    painter.setPen(QPen(Qt::white));
    painter.drawText(QRect(legendX + 2 * legendSpacing + 20, legendY, 80, 15), Qt::AlignLeft | Qt::AlignVCenter, "Done");
    
    // Canceled
    QRect legendRect4(legendX + 3 * legendSpacing, legendY, 15, 15);
    painter.fillRect(legendRect4, canceledColor);
    painter.setPen(QPen(Qt::white, 1));
    painter.drawRect(legendRect4);
    painter.setPen(QPen(Qt::white));
    painter.drawText(QRect(legendX + 3 * legendSpacing + 20, legendY, 80, 15), Qt::AlignLeft | Qt::AlignVCenter, "Canceled");
}

