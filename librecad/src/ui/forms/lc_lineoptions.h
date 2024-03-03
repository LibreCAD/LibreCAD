#ifndef LC_LINEOPTIONS_H
#define LC_LINEOPTIONS_H

#include<memory>
#include<QWidget>
#include <lc_actiondrawlinerel.h>

//class RS_ActionInterface;

//class RS_ActionDrawLine;
namespace Ui {
class Ui_LineOptionsRel;
}

class LC_LineOptions : public QWidget
{
    Q_OBJECT

public:
    LC_LineOptions(QWidget* parent = 0, Qt::WindowFlags fl = {});
    ~LC_LineOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a , bool update);
    virtual void close();
    virtual void undo();
    virtual void redo();
    virtual void polyline();
    virtual void start();
    void setPointState();
    void setYState();
    void setXState();
    void onAngleClicked(bool value);
    void onSetAngle();
    void onAngleRelativeClicked(bool value);
protected:
    LC_ActionDrawLineRel* action;

protected slots:
    virtual void languageChange();


private:
    Ui::Ui_LineOptionsRel* ui;

};

#endif // LC_LINEOPTIONS_H
