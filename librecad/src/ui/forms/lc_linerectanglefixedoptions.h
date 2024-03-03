#ifndef LC_LINERECTANGLEFIXEDOPTIONS_H
#define LC_LINERECTANGLEFIXEDOPTIONS_H

#include <QWidget>
#include "rs_actioninterface.h"
#include "lc_actiondrawlinerectanglefixed.h"

namespace Ui {
class LC_LineRectangleFixedOptions;
}

class LC_LineRectangleFixedOptions : public QWidget
{
    Q_OBJECT



public:
    explicit LC_LineRectangleFixedOptions(QWidget *parent = nullptr);
    ~LC_LineRectangleFixedOptions();

    virtual void setAction( RS_ActionInterface * a , bool update);

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

protected slots:
    virtual void languageChange();
private:
    Ui::LC_LineRectangleFixedOptions *ui;
    void saveSettings();

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
};

#endif // LC_LINERECTANGLEFIXEDOPTIONS_H
