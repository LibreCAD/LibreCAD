#ifndef QG_DLGOPTIONSVARIABLES_H
#define QG_DLGOPTIONSVARIABLES_H

#include <qvariant.h>


#include <Qt3Support/Q3Header>
#include <Qt3Support/Q3MimeSourceFactory>
#include <Qt3Support/Q3Table>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include "rs_filterdxf.h"
#include "rs_graphic.h"
#include "rs_math.h"
#include "rs_system.h"

QT_BEGIN_NAMESPACE

class Ui_QG_DlgOptionsVariables
{
public:
    QVBoxLayout *vboxLayout;
    Q3Table *tabVariables;
    QHBoxLayout *hboxLayout;
    QSpacerItem *Horizontal_Spacing2;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;

    void setupUi(QDialog *QG_DlgOptionsVariables)
    {
        if (QG_DlgOptionsVariables->objectName().isEmpty())
            QG_DlgOptionsVariables->setObjectName(QString::fromUtf8("QG_DlgOptionsVariables"));
        QG_DlgOptionsVariables->resize(439, 312);
        QG_DlgOptionsVariables->setSizeGripEnabled(true);
        vboxLayout = new QVBoxLayout(QG_DlgOptionsVariables);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        tabVariables = new Q3Table(QG_DlgOptionsVariables);
        tabVariables->setObjectName(QString::fromUtf8("tabVariables"));
        tabVariables->setResizePolicy(Q3Table::Default);
        tabVariables->setVScrollBarMode(Q3Table::AlwaysOn);
        tabVariables->setHScrollBarMode(Q3Table::AlwaysOff);
        tabVariables->setNumRows(0);
        tabVariables->setNumCols(3);
        tabVariables->setShowGrid(true);
        tabVariables->setSelectionMode(Q3Table::NoSelection);

        vboxLayout->addWidget(tabVariables);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        Horizontal_Spacing2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(Horizontal_Spacing2);

        buttonOk = new QPushButton(QG_DlgOptionsVariables);
        buttonOk->setObjectName(QString::fromUtf8("buttonOk"));
        buttonOk->setAutoDefault(true);
        buttonOk->setDefault(true);

        hboxLayout->addWidget(buttonOk);

        buttonCancel = new QPushButton(QG_DlgOptionsVariables);
        buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));
        buttonCancel->setAutoDefault(true);

        hboxLayout->addWidget(buttonCancel);


        vboxLayout->addLayout(hboxLayout);

        QWidget::setTabOrder(buttonOk, buttonCancel);

        retranslateUi(QG_DlgOptionsVariables);
        QObject::connect(buttonCancel, SIGNAL(clicked()), QG_DlgOptionsVariables, SLOT(reject()));

        QMetaObject::connectSlotsByName(QG_DlgOptionsVariables);
    } // setupUi

    void retranslateUi(QDialog *QG_DlgOptionsVariables)
    {
        QG_DlgOptionsVariables->setWindowTitle(QApplication::translate("QG_DlgOptionsVariables", "Drawing Variables", 0, QApplication::UnicodeUTF8));
        tabVariables->horizontalHeader()->setLabel(0, QApplication::translate("QG_DlgOptionsVariables", "Variable", 0, QApplication::UnicodeUTF8));
        tabVariables->horizontalHeader()->setLabel(1, QApplication::translate("QG_DlgOptionsVariables", "Code", 0, QApplication::UnicodeUTF8));
        tabVariables->horizontalHeader()->setLabel(2, QApplication::translate("QG_DlgOptionsVariables", "Value", 0, QApplication::UnicodeUTF8));
        buttonOk->setText(QApplication::translate("QG_DlgOptionsVariables", "&OK", 0, QApplication::UnicodeUTF8));
        buttonOk->setShortcut(QApplication::translate("QG_DlgOptionsVariables", "Alt+O", 0, QApplication::UnicodeUTF8));
        buttonCancel->setText(QApplication::translate("QG_DlgOptionsVariables", "Cancel", 0, QApplication::UnicodeUTF8));
        buttonCancel->setShortcut(QApplication::translate("QG_DlgOptionsVariables", "Esc", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_DlgOptionsVariables: public Ui_QG_DlgOptionsVariables {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DlgOptionsVariables : public QDialog, public Ui::QG_DlgOptionsVariables
{
    Q_OBJECT

public:
    QG_DlgOptionsVariables(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgOptionsVariables();

public slots:
    virtual void setGraphic( RS_Graphic * g );
    virtual void updateVariables();

protected slots:
    virtual void languageChange();

private:
    QStringList listPrec1;
    RS_Graphic* graphic;

    void init();

};

#endif // QG_DLGOPTIONSVARIABLES_H
