#ifndef LC_DUPLICATEOPTIONS_H
#define LC_DUPLICATEOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidget.h"
#include "lc_actionmodifyduplicate.h"

namespace Ui {
class LC_DuplicateOptions;
}

class LC_DuplicateOptions : public LC_ActionOptionsWidget
{
    Q_OBJECT
protected:
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void clearAction() override;
    void saveSettings() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;

public:
    explicit LC_DuplicateOptions(QWidget *parent = nullptr);
    ~LC_DuplicateOptions();

private:
    Ui::LC_DuplicateOptions *ui;
    LC_ActionModifyDuplicate * action;
    void onOffsetXEditingFinished();
    void setOffsetXToActionAndView(const QString &val);
    void setOffsetYToActionAndView(const QString &val);
    void onOffsetYEditingFinished();
};

#endif // LC_DUPLICATEOPTIONS_H
