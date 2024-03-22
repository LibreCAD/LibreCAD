#ifndef LC_RECTANGLE2POINTSOPTIONS_H
#define LC_RECTANGLE2POINTSOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidget.h"
#include "lc_actiondrawrectangle2points.h"

namespace Ui {
class LC_Rectangle2PointsOptions;
}

class LC_Rectangle2PointsOptions : public LC_ActionOptionsWidget
{
    Q_OBJECT

public:
    explicit LC_Rectangle2PointsOptions(QWidget *parent = nullptr);
    ~LC_Rectangle2PointsOptions();

    void saveSettings() override;

public slots:
    void onCornersIndexChanged(int index);
    void onInsertionPointSnapIndexChanged(int index);
    void onSecondPointSnapIndexChanged(int index);
    void onEdgesIndexChanged(int index);
    void onAngleEditingFinished();
    void onLenYEditingFinished();
    void onLenXEditingFinished();
    void onRadiusEditingFinished();
    void onUsePolylineClicked(bool value);
    void onSnapToCornerArcCenterClicked(bool value);

protected slots:
    void languageChange() override;
    void doSetAction( RS_ActionInterface * a, bool update) override;
    void clearAction() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;

private:
    LC_ActionDrawRectangle2Points *action;
    void setAngleToActionAndView(const QString &val);
    void setLenYToActionAnView(QString value);
    void setLenXToActionAnView(QString value);
    void setRadiusToActionAnView(QString value);
    void setCornersModeToActionAndView(int index);
    void setInsertSnapPointModeToActionAndView(int index);
    void setSecondPointSnapPointModeToActionAndView(int index);
    void setUsePolylineToActionAndView(bool value);
    void setSnapToCornerArcCenter(bool value);

private:
    Ui::LC_Rectangle2PointsOptions *ui;
    void setEdgesModeToActionAndView(int index);
};

#endif // LC_RECTANGLE2POINTSOPTIONS_H
