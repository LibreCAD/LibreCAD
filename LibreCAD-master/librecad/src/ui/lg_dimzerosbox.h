#ifndef LG_DIMZEROSBOX_H
#define LG_DIMZEROSBOX_H

#include <QComboBox>
//#include <QAbstractTableModel>
#include <QStandardItemModel>
#include <QListView>

/*
 * DimZin value is mixed integer and bit flag value
 * inches and feets are integer values, removal of left and right zeros are flags
 * 0: removes 0' & 0"
 * 1: draw 0' & 0"
 * 2: removes 0"
 * 4: removes 0'
 * bit 3 set (4) remove 0 to the left
 * bit 4 set (8) removes 0's to the right
 *
 * DimAZin value is integer or bit flag value
 * 0: draw all
 * 1: remove 0 to the left
 * 2: removes 0's to the right
 * 3: removes all zeros
*/

class LG_DimzerosBox : public QComboBox {
    Q_OBJECT

public:
    explicit LG_DimzerosBox(QWidget *parent = 0);
    ~LG_DimzerosBox();
    void setLinear();
    void setData(int i);
    int getData();
private:
    QStandardItemModel *model;
    QListView *view;
    bool dimLine;
    int convertDimZin(int v, bool toIdx);
};

#endif // LG_DIMZEROSBOX_H
