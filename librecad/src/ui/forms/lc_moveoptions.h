#ifndef LC_MOVEOPTIONS_H
#define LC_MOVEOPTIONS_H

#include <QWidget>
#include "lc_actionoptionswidgetbase.h"
#include "rs_actionmodifymove.h"

namespace Ui {
    class LC_MoveOptions;
}

class LC_MoveOptions : public LC_ActionOptionsWidgetBase{
    Q_OBJECT

public:
    explicit LC_MoveOptions();
    ~LC_MoveOptions() override;
public slots:
    void cbKeepOriginalsClicked(bool val);
    void cbMultipleCopiesClicked(bool val);
    void cbUseCurrentAttributesClicked(bool val);
    void cbUseCurrentLayerClicked(bool val);
    void on_sbNumberOfCopies_valueChanged(int number);
    void languageChange() override;
protected:
    void doSaveSettings() override;
    void doSetAction(RS_ActionInterface *a, bool update) override;
private:
    Ui::LC_MoveOptions *ui;
    RS_ActionModifyMove* action;
    void setCopiesNumberToActionAndView(int number);
    void setUseMultipleCopiesToActionAndView(bool copies);
    void setUseCurrentLayerToActionAndView(bool val);
    void setUseCurrentAttributesToActionAndView(bool val);
    void setKeepOriginalsToActionAndView(bool val);
};

#endif // LC_MOVEOPTIONS_H
