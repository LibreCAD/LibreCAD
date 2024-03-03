#ifndef LC_ACTIONOPTIONSWIDGET_H
#define LC_ACTIONOPTIONSWIDGET_H

#include <QWidget>
#include "rs.h"

class RS_ActionInterface;
class LC_ActionOptionsWidget:public QWidget
{
    Q_OBJECT

public:
    explicit LC_ActionOptionsWidget(QWidget *parent = nullptr);
    ~LC_ActionOptionsWidget();
    void setAction(RS_ActionInterface * a, bool update = false);
    virtual void hideOptions();
protected:
    virtual void saveSettings() {};
    virtual void doSetAction(RS_ActionInterface* a, bool update) = 0;
    virtual void clearAction() = 0;
    virtual bool checkActionRttiValid(RS2::ActionType actionType);

protected slots:
    virtual void languageChange(){};
};

#endif // LC_ACTIONOPTIONSWIDGET_H
