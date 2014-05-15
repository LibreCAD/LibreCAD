#ifndef PLOTDIALOG_H
#define PLOTDIALOG_H

#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>


class plotDialog : public QDialog
{
    Q_OBJECT
public:
    explicit plotDialog(QWidget *parent = 0);
    void getValues(QString& eq1, QString& eq2, QString &start, QString &end, double& step) const;
    
signals:
    
public slots:
    void slotDrawButtonClicked();

private:
    QString equation1;
    QString equation2;
    QString startValue;
    QString endValue;
    double stepSize;
    QGridLayout *mainLayout;
    QHBoxLayout* buttonLayout;
    QLabel* description;
    QLabel* lblEquasion1;
    QLabel* lblEquasion2;
    QLineEdit* lnedEquasion1;
    QLineEdit* lnedEquasion2;
    QLabel* lblStartValue;
    QLabel* lblEndValue;
    QLabel* lblStepSize;
    QLineEdit* lnedStartValue;
    QLineEdit* lnedEndValue;
    QLineEdit* lnedStepSize;
    QPushButton* btnAccept;
    QPushButton* btnCancel;
    QSpacerItem* space;

    bool readInput();

    
};

#endif // PLOTDIALOG_H
