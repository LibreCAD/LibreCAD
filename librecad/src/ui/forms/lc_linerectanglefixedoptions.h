#ifndef LC_LINERECTANGLEFIXEDOPTIONS_H
#define LC_LINERECTANGLEFIXEDOPTIONS_H

#include <QWidget>
#include "rs_actioninterface.h"
#include "lc_actiondrawlinerectanglefixed.h"

namespace Ui {
class LC_LineRectangleFixedOptions;
}

class LC_LineRectangleFixedOptions : public LC_ActionOptionsWidget
{
    Q_OBJECT



public:
    explicit LC_LineRectangleFixedOptions(QWidget *parent = nullptr);
    ~LC_LineRectangleFixedOptions() override;

    void saveSettings() override;
public slots:

    void onCornersIndexChanged(int index);
    void onSnapPointIndexChanged(int index);
    void onAngleEditingFinished();
    void onLenYEditingFinished();
    void onLenXEditingFinished();
    void onWidthEditingFinished();
    void onHeightEditingFinished();
    void onRadiusEditingFinished();
    void onUsePolylineClicked(bool value);
    void onSnapToCornerArcCenterClicked(bool value);

protected slots:
    void languageChange() override;
    void doSetAction( RS_ActionInterface * a, bool update) override;
    void clearAction() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
private:
    Ui::LC_LineRectangleFixedOptions *ui;


    LC_ActionDrawLineRectangleFixed *action;
    void setAngleToActionAndView(const QString &val);
    void setLenYToActionAnView(QString value);
    void setLenXToActionAnView(QString value);
    void setRadiusToActionAnView(QString value);
    void setHeightToActionAnView(QString height);
    void setWidthToActionAnView(QString width);
    void setCornersModeToActionAndView(int index);
    void setSnapPointModeToActionAndView(int index);
    void setUsePolylineToActionAndView(bool value);
    void setSnapToCornerArcCenter(bool value);
};

#endif // LC_LINERECTANGLEFIXEDOPTIONS_H
