#ifndef QG_LINEPARALLELTHROUGHOPTIONS_H
#define QG_LINEPARALLELTHROUGHOPTIONS_H

#include <qvariant.h>


#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/QWidget>
#include "rs_actiondrawlineparallelthrough.h"
#include "rs_line.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_LineParallelThroughOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lNumber;
    QSpinBox *sbNumber;
    QFrame *sep1;

    void setupUi(QWidget *QG_LineParallelThroughOptions)
    {
        if (QG_LineParallelThroughOptions->objectName().isEmpty())
            QG_LineParallelThroughOptions->setObjectName(QString::fromUtf8("QG_LineParallelThroughOptions"));
        QG_LineParallelThroughOptions->resize(140, 22);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(5));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_LineParallelThroughOptions->sizePolicy().hasHeightForWidth());
        QG_LineParallelThroughOptions->setSizePolicy(sizePolicy);
        QG_LineParallelThroughOptions->setMinimumSize(QSize(100, 22));
        QG_LineParallelThroughOptions->setMaximumSize(QSize(140, 22));
        hboxLayout = new QHBoxLayout(QG_LineParallelThroughOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lNumber = new QLabel(QG_LineParallelThroughOptions);
        lNumber->setObjectName(QString::fromUtf8("lNumber"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lNumber->sizePolicy().hasHeightForWidth());
        lNumber->setSizePolicy(sizePolicy1);
        lNumber->setWordWrap(false);

        hboxLayout->addWidget(lNumber);

        sbNumber = new QSpinBox(QG_LineParallelThroughOptions);
        sbNumber->setObjectName(QString::fromUtf8("sbNumber"));
        sbNumber->setMinimum(1);

        hboxLayout->addWidget(sbNumber);

        sep1 = new QFrame(QG_LineParallelThroughOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy2);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_LineParallelThroughOptions);
        QObject::connect(sbNumber, SIGNAL(valueChanged(int)), QG_LineParallelThroughOptions, SLOT(updateNumber(int)));

        QMetaObject::connectSlotsByName(QG_LineParallelThroughOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_LineParallelThroughOptions)
    {
        QG_LineParallelThroughOptions->setWindowTitle(QApplication::translate("QG_LineParallelThroughOptions", "Line Parallel Through Options", 0, QApplication::UnicodeUTF8));
        lNumber->setText(QApplication::translate("QG_LineParallelThroughOptions", "Number:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        sbNumber->setProperty("toolTip", QVariant(QApplication::translate("QG_LineParallelThroughOptions", "Number of parallels to create", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

namespace Ui {
    class QG_LineParallelThroughOptions: public Ui_QG_LineParallelThroughOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_LineParallelThroughOptions : public QWidget, public Ui::QG_LineParallelThroughOptions
{
    Q_OBJECT

public:
    QG_LineParallelThroughOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_LineParallelThroughOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateNumber( int n );

protected:
    RS_ActionDrawLineParallelThrough* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_LINEPARALLELTHROUGHOPTIONS_H
