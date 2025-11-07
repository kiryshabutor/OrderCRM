#include "include/ui/AddOrderDialog.h"
#include "include/ui/UtilsQt.h"
#include "include/Errors/CustomExceptions.h"
#include "include/utils/validation_utils.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>

AddOrderDialog::AddOrderDialog(OrderService& svc, QWidget* parent)
    : QDialog(parent), svc_(svc), clientCompleter_(nullptr) {
    setWindowTitle("Add order");
    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout();
    clientEdit_ = new QLineEdit(this);
    clientEdit_->setPlaceholderText("client name");
    form->addRow("Client name:", clientEdit_);
    root->addLayout(form);
    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons_->button(QDialogButtonBox::Ok)->setText("Add");
    root->addWidget(buttons_);
    connect(buttons_, &QDialogButtonBox::accepted, this, &AddOrderDialog::onAdd);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    setupCompleter();
    resize(360, 120);
}

void AddOrderDialog::setupCompleter() {
    QSet<QString> clientSet;
    const auto& orders = svc_.all();
    for (const auto& order : orders) {
        clientSet.insert(qs(order.client));
    }
    
    QStringList clientNames = clientSet.values();
    clientNames.sort(Qt::CaseInsensitive);
    
    if (clientCompleter_) {
        delete clientCompleter_;
    }
    clientCompleter_ = new QCompleter(clientNames, this);
    clientCompleter_->setCaseSensitivity(Qt::CaseInsensitive);
    clientCompleter_->setCompletionMode(QCompleter::PopupCompletion);
    clientCompleter_->setFilterMode(Qt::MatchContains);
    clientEdit_->setCompleter(clientCompleter_);
}

void AddOrderDialog::onAdd() {
    try {
        std::string client = formatName(ss(clientEdit_->text()));
        ValidationService V;
        V.validate_client_name(client);
        Order& o = svc_.create(client);
        createdId_ = o.id;
        accept();
    } catch (const CustomException& e) {
        QMessageBox::warning(this, "error", qs(e.what()));
    }
}
