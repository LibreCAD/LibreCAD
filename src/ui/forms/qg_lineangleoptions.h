#ifndef QG_LINEANGLEOPTIONS_H
#define QG_LINEANGLEOPTIONS_H

#include <qvariant.h>


#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QWidget>
#include "rs_actiondrawlineangle.h"
#include "rs_line.h"
#include "rs_settings.h"

QT_BEGIN_NAMESPACE

class Ui_QG_LineAngleOptions
{
public:
    QHBoxLayout *hboxLayout;
    QLabel *lAngle;
    QLineEdit *leAngle;
    QLabel *lLength;
    QLineEdit *leLength;
    QLabel *lSnapPoint;
    QComboBox *cbSnapPoint;
    QFrame *sep1;

    void setupUi(QWidget *QG_LineAngleOptions)
    {
        if (QG_LineAngleOptions->objectName().isEmpty())
            QG_LineAngleOptions->setObjectName(QString::fromUtf8("QG_LineAngleOptions"));
        QG_LineAngleOptions->resize(400, 22);
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(4), static_cast<QSizePolicy::Policy>(5));
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(QG_LineAngleOptions->sizePolicy().hasHeightForWidth());
        QG_LineAngleOptions->setSizePolicy(sizePolicy);
        QG_LineAngleOptions->setMinimumSize(QSize(300, 22));
        QG_LineAngleOptions->setMaximumSize(QSize(400, 22));
        hboxLayout = new QHBoxLayout(QG_LineAngleOptions);
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lAngle = new QLabel(QG_LineAngleOptions);
        lAngle->setObjectName(QString::fromUtf8("lAngle"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(1), static_cast<QSizePolicy::Policy>(0));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lAngle->sizePolicy().hasHeightForWidth());
        lAngle->setSizePolicy(sizePolicy1);
        lAngle->setWordWrap(false);

        hboxLayout->addWidget(lAngle);

        leAngle = new QLineEdit(QG_LineAngleOptions);
        leAngle->setObjectName(QString::fromUtf8("leAngle"));
        QSizePolicy sizePolicy2(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(0));
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(leAngle->sizePolicy().hasHeightForWidth());
        leAngle->setSizePolicy(sizePolicy2);

        hboxLayout->addWidget(leAngle);

        lLength = new QLabel(QG_LineAngleOptions);
        lLength->setObjectName(QString::fromUtf8("lLength"));
        sizePolicy1.setHeightForWidth(lLength->sizePolicy().hasHeightForWidth());
        lLength->setSizePolicy(sizePolicy1);
        lLength->setWordWrap(false);

        hboxLayout->addWidget(lLength);

        leLength = new QLineEdit(QG_LineAngleOptions);
        leLength->setObjectName(QString::fromUtf8("leLength"));
        sizePolicy2.setHeightForWidth(leLength->sizePolicy().hasHeightForWidth());
        leLength->setSizePolicy(sizePolicy2);

        hboxLayout->addWidget(leLength);

        lSnapPoint = new QLabel(QG_LineAngleOptions);
        lSnapPoint->setObjectName(QString::fromUtf8("lSnapPoint"));
        lSnapPoint->setWordWrap(false);

        hboxLayout->addWidget(lSnapPoint);

        cbSnapPoint = new QComboBox(QG_LineAngleOptions);
        cbSnapPoint->setObjectName(QString::fromUtf8("cbSnapPoint"));
        QFont font;
        cbSnapPoint->setFont(font);

        hboxLayout->addWidget(cbSnapPoint);

        sep1 = new QFrame(QG_LineAngleOptions);
        sep1->setObjectName(QString::fromUtf8("sep1"));
        QSizePolicy sizePolicy3(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(1));
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(sep1->sizePolicy().hasHeightForWidth());
        sep1->setSizePolicy(sizePolicy3);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);

        hboxLayout->addWidget(sep1);


        retranslateUi(QG_LineAngleOptions);
        QObject::connect(leAngle, SIGNAL(textChanged(QString)), QG_LineAngleOptions, SLOT(updateAngle(QString)));
        QObject::connect(leLength, SIGNAL(textChanged(QString)), QG_LineAngleOptions, SLOT(updateLength(QString)));
        QObject::connect(cbSnapPoint, SIGNAL(activated(int)), QG_LineAngleOptions, SLOT(updateSnapPoint(int)));

        QMetaObject::connectSlotsByName(QG_LineAngleOptions);
    } // setupUi

    void retranslateUi(QWidget *QG_LineAngleOptions)
    {
        QG_LineAngleOptions->setWindowTitle(QApplication::translate("QG_LineAngleOptions", "Line Angle Options", 0, QApplication::UnicodeUTF8));
        lAngle->setText(QApplication::translate("QG_LineAngleOptions", "Angle:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        leAngle->setProperty("toolTip", QVariant(QApplication::translate("QG_LineAngleOptions", "Line angle", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        lLength->setText(QApplication::translate("QG_LineAngleOptions", "Length:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        leLength->setProperty("toolTip", QVariant(QApplication::translate("QG_LineAngleOptions", "Length of line", 0, QApplication::UnicodeUTF8)));
#endif // QT_NO_TOOLTIP
        lSnapPoint->setText(QApplication::translate("QG_LineAngleOptions", "Snap Point:", 0, QApplication::UnicodeUTF8));
        cbSnapPoint->clear();
        cbSnapPoint->insertItems(0, QStringList()
         << QApplication::translate("QG_LineAngleOptions", "Start", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_LineAngleOptions", "Middle", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_LineAngleOptions", "End", 0, QApplication::UnicodeUTF8)
        );
    } // retranslateUi

};

namespace Ui {
    class QG_LineAngleOptions: public Ui_QG_LineAngleOptions {};
} // namespace Ui

QT_END_NAMESPACE

class QG_LineAngleOptions : public QWidget, public Ui::QG_LineAngleOptions
{
    Q_OBJECT

public:
    QG_LineAngleOptions(QWidget* parent = 0, const char* name = 0, Qt::WindowFlags fl = 0);
    ~QG_LineAngleOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateAngle( const QString & a );
    virtual void updateLength( const QString & l );
    virtual void updateSnapPoint( int sp );

protected:
    RS_ActionDrawLineAngle* action;

protected slots:
    virtual void languageChange();

private:
    void destroy();

};

#endif // QG_LINEANGLEOPTIONS_H
