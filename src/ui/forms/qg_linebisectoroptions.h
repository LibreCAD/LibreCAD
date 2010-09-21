#ifndef QG_LINEBISECTOROPTIONS_H
#define QG_LINEBISECTOROPTIONS_H

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
#include <QtGui/QLineEdit>
#include <QtGui/QSpinBox>
#include <QtGui/QWidget>
#include "rs_actiondrawlinebisector.h"
#include "rs_line.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_LineBisectorOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lLength;
    QLineEdit *leLength;
    QLabel *lNumber;
    QSpinBox *sbNumber;
    QFrame *sep1;

    void setupUi(QWidget *QG_LineBisectorOptions)
    {
        if (QG_LineBisectorOptions->objectName().isEmpty())
            QG_LineBisectorOptions->setObjectName(QString::fromUtf8("QG_LineBisectorOptions"));
        QG_LineBisectorOptions->resize(280, 22);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(0));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_LineBisectorOptions->sizePolicy().hasHeightForWidth());
        QG_LineBisectorOptions->setSizePolicy(sizePolicy);
        QG_LineBisectorOptions->setMinimumSize(QSize(220, 22));
        QG_LineBisectorOptions->setMaximumSize(QSize(280, 22));
        hboxLayout = new QHBoxLayout(QG_LineBisectorOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(1, 1, 1, 1);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lLength = new QLabel(QG_LineBisectorOptions);
        lLength->setObjectName(QString::fromUtf8("lLength"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(5));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lLength->sizePolicy().hasHeightForWidth());
        lLength->setSizePolicy(sizePolicy1);
        lLength->setWordWrap(false);

        hboxLayout->addWidget(lLength);

        leLength = new QLineEdit(QG_LineBisectorOptions);
        leLength->setObjectName(QString::fromUtf8("leLength"));

        hboxLayout->addWidget(leLength);

        lNumber = new QLabel(QG_LineBisectorOptions);
        lNumber->setObjectName(QString::fromUtf8("lNumber"));
        lNumber->setWordWrap(false);

        hboxLayout->addWidget(lNumber);

        sbNumber = new QSpinBox(QG_LineBisectorOptions);
        sbNumber->setObjectName(QString::fromUtf8("sbNumber"));
        sbNumber->setMinimum(1);

        hboxLayout->addWidget(sbNumber);

        sep1 = new QFrame(QG_LineBisectorOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy2);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_LineBisectorOptions);
        QObject::connect(leLength, SIGNAL(textChanged(QString)), QG_LineBisectorOptions, SLOT(updateLength(QString)));
        QObject::connect(sbNumber, SIGNAL(valueChanged(int)), QG_LineBisectorOptions, SLOT(updateNumber(int)));

        QMetaObject::connectSlotsByName(QG_LineBisectorOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_LineBisectorOptions)
    {
        QG_LineBisectorOptions->setWindowTitle(QApplication::translate("QG_LineBisectorOptions", "Line Bisector Options", 0, QApplication::UnicodeUTF8));
        lLength->setText(QApplication::translate("QG_LineBisectorOptions", "Length:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        leLength->setProperty("toolTip", QVariant(QApplication::translate("QG_LineBisectorOptions", "Length of bisector", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        lNumber->setText(QApplication::translate("QG_LineBisectorOptions", "Number:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        sbNumber->setProperty("toolTip", QVariant(QApplication::translate("QG_LineBisectorOptions", "Number of bisectors to create", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

namespace Ui {
    class QG_LineBisectorOptions: public Ui_QG_LineBisectorOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_LineBisectorOptions : public QWidget, public Ui::QG_LineBisectorOptions
{
    Q_OBJECT

public:
    QG_LineBisectorOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_LineBisectorOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateLength( const QString & l );
    virtual void updateNumber( int n );

protected:
    RS_ActionDrawLineBisector* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_LINEBISECTOROPTIONS_H
