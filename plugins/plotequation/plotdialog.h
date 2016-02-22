#ifndef PLOTDIALOG_H
#define PLOTDIALOG_H

#include <memory>
#include <QDialog>

class QGridLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QHBoxLayout;
class QSpacerItem;
class QComboBox;


class plotDialog : public QDialog
{
    Q_OBJECT
public:
    enum EntityType{
        LineSegments=0, //add line segments
        Polyline=1, //add polyline contains the line segments
        SplinePoints=2 //add LC_SplinePoints to use 2nd spline, and other features provided
    };

    explicit plotDialog(QWidget *parent = 0);
    ~plotDialog()=default;
    void getValues(QString& eq1, QString& eq2, QString &start, QString &end, double& step) const;
    EntityType getEntityType() const;

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
    QComboBox* m_pTypeSelection;

    bool readInput();

    
};

#endif // PLOTDIALOG_H
