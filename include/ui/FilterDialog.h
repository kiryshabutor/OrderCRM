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
    explicit FilterDialog(const QString& currentClient,
                          const QString& currentStatus,
                          const QString& minTotal,
                          const QString& maxTotal,
                          const QString& minId,
                          const QString& maxId,
                          const QDateTime& fromDt,
                          bool useFrom,
                          const QDateTime& toDt,
                          bool useTo,
                          QWidget* parent = nullptr);
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
