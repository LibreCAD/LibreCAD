#ifndef LC_CROSSOPTIONS_H
#define LC_CROSSOPTIONS_H

#include <QWidget>
#include "lc_actiondrawcross.h"
#include "lc_actionoptionswidget.h"

namespace Ui {
class LC_CrossOptions;
}

class LC_CrossOptions : public LC_ActionOptionsWidget
{
    Q_OBJECT

public:
    explicit LC_CrossOptions(QWidget *parent = nullptr);
    ~LC_CrossOptions() override;


public slots:
    void onXEditingFinished();
    void onYEditingFinished();
    void onAngleEditingFinished();
    void onModeIndexChanged(int index);

protected:
    void languageChange() override;
    void saveSettings() override;
    void doSetAction( RS_ActionInterface * a, bool update) override;
    void clearAction() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
private:
    Ui::LC_CrossOptions *ui;
    LC_ActionDrawCross* action = nullptr;

    void setXToActionAndView(const QString &strValue);
    void setYToActionAndView(const QString &strValue);
    void setAngleToActionAndView(const QString &expr);
    void setModeToActionAndView(int mode);
};

#endif // LC_CROSSOPTIONS_H
