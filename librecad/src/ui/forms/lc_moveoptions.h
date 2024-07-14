#ifndef LC_MOVEOPTIONS_H
#define LC_MOVEOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidgetbase.h"
#include "rs_actionmodifymove.h"

namespace Ui {
    class LC_MoveOptions;
}

class LC_MoveOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT

protected:
    void doSaveSettings() override;

    void doSetAction(RS_ActionInterface *a, bool update) override;

    void languageChange() override;

public:
    explicit LC_MoveOptions();
    ~LC_MoveOptions() override;

private:
    Ui::LC_MoveOptions *ui;
    RS_ActionModifyMove* action;
    void setCopiesNumberToActionAndView(int number);
    void setUseMultipleCopiesToActionAndView(bool copies);
};

#endif // LC_MOVEOPTIONS_H
