#ifndef LC_MODIFYSTRETCHOPTIONS_H
#define LC_MODIFYSTRETCHOPTIONS_H

#include <QWidget>

#include "rs_actionmodifystretch.h"
#include "lc_actionoptionswidgetbase.h"

namespace Ui {
class LC_ModifyStretchOptions;
}

class LC_ModifyStretchOptions : public LC_ActionOptionsWidgetBase {
    Q_OBJECT

public:
    explicit LC_ModifyStretchOptions();
    ~LC_ModifyStretchOptions() override;

protected slots:
    void languageChange() override;
    void onKeepOriginalsClicked(bool val);
protected:
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
private:
    Ui::LC_ModifyStretchOptions *ui;
    RS_ActionModifyStretch* action = nullptr;
    void setKeepOriginalsToActionAndView(bool val);
};

#endif // LC_MODIFYSTRETCHOPTIONS_H
