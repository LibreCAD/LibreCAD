#ifndef LC_DIMSTYLEEDIT_H
#define LC_DIMSTYLEEDIT_H

#include <QWidget>

namespace Ui {
class LC_DimStyleEdit;
}

class LC_DimStyleEdit : public QWidget
{
    Q_OBJECT

public:
    explicit LC_DimStyleEdit(QWidget *parent = nullptr);
    ~LC_DimStyleEdit();

private:
    Ui::LC_DimStyleEdit *ui;
};

#endif // LC_DIMSTYLEEDIT_H
