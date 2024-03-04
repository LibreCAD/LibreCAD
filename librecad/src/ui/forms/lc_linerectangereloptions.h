#ifndef LC_LINERECTANGERELOPTIONS_H
#define LC_LINERECTANGERELOPTIONS_H

#include <QWidget>

namespace Ui {
class LC_LineRectangeRelOptions;
}

class LC_LineRectangeRelOptions : public QWidget
{
    Q_OBJECT

public:
    explicit LC_LineRectangeRelOptions(QWidget *parent = nullptr);
    ~LC_LineRectangeRelOptions();

private:
    Ui::LC_LineRectangeRelOptions *ui;
};

#endif // LC_LINERECTANGERELOPTIONS_H
