#ifndef LC_ACTIONDRAWARC2POPTIONS_H
#define LC_ACTIONDRAWARC2POPTIONS_H

#include <QLabel>
#include "lc_actionoptionswidget.h"

namespace Ui {
    class LC_ActionDrawArc2POptions;
}

class RS_ActionInterface;
class LC_ActionDrawArc2PointsBase;

class LC_ActionDrawArc2POptions : public LC_ActionOptionsWidget{
    Q_OBJECT
public:
    explicit LC_ActionDrawArc2POptions(int actionType);
    ~LC_ActionDrawArc2POptions() override;
public slots:
    void onDirectionChanged(bool);
    void languageChange() override;
    void onValueChanged();
protected:
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
    void setReversedToActionAndView(bool reversed);
    bool checkActionRttiValid(RS2::ActionType actionType) override;
    QString getSettingsOptionNamePrefix() override;
private:
    Ui::LC_ActionDrawArc2POptions *ui;
    LC_ActionDrawArc2PointsBase* m_action = nullptr;
    int m_supportedActionType;
    QString m_optionNamePrefix;
    void updateTooltip( QLabel *label) const;
    void setParameterToActionAndView(QString val);
};

#endif // LC_ACTIONDRAWARC2POPTIONS_H
