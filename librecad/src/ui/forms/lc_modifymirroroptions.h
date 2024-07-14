#ifndef LC_MODIFYMIRROROPTIONS_H
#define LC_MODIFYMIRROROPTIONS_H

#include "rs_actionmodifymirror.h"
#include "lc_actionoptionswidgetbase.h"

namespace Ui {
class LC_ModifyMirrorOptions;
}

class LC_ModifyMirrorOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT

public:
    explicit LC_ModifyMirrorOptions();
    ~LC_ModifyMirrorOptions() override;
public slots:
    void onMirrorToLineClicked(bool clicked);
    void languageChange() override;
protected:
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
private:
    Ui::LC_ModifyMirrorOptions *ui = nullptr;
    RS_ActionModifyMirror* action = nullptr;
    void setMirrorToLineLineToActionAndView(bool line);
};

#endif // LC_MODIFYMIRROROPTIONS_H
