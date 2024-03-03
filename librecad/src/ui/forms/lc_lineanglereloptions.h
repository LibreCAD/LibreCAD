#ifndef LC_LINEANGLERELOPTIONS_H
#define LC_LINEANGLERELOPTIONS_H

#include <QWidget>
#include "rs_actioninterface.h"
#include "lc_actiondrawlineanglerel.h"

namespace Ui {
class LC_LineAngleRelOptions;
}

class LC_LineAngleRelOptions : public QWidget
{
    Q_OBJECT

public:
    explicit LC_LineAngleRelOptions(QWidget *parent = nullptr);
    ~LC_LineAngleRelOptions() override;

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update);
    void onLengthEditingFinished();
    void onOffsetEditingFinished();
    void onAngleEditingFinished();
    void onLineSnapModeIndexChanged(int index);
    void onTickSnapModeIndexChanged(int index);
    void onAngleRelatedClicked(bool clicked);
    void onFreeLengthClicked(bool clicked);
protected:
    virtual void languageChange();
private:
    Ui::LC_LineAngleRelOptions *ui;
    LC_ActionDrawLineAngleRel* action;
    bool fixedAngle {false};

    void saveSettings();
    QString keyName(QString key);
    void setAngleToActionAndView(const QString &expr);
    void setLengthToActionAndView(QString val);
    void setOffsetToActionAndView(QString val);
    void setLineSnapModeToActionAndView(int mode);
    void setTickSnapModeToActionAndView(int mode);
    void setRelativeAngleToActionAndView(int mode);
    void setAngleIsRelativeToActionAndView(bool relative);
    void setLengthIsFreeToActionAndView(bool free);
};

#endif // LC_LINEANGLERELOPTIONS_H
