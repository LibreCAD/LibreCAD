#ifndef LC_SNAPOPTIONSWIDGETSHOLDER_H
#define LC_SNAPOPTIONSWIDGETSHOLDER_H

#include <QWidget>

namespace Ui {
    class LC_SnapOptionsWidgetsHolder;
}

class LC_SnapOptionsWidgetsHolder : public QWidget{
Q_OBJECT

public:
    explicit LC_SnapOptionsWidgetsHolder(QWidget *parent = nullptr);
    ~LC_SnapOptionsWidgetsHolder();
    void showSnapMiddleOptions(int* middlePoints, bool on);
    void showSnapDistOptions(double* dist, bool on);
    void hideSnapOptions();
    void setLocatedOnLeft(bool value){widgetOnLeftWithinContainer = value;};
    void updateBy(LC_SnapOptionsWidgetsHolder *pHolder);
public slots:
    void languageChange();
private:
    bool widgetOnLeftWithinContainer = true;
    Ui::LC_SnapOptionsWidgetsHolder *ui;
    void hideSeparator();
    void showSeparator();

    void updateParent() const;
};

#endif // LC_SNAPOPTIONSWIDGETSHOLDER_H
