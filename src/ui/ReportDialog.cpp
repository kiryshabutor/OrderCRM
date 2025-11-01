#include "include/ui/ReportDialog.h"
#include <QVBoxLayout>
#include <QFormLayout>

ReportDialog::ReportDialog(bool filterActive, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Create report");
    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText("report name");
    form->addRow("Report name:", nameEdit_);

    scopeCombo_ = new QComboBox(this);
    scopeCombo_->addItems({"Current filter","All orders"});
    if (!filterActive) scopeCombo_->setCurrentIndex(1);
    form->addRow("Scope:", scopeCombo_);

    includeFilters_ = new QCheckBox("Include filters header", this);
    includeFilters_->setChecked(true);
    includeSummary_ = new QCheckBox("Include summary", this);
    includeSummary_->setChecked(true);
    includeStatusSummary_ = new QCheckBox("Include status summary", this);
    includeStatusSummary_->setChecked(false);

    root->addLayout(form);
    root->addWidget(includeFilters_);
    root->addWidget(includeSummary_);
    root->addWidget(includeStatusSummary_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    resize(420, 220);
}

QString ReportDialog::reportName() const { return nameEdit_->text().trimmed(); }
bool ReportDialog::scopeFiltered() const { return scopeCombo_->currentIndex() == 0; }
bool ReportDialog::includeFiltersHeader() const { return includeFilters_->isChecked(); }
bool ReportDialog::includeSummarySection() const { return includeSummary_->isChecked(); }
bool ReportDialog::includeStatusSummarySection() const { return includeStatusSummary_->isChecked(); }
