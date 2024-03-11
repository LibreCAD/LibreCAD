#ifndef LC_LINEJOINOPTIONS_H
#define LC_LINEJOINOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidget.h"
#include "lc_actionmodifylinejoin.h"

namespace Ui {
class LC_LineJoinOptions;
}

class LC_LineJoinOptions : public LC_ActionOptionsWidget {
Q_OBJECT

public:
    explicit LC_LineJoinOptions(QWidget *parent = nullptr);
    ~LC_LineJoinOptions() override;

    void saveSettings() override;

protected slots:
    void languageChange() override;
    void doSetAction( RS_ActionInterface * a, bool update) override;
    void clearAction() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;

//    void onRadiusEditingFinished();
    void onUsePolylineClicked(bool value);
    void onAttributesSourceIndexChanged(int index);
    void onEdgeModelLine1IndexChanged(int index);
    void onEdgeModelLine2IndexChanged(int index);
    void onRemoveOriginalsClicked(bool value);
private:
    Ui::LC_LineJoinOptions *ui;
    LC_ActionModifyLineJoin *action;

    void setEdgeModeLine1ToActionAndView(int index);
    void setEdgeModeLine2ToActionAndView(int index);
    void setUsePolylineToActionAndView(bool value);
    void setAttributesSourceToActionAndView(int index);
    void setRemoveOriginalsToActionAndView(bool value);
};

#endif // LC_LINEJOINOPTIONS_H
