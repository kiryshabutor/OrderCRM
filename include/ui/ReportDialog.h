#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDialogButtonBox>

class ReportDialog : public QDialog {
    Q_OBJECT
private:
    QLineEdit* nameEdit_;
    QComboBox* scopeCombo_;
    QCheckBox* includeFilters_;
    QCheckBox* includeSummary_;
public:
    explicit ReportDialog(bool filterActive, QWidget* parent = nullptr);
    QString reportName() const;
    bool scopeFiltered() const;
    bool includeFiltersHeader() const;
    bool includeSummarySection() const;
};
