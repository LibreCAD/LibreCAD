#ifndef LC_SLICEDIVIDEOPTIONS_H
#define LC_SLICEDIVIDEOPTIONS_H

#include <QWidget>
#include "rs_actioninterface.h"
#include "lc_actiondrawslicedivide.h"

namespace Ui {
class LC_SliceDivideOptions;
}

class LC_SliceDivideOptions : public LC_ActionOptionsWidget
{
    Q_OBJECT

public:
    explicit LC_SliceDivideOptions(QWidget *parent = nullptr);
    ~LC_SliceDivideOptions();

public slots:

    void onCountEditingFinished();
    void onTickLengthEditingFinished();
    void onTickAngleEditingFinished();
    void onTickOffsetEditingFinished();
    void onCircleStartAngleEditingFinished();
    void onDrawTickOnEdgesIndexChanged(int index);
    void onTickSnapIndexChanged(int index);
    void onRelAngleClicked(bool checked);
    void onDivideClicked(bool checked);

protected slots:
    virtual void languageChange() override;
    void clearAction() override;
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
private:
    Ui::LC_SliceDivideOptions *ui;

    void saveSettings() override;
    void setCountToActionAndView(const QString &val);
    void setTickLengthToActionAndView(const QString &qString);
    void setTickAngleToActionAndView(const QString &val);
    void setTickOffsetToActionAndView(const QString &val);
    void setDrawEdgesTicksModeToActionAndView(int index);
    void setCircleStartAngleToActionAndView(const QString &val);

    LC_ActionDrawSliceDivide* action;
    void setTickAngleRelativeToActionAndView(bool relative);    
    void setTicksSnapModeToActionAndView(int index);
    void setDivideFlagToActionAndView(bool value);
};

#endif // LC_SLICEDIVIDEOPTIONS_H
