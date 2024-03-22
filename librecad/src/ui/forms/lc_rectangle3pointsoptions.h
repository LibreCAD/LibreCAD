#ifndef LC_RECTANGLE3POINTSOPTIONS_H
#define LC_RECTANGLE3POINTSOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidget.h"
#include "lc_actiondrawrectangle3points.h"

namespace Ui {
class LC_Rectangle3PointsOptions;
}

class LC_Rectangle3PointsOptions : public LC_ActionOptionsWidget
{
    Q_OBJECT
protected:
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void clearAction() override;

public:
    explicit LC_Rectangle3PointsOptions(QWidget *parent = nullptr);
    ~LC_Rectangle3PointsOptions();
    void saveSettings() override;
    void languageChange() override;
protected slots:
    void onAngleEditingFinished();
    void onCornersIndexChanged(int index);
    void onLenYEditingFinished();
    void onLenXEditingFinished();
    void onRadiusEditingFinished();
    void onUsePolylineClicked(bool value);
    void onSnapToCornerArcCenterClicked(bool value);
    void onQuadrangleClicked(bool value);
    void onInnerAngleEditingFinished();
    void onInnerAngleFixedClicked(bool value);
    void onEdgesIndexChanged(int index);
private:
    LC_ActionDrawRectangle3Points* action;
    Ui::LC_Rectangle3PointsOptions *ui;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    void setSnapToCornerArcCenter(bool value);
    void setUsePolylineToActionAndView(bool value);
    void setRadiusToActionAnView(QString value);
    void setLenXToActionAnView(QString value);
    void setLenYToActionAnView(QString value);
    void setAngleToActionAndView(const QString &val);
    void setCornersModeToActionAndView(int index);
    void setQuadrangleToActionAndView(bool value);
    void setInnerAngleFixedToActionAndView(bool angle);
    void setInnerAngleToActionAndView(QString value);
    void setEdgesModeToActionAndView(int index);
};

#endif // LC_RECTANGLE3POINTSOPTIONS_H
