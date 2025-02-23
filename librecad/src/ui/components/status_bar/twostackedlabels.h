#ifndef TWOSTACKEDLABELS_H
#define TWOSTACKEDLABELS_H

#include <QFrame>

class QLabel;

class TwoStackedLabels : public QFrame{
    Q_OBJECT
protected:
    void mouseReleaseEvent(QMouseEvent *event) override;

public:
    TwoStackedLabels(QWidget* parent);
    void setTopLabel(const QString& status);
    void setTopLabelToolTip(const QString& tooltip);
    void setBottomLabel(const QString& status);
    void setBottomLabelToolTips(const QString& tooltip);
signals:
    void clicked();

private:
    QLabel* top_label = nullptr;
    QLabel* bottom_label = nullptr;
};

#endif // TWOSTACKEDLABELS_H
