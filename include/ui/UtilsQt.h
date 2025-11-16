#pragma once
#include <QString>
#include <QPushButton>
#include <QWidget>
#include <QHBoxLayout>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QIntValidator>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <string>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include "include/Errors/CustomExceptions.h"
#include "include/core/Product.h"
#include "include/ui/NumericItem.h"

inline QString qs(const std::string& s) { return QString::fromUtf8(s.c_str()); }
inline std::string ss(const QString& s) { return s.toUtf8().constData(); }

inline std::string formatName(const std::string& name) {
    if (name.empty()) return name;
    std::string result = name;
    std::ranges::transform(result, result.begin(), ::tolower);
    if (!result.empty()) {
        result[0] = std::toupper(result[0]);
    }
    return result;
}

inline double parsePrice(const QString& input) {
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

inline QPushButton* createEditButton(QWidget* parent, const QString& tooltip = "Edit") {
    auto* btn = new QPushButton("⚙️", parent);
    btn->setStyleSheet(
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
    btn->setToolTip(tooltip);
    btn->setFixedSize(35, 25);
    return btn;
}

inline QPushButton* createDeleteButton(QWidget* parent, const QString& tooltip = "Delete") {
    auto* btn = new QPushButton("❌", parent);
    btn->setStyleSheet(
        "QPushButton {"
        "background-color: #F44336;"
        "color: white;"
        "border: none;"
        "padding: 4px 8px;"
        "border-radius: 4px;"
        "font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "background-color: #D32F2F;"
        "}"
        "QPushButton:pressed {"
        "background-color: #B71C1C;"
        "}"
    );
    btn->setToolTip(tooltip);
    btn->setFixedSize(35, 25);
    return btn;
}

inline QWidget* createButtonContainer(QWidget* parent, QPushButton* button, bool centered = true) {
    auto* container = new QWidget(parent);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    if (centered) {
        layout->addStretch();
        layout->addWidget(button);
        layout->addStretch();
    } else {
        layout->setAlignment(Qt::AlignCenter);
        layout->addWidget(button);
    }
    return container;
}

struct ProductEditDialogFields {
    QLineEdit* nameEdit{nullptr};
    QLineEdit* priceEdit{nullptr};
    QLineEdit* stockEdit{nullptr};
};

inline ProductEditDialogFields createProductEditDialogFields(QDialog* dialog, const QString& name, double price, int stock) {
    ProductEditDialogFields fields;
    auto* form = new QFormLayout();
    
    fields.nameEdit = new QLineEdit(dialog);
    fields.nameEdit->setText(name);
    form->addRow("Product name:", fields.nameEdit);
    
    fields.priceEdit = new QLineEdit(dialog);
    fields.priceEdit->setText(QString::number(price, 'f', 2));
    form->addRow("Price:", fields.priceEdit);
    
    fields.stockEdit = new QLineEdit(dialog);
    fields.stockEdit->setText(QString::number(stock));
    fields.stockEdit->setValidator(new QIntValidator(0, 1000000000, dialog));
    form->addRow("Stock:", fields.stockEdit);
    
    auto* layout = new QVBoxLayout(dialog);
    layout->addLayout(form);
    
    return fields;
}

struct ProductTableRowData {
    QTableWidgetItem* nameCell{nullptr};
    NumericItem* priceCell{nullptr};
    QTableWidgetItem* stockCell{nullptr};
    QPushButton* editBtn{nullptr};
    QPushButton* deleteBtn{nullptr};
};

inline ProductTableRowData createProductTableRow(QTableWidget* table, int row, const Product& prod, QWidget* parent,
                                                  bool centeredButtons = true) {
    ProductTableRowData data;
    
    data.nameCell = new QTableWidgetItem(qs(prod.name));
    data.nameCell->setTextAlignment(Qt::AlignCenter);
    data.priceCell = new NumericItem(prod.price, QString::number(prod.price, 'f', 2));
    data.priceCell->setTextAlignment(Qt::AlignCenter);
    data.stockCell = new QTableWidgetItem(QString::number(prod.stock));
    data.stockCell->setTextAlignment(Qt::AlignCenter);
    
    table->setItem(row, 0, data.nameCell);
    table->setItem(row, 1, data.priceCell);
    table->setItem(row, 2, data.stockCell);
    
    data.editBtn = createEditButton(parent, "Edit product");
    data.deleteBtn = createDeleteButton(parent, "Delete product");
    
    table->setCellWidget(row, 3, createButtonContainer(parent, data.editBtn, centeredButtons));
    table->setCellWidget(row, 4, createButtonContainer(parent, data.deleteBtn, centeredButtons));
    
    return data;
}

struct ProductEditValidationResult {
    std::string newName;
    double price{0.0};
    int stock{0};
    bool isValid{false};
    QString errorMessage;
};

inline ProductEditValidationResult validateProductEditInputs(const QLineEdit* nameEdit, const QLineEdit* priceEdit, const QLineEdit* stockEdit) {
    ProductEditValidationResult result;
    
    result.newName = formatName(ss(nameEdit->text()));
    if (result.newName.empty()) {
        result.errorMessage = "Product name cannot be empty";
        return result;
    }
    
    try {
        result.price = parsePrice(priceEdit->text());
    } catch (const ValidationException& e) {
        result.errorMessage = qs(e.what());
        return result;
    }
    
    bool ok = false;
    result.stock = stockEdit->text().toInt(&ok);
    if (!ok || result.stock < 0) {
        result.errorMessage = "Invalid stock value";
        return result;
    }
    
    result.isValid = true;
    return result;
}
