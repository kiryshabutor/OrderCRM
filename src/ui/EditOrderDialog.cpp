#include "include/ui/EditOrderDialog.h"
#include "include/ui/UtilsQt.h"
#include "include/Errors/CustomExceptions.h"
#include "include/utils/validation_utils.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QIntValidator>

EditOrderDialog::EditOrderDialog(OrderService& svc, int orderId, QWidget* parent)
    : QDialog(parent), svc_(svc), orderId_(orderId) {
    setWindowTitle("Edit order");
    auto* root = new QVBoxLayout(this);

    auto* idRow = new QFormLayout();
    idEdit_ = new QLineEdit(this);
    idEdit_->setText(QString::number(orderId_));
    idEdit_->setReadOnly(true);
    idRow->addRow("Order ID:", idEdit_);
    root->addLayout(idRow);

    tabs_ = new QTabWidget(this);

    tabStatus_ = new QWidget(this);
    {
        auto* lay = new QVBoxLayout(tabStatus_);
        auto* form = new QFormLayout();
        statusCombo_ = new QComboBox(this);
        statusCombo_->addItems({"new","in_progress","done","canceled"});
        form->addRow("Status:", statusCombo_);
        lay->addLayout(form);
        applyStatusBtn_ = new QPushButton("Apply", this);
        lay->addWidget(applyStatusBtn_);
        lay->addStretch();
        connect(applyStatusBtn_, &QPushButton::clicked, this, &EditOrderDialog::onApplyStatus);
    }

    tabAdd_ = new QWidget(this);
    {
        auto* lay = new QVBoxLayout(tabAdd_);
        auto* form = new QFormLayout();
        addItemName_ = new QLineEdit(this);
        addItemName_->setPlaceholderText("item name");
        addQty_ = new QLineEdit(this);
        addQty_->setPlaceholderText("qty (pcs)");
        addQty_->setValidator(new QIntValidator(1, 1000000000, this));
        form->addRow("Item name:", addItemName_);
        form->addRow("Quantity:", addQty_);
        lay->addLayout(form);
        addItemBtn_ = new QPushButton("Add", this);
        lay->addWidget(addItemBtn_);
        lay->addStretch();
        connect(addItemBtn_, &QPushButton::clicked, this, &EditOrderDialog::onAddItem);
    }

    tabRemove_ = new QWidget(this);
    {
        auto* lay = new QVBoxLayout(tabRemove_);
        auto* form = new QFormLayout();
        removeItemName_ = new QLineEdit(this);
        removeItemName_->setPlaceholderText("item name");
        form->addRow("Item name:", removeItemName_);
        lay->addLayout(form);
        removeItemBtn_ = new QPushButton("Remove", this);
        lay->addWidget(removeItemBtn_);
        lay->addStretch();
        connect(removeItemBtn_, &QPushButton::clicked, this, &EditOrderDialog::onRemoveItem);
    }

    tabs_->addTab(tabStatus_, "Edit status");
    tabs_->addTab(tabAdd_, "Add items");
    tabs_->addTab(tabRemove_, "Remove items");

    root->addWidget(tabs_);
    resize(420, 260);
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
        std::string name = ss(addItemName_->text());
        ValidationService V;
        V.validate_item_name(name);
        bool ok = false;
        int q = addQty_->text().toInt(&ok);
        if (!ok) throw ValidationException("invalid qty");
        V.validate_qty(q);
        svc_.addItem(*o, name, q);
        addItemName_->clear();
        addQty_->clear();
        QMessageBox::information(this, "ok", "item added");
        emit dataChanged();
    } catch (const CustomException& e) {
        QMessageBox::warning(this, "error", qs(e.what()));
    }
}

void EditOrderDialog::onRemoveItem() {
    try {
        Order* o = orderOrWarn();
        if (!o) return;
        std::string name = ss(removeItemName_->text());
        if (name.empty()) throw ValidationException("enter item name to remove");
        svc_.removeItem(*o, name);
        removeItemName_->clear();
        QMessageBox::information(this, "ok", "item removed");
        emit dataChanged();
    } catch (const CustomException& e) {
        QMessageBox::warning(this, "error", qs(e.what()));
    }
}
