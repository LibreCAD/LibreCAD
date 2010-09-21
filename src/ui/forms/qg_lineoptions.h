#ifndef QG_LINEOPTIONS_H
#define QG_LINEOPTIONS_H

#include <qvariant.h>


#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>
#include "rs_actiondrawline.h"
#include "rs_line.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_LineOptions
{
public:
    QHBoxLayout *hboxLayout;
    QToolButton *bClose;
    QToolButton *bUndo;
    QFrame *sep1;

    void setupUi(QWidget *QG_LineOptions)
    {
        if (QG_LineOptions->objectName().isEmpty())
            QG_LineOptions->setObjectName(QString::fromUtf8("QG_LineOptions"));
        QG_LineOptions->resize(200, 22);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(5));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_LineOptions->sizePolicy().hasHeightForWidth());
        QG_LineOptions->setSizePolicy(sizePolicy);
        QG_LineOptions->setMinimumSize(QSize(200, 22));
        QG_LineOptions->setMaximumSize(QSize(280, 22));
        hboxLayout = new QHBoxLayout(QG_LineOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        bClose = new QToolButton(QG_LineOptions);
        bClose->setObjectName(QString::fromUtf8("bClose"));

        hboxLayout->addWidget(bClose);

        bUndo = new QToolButton(QG_LineOptions);
        bUndo->setObjectName(QString::fromUtf8("bUndo"));

        hboxLayout->addWidget(bUndo);

        sep1 = new QFrame(QG_LineOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy1);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_LineOptions);
        QObject::connect(bClose, SIGNAL(clicked()), QG_LineOptions, SLOT(close()));
        QObject::connect(bUndo, SIGNAL(clicked()), QG_LineOptions, SLOT(undo()));

        QMetaObject::connectSlotsByName(QG_LineOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_LineOptions)
    {
        QG_LineOptions->setWindowTitle(QApplication::translate("QG_LineOptions", "Line Options", 0, QApplication::UnicodeUTF8));
        bClose->setText(QApplication::translate("QG_LineOptions", "Close", 0, QApplication::UnicodeUTF8));
        bUndo->setText(QApplication::translate("QG_LineOptions", "Undo", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_LineOptions: public Ui_QG_LineOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_LineOptions : public QWidget, public Ui::QG_LineOptions
{
    Q_OBJECT

public:
    QG_LineOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_LineOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a );
    virtual void close();
    virtual void undo();

protected:
    RS_ActionDrawLine* action;

protected slots:
    virtual void languageChange();

};

#endif // QG_LINEOPTIONS_H
