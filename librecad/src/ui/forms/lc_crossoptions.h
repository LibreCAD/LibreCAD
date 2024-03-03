#ifndef LC_CROSSOPTIONS_H
#define LC_CROSSOPTIONS_H

#include <QWidget>
#include "lc_actiondrawcross.h"

namespace Ui {
class LC_CrossOptions;
}

class LC_CrossOptions : public QWidget
{
    Q_OBJECT

public:
    explicit LC_CrossOptions(QWidget *parent = nullptr);
    ~LC_CrossOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    void onXEditingFinished();
    void onYEditingFinished();
    void onAngleEditingFinished();
    void onModeIndexChanged(int index);
protected:
    virtual void languageChange();
private:
    Ui::LC_CrossOptions *ui;
    LC_ActionDrawCross* action = nullptr;
    void saveSettings();
    void setXToActionAndView(const QString &strValue);
    void setYToActionAndView(const QString &strValue);
    void setAngleToActionAndView(const QString &expr);
    void setModeToActionAndView(int mode);
};

#endif // LC_CROSSOPTIONS_H
