#ifndef LC_CENTRALWIDGET_H
#define LC_CENTRALWIDGET_H

#include <QFrame>

class QMdiArea;

/**
 * a QMdiArea in a QFrame (for QMainWindow.setCentralWidget)
 */
class LC_CentralWidget : public QFrame
{
    Q_OBJECT

public:

    LC_CentralWidget(QWidget* parent);
    QMdiArea* getMdiArea();

protected:

    QMdiArea* mdi_area;
};

#endif // LC_CENTRALWIDGET_H
