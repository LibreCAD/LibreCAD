#ifndef LC_RECTANGLE3POINTSOPTIONS_H
#define LC_RECTANGLE3POINTSOPTIONS_H

#include <QWidget>

namespace Ui {
class LC_Rectangle3PointsOptions;
}

class LC_Rectangle3PointsOptions : public QWidget
{
    Q_OBJECT

public:
    explicit LC_Rectangle3PointsOptions(QWidget *parent = nullptr);
    ~LC_Rectangle3PointsOptions();

private:
    Ui::LC_Rectangle3PointsOptions *ui;
};

#endif // LC_RECTANGLE3POINTSOPTIONS_H
