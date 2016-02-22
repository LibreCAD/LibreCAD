#include "lg_dimzerosbox.h"
#include <QTableView>
#include <QListWidgetItem>

LG_DimzerosBox::LG_DimzerosBox(QWidget *parent) : QComboBox(parent) {
    dimLine = false;
    view = new QListView();
    model = new QStandardItemModel(3, 1);
    QStandardItem* item = new QStandardItem(tr("select:"));
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    model->setItem(0,0,item);
    item = new QStandardItem(tr("remove left"));
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setData(Qt::Unchecked, Qt::CheckStateRole);
    model->setItem(1,0,item);
    item = new QStandardItem(tr("remove right"));
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setData(Qt::Unchecked, Qt::CheckStateRole);
    model->setItem(2,0,item);

    setModel(model);
    setView(view);
    setEditable(false);
    setEditText("selectar:");
}

LG_DimzerosBox::~LG_DimzerosBox() {
    delete model;
    delete view;
}

void LG_DimzerosBox::setLinear(){
    dimLine = true;
    QStandardItem* item = new QStandardItem(tr("remove 0'"));
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setData(Qt::Unchecked, Qt::CheckStateRole);
    model->appendRow(item);
    item = new QStandardItem(tr("remove 0\""));
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setData(Qt::Unchecked, Qt::CheckStateRole);
    model->appendRow(item);
}

void LG_DimzerosBox::setData(int i){
    if (dimLine) {
        if (i & 1) {
            if (i&2)
                model->item(3)->setCheckState(Qt::Checked);
        } else {
            model->item(4)->setCheckState(Qt::Checked);
            if (!(i&2))
                model->item(3)->setCheckState(Qt::Checked);
        }
        if (i&4)
            model->item(1)->setCheckState(Qt::Checked);
        if (i&8)
            model->item(2)->setCheckState(Qt::Checked);
    } else {
        if (i & 1)
            model->item(1)->setCheckState(Qt::Checked);
        if (i & 2)
            model->item(2)->setCheckState(Qt::Checked);
    }
}

int LG_DimzerosBox::getData(){
    int ret = 0;
    if (dimLine){
        if (model->item(1)->checkState() == Qt::Checked)
            ret |= 4;
        if (model->item(2)->checkState() == Qt::Checked)
            ret |= 8;
        //imperial:
        if (model->item(3)->checkState() == Qt::Checked){
            if (model->item(4)->checkState() == Qt::Unchecked)
                ret |= 3;
        } else {
            if (model->item(4)->checkState() == Qt::Checked)
                ret |= 2;
            else
                ret |= 1;
        }
    } else {
        if (model->item(1)->checkState() == Qt::Checked)
            ret |= 1;
        if (model->item(2)->checkState() == Qt::Checked)
            ret |= 2;
    }
    return ret;
}

/**
 * helper function for DIMZIN var.
 */
int LG_DimzerosBox::convertDimZin(int v, bool toIdx){
    if (toIdx){
        if (v < 5)
            return 0;
        int res = 0;
        if (v & 4)
            res = 3;
        if (v & 8)
            return (res==3) ? 5 :4;
    }
    //toIdx false
    switch (v) {
        case 3:
            return 4;
            break;
        case 4:
            return 8;
            break;
        case 5:
            return 12;
            break;
        default:
            break;
    }
    return 1;
}


/*MyModel::MyModel(QObject *parent)
    :QAbstractTableModel(parent)
{
}

int MyModel::rowCount(const QModelIndex & parent) const
{
   return 2;
}

int MyModel::columnCount(const QModelIndex & parent) const
{
    return 3;
}

QVariant MyModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole)
    {
       return QString("Row%1, Column%2")
                   .arg(index.row() + 1)
                   .arg(index.column() +1);
    }
    return QVariant();
}*/
