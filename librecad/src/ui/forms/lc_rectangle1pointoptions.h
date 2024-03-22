#ifndef LC_RECTANGLE1POINTOPTIONS_H
#define LC_RECTANGLE1POINTOPTIONS_H

#include <QWidget>
#include "rs_actioninterface.h"
#include "lc_actiondrawrectangle1point.h"

namespace Ui {
class LC_Rectangle1PointOptions;
}

class LC_Rectangle1PointOptions :public LC_ActionOptionsWidget {
    Q_OBJECT

public:
    explicit LC_Rectangle1PointOptions(QWidget *parent = nullptr);
    ~LC_Rectangle1PointOptions() override;

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
    void onInnerSizeClicked(bool value);
    void onEdgesIndexChanged(int index);

protected slots:
    void languageChange() override;
    void doSetAction( RS_ActionInterface * a, bool update) override;
    void clearAction() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
private:
    Ui::LC_Rectangle1PointOptions *ui;

    LC_ActionDrawRectangle1Point *action;
    void setAngleToActionAndView(const QString &val);
    void setLenYToActionAnView(QString value);
    void setLenXToActionAnView(QString value);
    void setRadiusToActionAnView(QString value);
    void setHeightToActionAnView(QString height);
    void setWidthToActionAnView(QString width);
    void setCornersModeToActionAndView(int index);
    void setSnapPointModeToActionAndView(int index);
    void setUsePolylineToActionAndView(bool value);
    void setSnapToCornerArcCenterToActionAndView(bool value);
    void setSizeInnerToActionAndView(bool value);
    void setEdgesModeToActionAndView(int index);
};

#endif // LC_RECTANGLE1POINTOPTIONS_H
