#ifndef LC_UCSSTATEWIDGET_H
#define LC_UCSSTATEWIDGET_H

#include <QWidget>

namespace Ui {
class LC_UCSStateWidget;
}

class LC_UCSStateWidget : public QWidget{
    Q_OBJECT
public:
    explicit LC_UCSStateWidget(QWidget *parent,const char* name);
    ~LC_UCSStateWidget();
    void update(QIcon icon, QString ucsName, QString ucsInfo);
private:
    Ui::LC_UCSStateWidget *ui;
    int iconSize = 24;
};

#endif // LC_UCSSTATEWIDGET_H
