#include "include/ui/FilterDialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QDateTimeEdit>
#include <QCheckBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

FilterDialog::FilterDialog(const FilterParams& params, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Filter Orders");
    auto* root = new QVBoxLayout(this);

    auto* form = new QFormLayout();
    clientEdit_ = new QLineEdit(this);
    clientEdit_->setPlaceholderText("Enter client name");
    clientEdit_->setText(params.currentClient);

    statusCombo_ = new QComboBox(this);
    statusCombo_->addItems({"Any", "new", "in_progress", "done", "canceled"});
    int idx = 0;
    if (params.currentStatus == "new") idx = 1;
    else if (params.currentStatus == "in_progress") idx = 2;
    else if (params.currentStatus == "done") idx = 3;
    else if (params.currentStatus == "canceled") idx = 4;
    statusCombo_->setCurrentIndex(idx);

    QRegularExpression intRe("^[0-9]*$");
    QRegularExpression dblRe("^[0-9]+([\\.,][0-9]+)?$");

    minTotalEdit_ = new QLineEdit(this);
    minTotalEdit_->setPlaceholderText("min total");
    minTotalEdit_->setText(params.minTotal);
    minTotalEdit_->setValidator(new QRegularExpressionValidator(dblRe, this));

    maxTotalEdit_ = new QLineEdit(this);
    maxTotalEdit_->setPlaceholderText("max total");
    maxTotalEdit_->setText(params.maxTotal);
    maxTotalEdit_->setValidator(new QRegularExpressionValidator(dblRe, this));

    minIdEdit_ = new QLineEdit(this);
    minIdEdit_->setPlaceholderText("min id");
    minIdEdit_->setText(params.minId);
    minIdEdit_->setValidator(new QRegularExpressionValidator(intRe, this));

    maxIdEdit_ = new QLineEdit(this);
    maxIdEdit_->setPlaceholderText("max id");
    maxIdEdit_->setText(params.maxId);
    maxIdEdit_->setValidator(new QRegularExpressionValidator(intRe, this));

    auto* dateRowFrom = new QHBoxLayout();
    useFromCheck_ = new QCheckBox("From:", this);
    fromDateEdit_ = new QDateTimeEdit(this);
    fromDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    fromDateEdit_->setCalendarPopup(true);
    fromDateEdit_->setDateTime(params.fromDt.isValid() ? params.fromDt : QDateTime::currentDateTime());
    useFromCheck_->setChecked(params.useFrom);
    dateRowFrom->addWidget(useFromCheck_);
    dateRowFrom->addWidget(fromDateEdit_);

    auto* dateRowTo = new QHBoxLayout();
    useToCheck_ = new QCheckBox("To:", this);
    toDateEdit_ = new QDateTimeEdit(this);
    toDateEdit_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    toDateEdit_->setCalendarPopup(true);
    toDateEdit_->setDateTime(params.toDt.isValid() ? params.toDt : QDateTime::currentDateTime());
    useToCheck_->setChecked(params.useTo);
    dateRowTo->addWidget(useToCheck_);
    dateRowTo->addWidget(toDateEdit_);

    form->addRow("Client name:", clientEdit_);
    form->addRow("Order status:", statusCombo_);
    form->addRow("Total range:", new QWidget(this));
    form->addRow("Min total:", minTotalEdit_);
    form->addRow("Max total:", maxTotalEdit_);
    form->addRow("ID range:", new QWidget(this));
    form->addRow("Min id:", minIdEdit_);
    form->addRow("Max id:", maxIdEdit_);

    root->addLayout(form);
    root->addLayout(dateRowFrom);
    root->addLayout(dateRowTo);

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons_);

    connect(buttons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);

    resize(420, 380);
}

QString FilterDialog::clientFilter() const { return clientEdit_->text().trimmed(); }
QString FilterDialog::statusFilter() const { return statusCombo_->currentIndex() == 0 ? QString() : statusCombo_->currentText(); }
QString FilterDialog::minTotalText() const { return minTotalEdit_->text().trimmed(); }
QString FilterDialog::maxTotalText() const { return maxTotalEdit_->text().trimmed(); }
QString FilterDialog::minIdText() const { return minIdEdit_->text().trimmed(); }
QString FilterDialog::maxIdText() const { return maxIdEdit_->text().trimmed(); }
bool FilterDialog::useFrom() const { return useFromCheck_->isChecked(); }
bool FilterDialog::useTo() const { return useToCheck_->isChecked(); }
QDateTime FilterDialog::fromDate() const { return fromDateEdit_->dateTime(); }
QDateTime FilterDialog::toDate() const { return toDateEdit_->dateTime(); }
