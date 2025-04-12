#ifndef LC_CENTRALWIDGET_H
#define LC_CENTRALWIDGET_H

#include <QFrame>

class QMdiArea;

/**
 * a QMdiArea in a QFrame (for QMainWindow.setCentralWidget)
 */
class LC_CentralWidget : public QFrame{
    Q_OBJECT
public:
    LC_CentralWidget(QWidget* parent);
    QMdiArea* getMdiArea() const;
protected:
    QMdiArea* m_mdiArea = nullptr;
};

#endif // LC_CENTRALWIDGET_H
