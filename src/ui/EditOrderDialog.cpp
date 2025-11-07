#include "include/ui/EditOrderDialog.h"
#include "include/ui/UtilsQt.h"
#include "include/Errors/CustomExceptions.h"
#include "include/utils/validation_utils.h"
#include "include/core/Product.h"
#include "include/core/Order.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QIntValidator>
#include <QSet>
#include <QHeaderView>
#include <QDialog>
#include <QDialogButtonBox>
#include <algorithm>

EditOrderDialog::EditOrderDialog(OrderService& svc, int orderId, QWidget* parent)
    : QDialog(parent), svc_(svc), productSvc_(nullptr), orderId_(orderId), addItemCompleter_(nullptr) {
    setWindowTitle("Edit order");
    auto* root = new QVBoxLayout(this);

    // Order ID
    auto* idRow = new QFormLayout();
    idEdit_ = new QLineEdit(this);
    idEdit_->setText(QString::number(orderId_));
    idEdit_->setReadOnly(true);
    idRow->addRow("Order ID:", idEdit_);
    root->addLayout(idRow);

    // Status
    auto* statusForm = new QFormLayout();
    statusCombo_ = new QComboBox(this);
    statusCombo_->addItems({"new","in_progress","done","canceled"});
    statusForm->addRow("Status:", statusCombo_);
    root->addLayout(statusForm);
    
    auto* applyStatusBtn = new QPushButton("Apply status", this);
    root->addWidget(applyStatusBtn);
    connect(applyStatusBtn, &QPushButton::clicked, this, &EditOrderDialog::onApplyStatus);

    // Items table
    itemsTable_ = new QTableWidget(this);
    itemsTable_->setColumnCount(4);
    itemsTable_->setHorizontalHeaderLabels({"Product", "Quantity", "", ""});
    itemsTable_->horizontalHeader()->setStretchLastSection(false);
    // Первые две колонки растягиваются
    itemsTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    itemsTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    // Колонки с кнопками - фиксированная ширина
    itemsTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    itemsTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    itemsTable_->setColumnWidth(2, 40);
    itemsTable_->setColumnWidth(3, 40);
    itemsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    itemsTable_->setSelectionMode(QAbstractItemView::NoSelection);
    root->addWidget(itemsTable_);

    // Add item section
    auto* addSection = new QHBoxLayout();
    addItemName_ = new QLineEdit(this);
    addItemName_->setPlaceholderText("product name");
    addQty_ = new QLineEdit(this);
    addQty_->setPlaceholderText("quantity");
    addQty_->setValidator(new QIntValidator(1, 1000000000, this));
    addQty_->setMaximumWidth(100);
    addItemBtn_ = new QPushButton("Add item", this);
    addSection->addWidget(addItemName_);
    addSection->addWidget(addQty_);
    addSection->addWidget(addItemBtn_);
    root->addLayout(addSection);

    // Dialog buttons
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(addItemBtn_, &QPushButton::clicked, this, &EditOrderDialog::onAddItem);
    
    setupCompleters();
    refreshItemsTable();
    resize(500, 500);
}

void EditOrderDialog::setProductService(ProductService* productSvc) {
    productSvc_ = productSvc;
    setupCompleters();
    refreshItemsTable();
}

void EditOrderDialog::setupCompleters() {
    QStringList productNames;
    
    // Если ProductService доступен, используем его
    if (productSvc_) {
        const auto& products = productSvc_->all();
        for (const auto& kv : products) {
            productNames << qs(kv.second.name);
        }
    } else {
        // Иначе используем список продуктов из OrderService (price map)
        const auto& prices = svc_.price();
        QSet<QString> productSet;
        for (const auto& kv : prices) {
            productSet.insert(qs(kv.first));
        }
        productNames = productSet.values();
    }
    
    productNames.sort(Qt::CaseInsensitive);
    
    // Создаем или обновляем completer для добавления товара
    if (addItemCompleter_) {
        delete addItemCompleter_;
    }
    addItemCompleter_ = new QCompleter(productNames, this);
    addItemCompleter_->setCaseSensitivity(Qt::CaseInsensitive);
    addItemCompleter_->setCompletionMode(QCompleter::PopupCompletion);
    addItemCompleter_->setFilterMode(Qt::MatchContains);
    addItemName_->setCompleter(addItemCompleter_);
}

void EditOrderDialog::refreshItemsTable() {
    Order* o = svc_.findById(orderId_);
    if (!o) return;
    
    // Обновляем статус в комбобоксе
    int statusIndex = statusCombo_->findText(qs(o->status));
    if (statusIndex >= 0) {
        statusCombo_->setCurrentIndex(statusIndex);
    }
    
    itemsTable_->setSortingEnabled(false);
    itemsTable_->clearContents();
    itemsTable_->setRowCount(static_cast<int>(o->items.size()));
    
    int row = 0;
    for (const auto& kv : o->items) {
        // Получаем имя продукта (может быть в lowercase в items)
        std::string displayName = kv.first;
        if (productSvc_) {
            const Product* prod = productSvc_->findProduct(kv.first);
            if (prod) {
                displayName = prod->name;
            }
        }
        
        // Product name
        auto* nameItem = new QTableWidgetItem(qs(displayName));
        nameItem->setTextAlignment(Qt::AlignCenter);
        itemsTable_->setItem(row, 0, nameItem);
        
        // Quantity
        auto* qtyItem = new QTableWidgetItem(QString::number(kv.second));
        qtyItem->setTextAlignment(Qt::AlignCenter);
        itemsTable_->setItem(row, 1, qtyItem);
        
        // Edit button
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
        editBtn->setToolTip("Edit quantity");
        editBtn->setFixedSize(35, 25);
        connect(editBtn, &QPushButton::clicked, this, [this, itemKey = kv.first, currentQty = kv.second]() {
            onEditItem(itemKey, currentQty);
        });
        
        // Delete button
        auto* deleteBtn = new QPushButton("❌", this);
        deleteBtn->setStyleSheet(
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
        deleteBtn->setToolTip("Delete item");
        deleteBtn->setFixedSize(35, 25);
        connect(deleteBtn, &QPushButton::clicked, this, [this, itemKey = kv.first]() {
            onDeleteItem(itemKey);
        });
        
        // Обёртки для центрирования кнопок
        auto* editWidget = new QWidget(this);
        auto* editLayout = new QHBoxLayout(editWidget);
        editLayout->setAlignment(Qt::AlignCenter);
        editLayout->setContentsMargins(0, 0, 0, 0);
        editLayout->addWidget(editBtn);
        itemsTable_->setCellWidget(row, 2, editWidget);
        
        auto* deleteWidget = new QWidget(this);
        auto* deleteLayout = new QHBoxLayout(deleteWidget);
        deleteLayout->setAlignment(Qt::AlignCenter);
        deleteLayout->setContentsMargins(0, 0, 0, 0);
        deleteLayout->addWidget(deleteBtn);
        itemsTable_->setCellWidget(row, 3, deleteWidget);
        
        row++;
    }
    
    itemsTable_->resizeRowsToContents();
    itemsTable_->setSortingEnabled(true);
}

Order* EditOrderDialog::orderOrWarn() {
    Order* o = svc_.findById(orderId_);
    if (!o) QMessageBox::warning(this, "error", "order not found");
    return o;
}

void EditOrderDialog::onApplyStatus() {
    try {
        Order* o = orderOrWarn();
        if (!o) return;
        std::string st = ss(statusCombo_->currentText());
        ValidationService V;
        V.validate_status(st);
        svc_.setStatus(*o, st);
        refreshItemsTable();
        QMessageBox::information(this, "ok", "status updated");
        emit dataChanged();
    } catch (const CustomException& e) {
        QMessageBox::warning(this, "error", qs(e.what()));
    }
}

void EditOrderDialog::onAddItem() {
    try {
        Order* o = orderOrWarn();
        if (!o) return;
        std::string name = formatName(ss(addItemName_->text()));
        ValidationService V;
        V.validate_item_name(name);
        bool ok = false;
        int q = addQty_->text().toInt(&ok);
        if (!ok) throw ValidationException("invalid qty");
        V.validate_qty(q);
        svc_.addItem(*o, name, q);
        addItemName_->clear();
        addQty_->clear();
        refreshItemsTable();
        QMessageBox::information(this, "ok", "item added");
        emit dataChanged();
    } catch (const CustomException& e) {
        QMessageBox::warning(this, "error", qs(e.what()));
    }
}

void EditOrderDialog::onEditItem(const std::string& itemKey, int currentQty) {
    try {
        Order* o = orderOrWarn();
        if (!o) return;
        
        // Создаем диалог для редактирования количества
        QDialog* editDialog = new QDialog(this);
        editDialog->setWindowTitle("Edit quantity");
        auto* layout = new QVBoxLayout(editDialog);
        
        auto* form = new QFormLayout();
        auto* qtyEdit = new QLineEdit(editDialog);
        qtyEdit->setText(QString::number(currentQty));
        qtyEdit->setValidator(new QIntValidator(1, 1000000000, editDialog));
        form->addRow("Quantity:", qtyEdit);
        layout->addLayout(form);
        
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, editDialog);
        buttons->button(QDialogButtonBox::Ok)->setText("Save");
        layout->addWidget(buttons);
        
        connect(buttons, &QDialogButtonBox::accepted, editDialog, [this, editDialog, qtyEdit, o, itemKey, currentQty]() {
            try {
                bool ok = false;
                int newQty = qtyEdit->text().toInt(&ok);
                if (!ok || newQty <= 0) {
                    QMessageBox::warning(editDialog, "error", "Invalid quantity");
                    return;
                }
                
                if (newQty == currentQty) {
                    editDialog->accept();
                    return;
                }
                
                // Удаляем старый товар и добавляем с новым количеством
                int diff = newQty - currentQty;
                if (diff > 0) {
                    svc_.addItem(*o, itemKey, diff);
                } else {
                    // Уменьшаем количество
                    svc_.removeItem(*o, itemKey);
                    if (newQty > 0) {
                        svc_.addItem(*o, itemKey, newQty);
                    }
                }
                
                refreshItemsTable();
                editDialog->accept();
                QMessageBox::information(this, "ok", "quantity updated");
                emit dataChanged();
            } catch (const CustomException& e) {
                QMessageBox::warning(editDialog, "error", qs(e.what()));
            }
        });
        
        connect(buttons, &QDialogButtonBox::rejected, editDialog, &QDialog::reject);
        
        editDialog->resize(300, 120);
        editDialog->exec();
    } catch (const CustomException& e) {
        QMessageBox::warning(this, "error", qs(e.what()));
    }
}

void EditOrderDialog::onDeleteItem(const std::string& itemKey) {
    try {
        Order* o = orderOrWarn();
        if (!o) return;
        svc_.removeItem(*o, itemKey);
        refreshItemsTable();
        QMessageBox::information(this, "ok", "item removed");
        emit dataChanged();
    } catch (const CustomException& e) {
        QMessageBox::warning(this, "error", qs(e.what()));
    }
}
