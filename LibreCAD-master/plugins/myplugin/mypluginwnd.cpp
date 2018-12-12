#include "mypluginwnd.h"
#include "qpoint.h"
#include <QIntValidator>


MyPluginWnd::MyPluginWnd(QWidget* parent):
    QDialog(parent)
{
    //doc = d;
    this->setWindowTitle(tr("draw wnd"));

    showWidget();

    connect(button,SIGNAL(clicked(bool)),this,SLOT(draw()));
    connect(button1,SIGNAL(clicked(bool)),this,SLOT(offset()));
    //connect()
}



vector<QPointF> MyPluginWnd::getData()
{
    return vec;
}

vector<QPointF> MyPluginWnd::getOffsetData()
{
    return vec1;
}

void MyPluginWnd::draw()
{
    double x1 = edit_x1->text().toDouble();
    double y1 = edit_y1->text().toDouble();
    double x2 = edit_x2->text().toDouble();
    double y2 = edit_y2->text().toDouble();

    double x3 = edit_x3->text().toDouble();
    double y3 = edit_y3->text().toDouble();
    double x4 = edit_x4->text().toDouble();
    double y4 = edit_y4->text().toDouble();
    vec.emplace_back(QPointF(x1,y1));
    vec.emplace_back(QPointF(x2,y2));

    vec.emplace_back(QPointF(x3,y3));
    vec.emplace_back(QPointF(x4,y4));

    //vec.emplace_back(vec.front());

    offset();
    this->accept();
}


void MyPluginWnd::offset()
{
    Path subj;
    Paths solution;
    int off_val = offsetVal->text().toDouble();

    if(!vec.empty())
    {
        for(QPointF point : vec)
        {
            subj << IntPoint(point.x(),point.y());
        }

        ClipperOffset co;
        co.AddPath(subj,jtRound,etClosedPolygon);
        co.Execute(solution,off_val);
    }

    for(Path path : solution)
        for(IntPoint point : path)
        {
            vec1.emplace_back(QPointF(point.X,point.Y));
        }

    this->accept();
}

void MyPluginWnd::showWidget()
{
    edit_x1 = new QLineEdit;
    edit_y1 = new QLineEdit;
    edit_x2 = new QLineEdit;
    edit_y2 = new QLineEdit;

    edit_x3 = new QLineEdit;
    edit_y3 = new QLineEdit;
    edit_x4 = new QLineEdit;
    edit_y4 = new QLineEdit;

    offsetVal = new QLineEdit;

    edit_x1->setValidator(new QIntValidator(0,10000,this));
    edit_y1->setValidator(new QIntValidator(0,10000,this));
    edit_x2->setValidator(new QIntValidator(0,10000,this));
    edit_y2->setValidator(new QIntValidator(0,10000,this));
    edit_x3->setValidator(new QIntValidator(0,10000,this));
    edit_y3->setValidator(new QIntValidator(0,10000,this));
    edit_x4->setValidator(new QIntValidator(0,10000,this));
    edit_y4->setValidator(new QIntValidator(0,10000,this));
    offsetVal->setValidator(new QIntValidator(-100,100,this));

    edit_x1->setText(QString("0"));
    edit_y1->setText(QString("0"));
    edit_x2->setText(QString("400"));
    edit_y2->setText(QString("0"));
    edit_x3->setText(QString("400"));
    edit_y3->setText(QString("300"));
    edit_x4->setText(QString("0"));
    edit_y4->setText(QString("300"));
    offsetVal->setText(QString("50"));

    button = new QPushButton;
    button->setText(tr("Draw"));

    button1 = new QPushButton;
    button1->setText(tr("Offset"));

    layout = new QGridLayout;
    layout->addWidget(new QLabel(tr("x1: ")),0,0);
    layout->addWidget(edit_x1,0,1);

    layout->addWidget(new QLabel(tr("y1: ")),0,2);
    layout->addWidget(edit_y1,0,3);

    layout->addWidget(new QLabel(tr("x2: ")),1,0);
    layout->addWidget(edit_x2,1,1);

    layout->addWidget(new QLabel(tr("y2: ")),1,2);
    layout->addWidget(edit_y2,1,3);

    layout->addWidget(new QLabel(tr("x3: ")),2,0);
    layout->addWidget(edit_x3,2,1);

    layout->addWidget(new QLabel(tr("y3: ")),2,2);
    layout->addWidget(edit_y3,2,3);

    layout->addWidget(new QLabel(tr("x4: ")),3,0);
    layout->addWidget(edit_x4,3,1);

    layout->addWidget(new QLabel(tr("y2: ")),3,2);
    layout->addWidget(edit_y4,3,3);

    layout->addWidget(new QLabel(tr("offsetVal: ")),4,0);
    layout->addWidget(offsetVal,4,1);

    layout->addWidget(button1,5,0,Qt::AlignLeft);
    layout->addWidget(button,5,1,Qt::AlignRight);

    this->setLayout(layout);
}


