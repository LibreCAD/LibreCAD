#ifndef MYPLUGINWND_H
#define MYPLUGINWND_H

#include <QDialog>
#include <QObject>
#include <QWidget>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include "document_interface.h"
#include <vector>

#include "clipper.hpp"

using namespace ClipperLib;
using namespace std;

class MyPluginWnd : public QDialog
{
    Q_OBJECT
public:
    MyPluginWnd(QWidget* parent = nullptr);
    ~MyPluginWnd() = default;
    void showWidget();
    vector<QPointF> getData();
    vector<QPointF> getOffsetData();

private slots:
    void draw();
    void offset();



private:
    QLineEdit* edit_x1;
    QLineEdit* edit_y1;
    QLineEdit* edit_x2;
    QLineEdit* edit_y2;
    QLineEdit* edit_x3;
    QLineEdit* edit_y3;
    QLineEdit* edit_x4;
    QLineEdit* edit_y4;

    QLineEdit* offsetVal;
    QGridLayout* layout;
    QPushButton* button;
    QPushButton* button1;

    vector<QPointF> vec;
    vector<QPointF> vec1;
    //Path subj;
    //Paths solution;

    //Document_Interface* doc;


};

#endif // MYPLUGINWND_H
