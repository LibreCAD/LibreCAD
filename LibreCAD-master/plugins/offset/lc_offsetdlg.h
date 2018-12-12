#ifndef LC_OFFSETDLG_H
#define LC_OFFSETDLG_H

#include <QObject>
#include <QDialog>
#include <QLineEdit>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
class LC_OffsetDlg : public QDialog
{
    Q_OBJECT
public:
    LC_OffsetDlg(QWidget* parent = nullptr);
    ~LC_OffsetDlg();

    void Layout();


private:
    QLineEdit* edit;
    QGridLayout* layout;
    QPushButton* button;

    double offset_val;

public slots:
    void setData();
    double getData();
};

#endif // LC_OFFSETDLG_H
