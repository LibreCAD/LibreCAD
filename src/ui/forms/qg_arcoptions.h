#ifndef QG_ARCOPTIONS_H
#define QG_ARCOPTIONS_H

#include <qvariant.h>


#include <Qt3Support/Q3ButtonGroup>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHeaderView>
#include <QtGui/QRadioButton>
#include <QtGui/QWidget>
#include "rs_actiondrawarc.h"
#include "rs_arc.h"

QT_BEGIN_NAMESPACE

class Ui_QG_ArcOptions
{
public:
    Q3ButtonGroup *buttonGroup1;
    QRadioButton *rbNeg;
    QRadioButton *rbPos;
    QFrame *sep1;

    void setupUi(QWidget *QG_ArcOptions)
    {
        if (QG_ArcOptions->objectName().isEmpty())
            QG_ArcOptions->setObjectName(QString::fromUtf8("QG_ArcOptions"));
        QG_ArcOptions->resize(90, 22);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_ArcOptions->sizePolicy().hasHeightForWidth());
        QG_ArcOptions->setSizePolicy(sizePolicy);
        QG_ArcOptions->setMinimumSize(QSize(90, 22));
        buttonGroup1 = new Q3ButtonGroup(QG_ArcOptions);
        buttonGroup1->setObjectName(QString::fromUtf8("buttonGroup1"));
        buttonGroup1->setGeometry(QRect(0, 0, 78, 19));
        buttonGroup1->setLineWidth(0);
        buttonGroup1->setFlat(true);
        rbNeg = new QRadioButton(buttonGroup1);
        rbNeg->setObjectName(QString::fromUtf8("rbNeg"));
        rbNeg->setGeometry(QRect(38, 2, 36, 19));
        rbNeg->setIcon(qt_get_icon(image0_ID));
        rbPos = new QRadioButton(buttonGroup1);
        rbPos->setObjectName(QString::fromUtf8("rbPos"));
        rbPos->setGeometry(QRect(2, 2, 36, 19));
        rbPos->setIcon(qt_get_icon(image1_ID));
        rbPos->setChecked(true);
        sep1 = new QFrame(QG_ArcOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        sep1->setGeometry(QRect(80, 2, 6, 19));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy1);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        retranslateUi(QG_ArcOptions);
        QObject::connect(rbPos, SIGNAL(toggled(bool)), QG_ArcOptions, SLOT(updateDirection(bool)));
        QObject::connect(rbNeg, SIGNAL(toggled(bool)), QG_ArcOptions, SLOT(updateDirection(bool)));

        QMetaObject::connectSlotsByName(QG_ArcOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_ArcOptions)
    {
        QG_ArcOptions->setWindowTitle(QApplication::translate("QG_ArcOptions", "Arc Options", 0, QApplication::UnicodeUTF8));
        buttonGroup1->setTitle(QString());
        rbNeg->setText(QString());
#ifndef QT_NO_TOOLTIP
        rbNeg->setProperty("toolTip", QVariant(QApplication::translate("QG_ArcOptions", "Clockwise", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        rbPos->setText(QString());
#ifndef QT_NO_TOOLTIP
        rbPos->setProperty("toolTip", QVariant(QApplication::translate("QG_ArcOptions", "Counter Clockwise", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
    } // retranslateUi


protected:
    enum IconID
    {
        image0_ID,
        image1_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    static const char* const image0_data[] = { 
"14 15 3 1",
". c None",
"a c #000000",
"# c #ff0000",
".............#",
"....aaaaaa..##",
"...a......a###",
"..a.......####",
".a.......#####",
"a.......######",
"a.............",
"a.............",
"a.............",
"a............a",
"a............a",
".a..........a.",
"..a........a..",
"...a......a...",
"....aaaaaa...."};


    static const char* const image1_data[] = { 
"14 15 3 1",
". c None",
"# c #000000",
"a c #ff0000",
"....######....",
"...#......#...",
"..#........#..",
".#..........#.",
"#............#",
"#............#",
"#.............",
"#.............",
"#.............",
"#.......aaaaaa",
".#.......aaaaa",
"..#.......aaaa",
"...#......#aaa",
"....######..aa",
".............a"};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        case image1_ID: return QPixmap((const char**)image1_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_ArcOptions: public Ui_QG_ArcOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_ArcOptions : public QWidget, public Ui::QG_ArcOptions
{
    Q_OBJECT

public:
    QG_ArcOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_ArcOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateDirection( bool );

protected:
    RS_ActionDrawArc* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_ARCOPTIONS_H
