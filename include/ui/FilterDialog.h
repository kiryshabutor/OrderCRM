#pragma once
#include <QDialog>
#include <QString>
#include <QDateTime>

class QLineEdit;
class QComboBox;
class QDialogButtonBox;
class QDateTimeEdit;
class QCheckBox;
class QLabel;

struct FilterParams {
    QString currentClient;
    QString currentStatus;
    QString minTotal;
    QString maxTotal;
    QString minId;
    QString maxId;
    QDateTime fromDt;
    bool useFrom{false};
    QDateTime toDt;
    bool useTo{false};
};

class FilterDialog : public QDialog {
    Q_OBJECT
private:
    QLineEdit* clientEdit_;
    QComboBox* statusCombo_;
    QLineEdit* minTotalEdit_;
    QLineEdit* maxTotalEdit_;
    QLineEdit* minIdEdit_;
    QLineEdit* maxIdEdit_;
    QDateTimeEdit* fromDateEdit_;
    QDateTimeEdit* toDateEdit_;
    QCheckBox* useFromCheck_;
    QCheckBox* useToCheck_;
    QDialogButtonBox* buttons_;
public:
    explicit FilterDialog(const FilterParams& params, QWidget* parent = nullptr);
    QString clientFilter() const;
    QString statusFilter() const;
    QString minTotalText() const;
    QString maxTotalText() const;
    QString minIdText() const;
    QString maxIdText() const;
    bool useFrom() const;
    bool useTo() const;
    QDateTime fromDate() const;
    QDateTime toDate() const;
};
