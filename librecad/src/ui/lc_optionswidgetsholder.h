#ifndef LC_OPTIONSWIDGETSHOLDER_H
#define LC_OPTIONSWIDGETSHOLDER_H

#include <QWidget>
#include "lc_snapoptionswidgetsholder.h"

namespace Ui {
    class LC_OptionsWidgetsHolder;
}
class LC_OptionsWidgetsHolder : public QWidget{
    Q_OBJECT

public:
    explicit LC_OptionsWidgetsHolder(QWidget *parent = nullptr);
    ~LC_OptionsWidgetsHolder();
    void addOptionsWidget(QWidget* optionsWidget);
    void removeOptionsWidget(QWidget* optionsWidget);
    LC_SnapOptionsWidgetsHolder *getSnapOptionsHolder();
private:
    Ui::LC_OptionsWidgetsHolder *ui;
};

#endif // LC_OPTIONSWIDGETSHOLDER_H
