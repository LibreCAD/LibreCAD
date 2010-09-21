#ifndef QG_BLOCKDIALOG_H
#define QG_BLOCKDIALOG_H

#include <qvariant.h>


#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include "rs.h"
#include "rs_block.h"
#include "rs_blocklist.h"

QT_BEGIN_NAMESPACE

class Ui_QG_BlockDialog
{
public:
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QLabel *lName;
    QSpacerItem *spacer87;
    QLineEdit *leName;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *spacer1;
    QPushButton *bOk;
    QPushButton *bCancel;

    void setupUi(QDialog *QG_BlockDialog)
    {
        if (QG_BlockDialog->objectName().isEmpty())
            QG_BlockDialog->setObjectName(QString::fromUtf8("QG_BlockDialog"));
        QG_BlockDialog->resize(253, 79);
        QG_BlockDialog->setBaseSize(QSize(0, 0));
        QG_BlockDialog->setSizeGripEnabled(false);
        vboxLayout = new QVBoxLayout(QG_BlockDialog);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lName = new QLabel(QG_BlockDialog);
        lName->setObjectName(QString::fromUtf8("lName"));
        lName->setFrameShape(QFrame::NoFrame);
        lName->setFrameShadow(QFrame::Plain);
        lName->setWordWrap(false);

        hboxLayout->addWidget(lName);

        spacer87 = new QSpacerItem(29, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacer87);

        leName = new QLineEdit(QG_BlockDialog);
        leName->setObjectName(QString::fromUtf8("leName"));

        hboxLayout->addWidget(leName);


        vboxLayout->addLayout(hboxLayout);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        spacer1 = new QSpacerItem(79, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacer1);

        bOk = new QPushButton(QG_BlockDialog);
        bOk->setObjectName(QString::fromUtf8("bOk"));
        bOk->setAutoDefault(true);
        bOk->setDefault(true);

        hboxLayout1->addWidget(bOk);

        bCancel = new QPushButton(QG_BlockDialog);
        bCancel->setObjectName(QString::fromUtf8("bCancel"));
        bCancel->setAutoDefault(true);

        hboxLayout1->addWidget(bCancel);


        vboxLayout->addLayout(hboxLayout1);


        retranslateUi(QG_BlockDialog);
        QObject::connect(bCancel, SIGNAL(clicked()), QG_BlockDialog, SLOT(reject()));
        QObject::connect(bOk, SIGNAL(clicked()), QG_BlockDialog, SLOT(validate()));

        QMetaObject::connectSlotsByName(QG_BlockDialog);
    } // setupUi

    void retranslateUi(QDialog *QG_BlockDialog)
    {
        QG_BlockDialog->setWindowTitle(QApplication::translate("QG_BlockDialog", "Block Settings", 0, QApplication::UnicodeUTF8));
        lName->setText(QApplication::translate("QG_BlockDialog", "Block Name:", 0, QApplication::UnicodeUTF8));
        leName->setText(QString());
        bOk->setText(QApplication::translate("QG_BlockDialog", "&OK", 0, QApplication::UnicodeUTF8));
        bOk->setShortcut(QApplication::translate("QG_BlockDialog", "Alt+O", 0, QApplication::UnicodeUTF8));
        bCancel->setText(QApplication::translate("QG_BlockDialog", "Cancel", 0, QApplication::UnicodeUTF8));
        bCancel->setShortcut(QApplication::translate("QG_BlockDialog", "Esc", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_BlockDialog: public Ui_QG_BlockDialog {};
} // namespace Ui

QT_END_NAMESPACE

class QG_BlockDialog : public QDialog, public Ui::QG_BlockDialog
{
    Q_OBJECT

public:
    QG_BlockDialog(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_BlockDialog();

    virtual RS_BlockData getBlockData();

public slots:
    virtual void setBlockList( RS_BlockList * l );
    virtual void validate();
    virtual void cancel();

protected:
    RS_BlockList* blockList;

protected slots:
    virtual void languageChange();

};

#endif // QG_BLOCKDIALOG_H
