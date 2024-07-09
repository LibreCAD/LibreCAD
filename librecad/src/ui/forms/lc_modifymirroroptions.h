#ifndef LC_MODIFYMIRROROPTIONS_H
#define LC_MODIFYMIRROROPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidget.h"
#include "rs_actionmodifymirror.h"

namespace Ui {
class LC_ModifyMirrorOptions;
}

class LC_ModifyMirrorOptions : public LC_ActionOptionsWidget
{
    Q_OBJECT

public:
    explicit LC_ModifyMirrorOptions(QWidget *parent = nullptr);
    ~LC_ModifyMirrorOptions() override;

public slots:
    void onMirrorToLineClicked(bool clicked);
    void languageChange() override;

protected:
    void doSaveSettings() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
    QString getSettingsOptionNamePrefix() override;
    QString getSettingsGroupName() override;

private:
    Ui::LC_ModifyMirrorOptions *ui = nullptr;
    RS_ActionModifyMirror* action = nullptr;
    void setMirrorToLineLineToActionAndView(bool line);
};

#endif // LC_MODIFYMIRROROPTIONS_H
