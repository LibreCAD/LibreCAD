#ifndef TWOSTACKEDLABELS_H
#define TWOSTACKEDLABELS_H

#include <QFrame>

class QLabel;

class TwoStackedLabels : public QFrame
{
    Q_OBJECT

public:
    TwoStackedLabels(QWidget* parent);

    void setTopLabel(const QString& status);
    void setBottomLabel(const QString& status);

private:
    QLabel* top_label;
    QLabel* bottom_label;
};

#endif // TWOSTACKEDLABELS_H
