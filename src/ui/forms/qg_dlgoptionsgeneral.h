#ifndef QG_DLGOPTIONSGENERAL_H
#define QG_DLGOPTIONSGENERAL_H

#include <qvariant.h>


#include <Qt3Support/Q3ButtonGroup>
#include <Qt3Support/Q3MimeSourceFactory>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QTabWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include "rs.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_units.h"

QT_BEGIN_NAMESPACE

class Ui_QG_DlgOptionsGeneral
{
public:
    QVBoxLayout *vboxLayout;
    QTabWidget *tabWidget;
    QWidget *Widget9;
    QGridLayout *gridLayout;
    Q3ButtonGroup *bgLanguage;
    QGridLayout *gridLayout1;
    QLabel *lLanguage;
    QLabel *lLanguageCmd;
    QComboBox *cbLanguageCmd;
    QComboBox *cbLanguage;
    Q3ButtonGroup *bgColors;
    QGridLayout *gridLayout2;
    QLabel *lBackground;
    QLabel *lGridColor;
    QComboBox *cbBackgroundColor;
    QComboBox *cbGridColor;
    QLabel *lMetaGridColor;
    QComboBox *cbMetaGridColor;
    QLabel *lSelectedColor;
    QComboBox *cbSelectedColor;
    QComboBox *cbHighlightedColor;
    QLabel *lHighlightedColor;
    QSpacerItem *spacer7;
    QSpacerItem *spacer8;
    Q3ButtonGroup *buttonGroup5_2;
    QGridLayout *gridLayout3;
    QLabel *lSizeStatus;
    QComboBox *cbSizeStatus;
    QSpacerItem *spacer11_3;
    QSpacerItem *spacer12;
    Q3ButtonGroup *bgGraphicView;
    QGridLayout *gridLayout4;
    QCheckBox *cbShowCrosshairs;
    QCheckBox *cbScaleGrid;
    QSpacerItem *spacer29;
    QSpacerItem *spacer29_2;
    QLabel *lMaxPreview;
    QComboBox *cbMaxPreview;
    QLabel *lMinGridSpacing;
    QComboBox *cbMinGridSpacing;
    QSpacerItem *spacer9;
    QSpacerItem *spacer4;
    QWidget *tab;
    QGridLayout *gridLayout5;
    QLabel *textLabel2;
    QLabel *textLabel3;
    QLabel *textLabel4;
    QLabel *textLabel5;
    QLabel *textLabel6;
    QLineEdit *lePathTranslations;
    QLineEdit *lePathHatch;
    QLineEdit *lePathFonts;
    QLineEdit *lePathScripts;
    QLineEdit *lePathLibrary;
    QSpacerItem *spacer5;
    QSpacerItem *spacer5_2;
    QWidget *tab1;
    QGridLayout *gridLayout6;
    Q3ButtonGroup *buttonGroup5;
    QHBoxLayout *hboxLayout;
    QLabel *lUnit;
    QComboBox *cbUnit;
    QSpacerItem *spacer11;
    QSpacerItem *spacer11_2;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *Horizontal_Spacing2;
    QPushButton *buttonOk;
    QPushButton *buttonCancel;

    void setupUi(QDialog *QG_DlgOptionsGeneral)
    {
        if (QG_DlgOptionsGeneral->objectName().isEmpty())
            QG_DlgOptionsGeneral->setObjectName(QString::fromUtf8("QG_DlgOptionsGeneral"));
        QG_DlgOptionsGeneral->resize(554, 397);
        QG_DlgOptionsGeneral->setSizeGripEnabled(true);
        vboxLayout = new QVBoxLayout(QG_DlgOptionsGeneral);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        tabWidget = new QTabWidget(QG_DlgOptionsGeneral);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        Widget9 = new QWidget();
        Widget9->setObjectName(QString::fromUtf8("Widget9"));
        gridLayout = new QGridLayout(Widget9);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        bgLanguage = new Q3ButtonGroup(Widget9);
        bgLanguage->setObjectName(QString::fromUtf8("bgLanguage"));
        bgLanguage->setColumnLayout(0, Qt::Vertical);
        bgLanguage->layout()->setSpacing(6);
        bgLanguage->layout()->setContentsMargins(11, 11, 11, 11);
        gridLayout1 = new QGridLayout();
        QBoxLayout *boxlayout = qobject_cast<QBoxLayout *>(bgLanguage->layout());
        if (boxlayout)
            boxlayout->addLayout(gridLayout1);
        gridLayout1->setAlignment(Qt::AlignTop);
        gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
        lLanguage = new QLabel(bgLanguage);
        lLanguage->setObjectName(QString::fromUtf8("lLanguage"));
        lLanguage->setWordWrap(false);

        gridLayout1->addWidget(lLanguage, 0, 0, 1, 1);

        lLanguageCmd = new QLabel(bgLanguage);
        lLanguageCmd->setObjectName(QString::fromUtf8("lLanguageCmd"));
        lLanguageCmd->setWordWrap(false);

        gridLayout1->addWidget(lLanguageCmd, 1, 0, 1, 1);

        cbLanguageCmd = new QComboBox(bgLanguage);
        cbLanguageCmd->setObjectName(QString::fromUtf8("cbLanguageCmd"));

        gridLayout1->addWidget(cbLanguageCmd, 1, 1, 1, 1);

        cbLanguage = new QComboBox(bgLanguage);
        cbLanguage->setObjectName(QString::fromUtf8("cbLanguage"));

        gridLayout1->addWidget(cbLanguage, 0, 1, 1, 1);


        gridLayout->addWidget(bgLanguage, 0, 0, 1, 1);

        bgColors = new Q3ButtonGroup(Widget9);
        bgColors->setObjectName(QString::fromUtf8("bgColors"));
        bgColors->setColumnLayout(0, Qt::Vertical);
        bgColors->layout()->setSpacing(6);
        bgColors->layout()->setContentsMargins(11, 11, 11, 11);
        gridLayout2 = new QGridLayout();
        QBoxLayout *boxlayout1 = qobject_cast<QBoxLayout *>(bgColors->layout());
        if (boxlayout1)
            boxlayout1->addLayout(gridLayout2);
        gridLayout2->setAlignment(Qt::AlignTop);
        gridLayout2->setObjectName(QString::fromUtf8("gridLayout2"));
        lBackground = new QLabel(bgColors);
        lBackground->setObjectName(QString::fromUtf8("lBackground"));
        lBackground->setWordWrap(false);

        gridLayout2->addWidget(lBackground, 0, 0, 1, 1);

        lGridColor = new QLabel(bgColors);
        lGridColor->setObjectName(QString::fromUtf8("lGridColor"));
        lGridColor->setWordWrap(false);

        gridLayout2->addWidget(lGridColor, 1, 0, 1, 1);

        cbBackgroundColor = new QComboBox(bgColors);
        cbBackgroundColor->setObjectName(QString::fromUtf8("cbBackgroundColor"));
        cbBackgroundColor->setEditable(true);

        gridLayout2->addWidget(cbBackgroundColor, 0, 1, 1, 1);

        cbGridColor = new QComboBox(bgColors);
        cbGridColor->setObjectName(QString::fromUtf8("cbGridColor"));
        cbGridColor->setEditable(true);

        gridLayout2->addWidget(cbGridColor, 1, 1, 1, 1);

        lMetaGridColor = new QLabel(bgColors);
        lMetaGridColor->setObjectName(QString::fromUtf8("lMetaGridColor"));
        lMetaGridColor->setWordWrap(false);

        gridLayout2->addWidget(lMetaGridColor, 2, 0, 1, 1);

        cbMetaGridColor = new QComboBox(bgColors);
        cbMetaGridColor->setObjectName(QString::fromUtf8("cbMetaGridColor"));
        cbMetaGridColor->setEditable(true);

        gridLayout2->addWidget(cbMetaGridColor, 2, 1, 1, 1);

        lSelectedColor = new QLabel(bgColors);
        lSelectedColor->setObjectName(QString::fromUtf8("lSelectedColor"));
        lSelectedColor->setWordWrap(false);

        gridLayout2->addWidget(lSelectedColor, 3, 0, 1, 1);

        cbSelectedColor = new QComboBox(bgColors);
        cbSelectedColor->setObjectName(QString::fromUtf8("cbSelectedColor"));
        cbSelectedColor->setEditable(true);

        gridLayout2->addWidget(cbSelectedColor, 3, 1, 1, 1);

        cbHighlightedColor = new QComboBox(bgColors);
        cbHighlightedColor->setObjectName(QString::fromUtf8("cbHighlightedColor"));
        cbHighlightedColor->setEditable(true);

        gridLayout2->addWidget(cbHighlightedColor, 4, 1, 1, 1);

        lHighlightedColor = new QLabel(bgColors);
        lHighlightedColor->setObjectName(QString::fromUtf8("lHighlightedColor"));
        lHighlightedColor->setWordWrap(false);

        gridLayout2->addWidget(lHighlightedColor, 4, 0, 1, 1);

        spacer7 = new QSpacerItem(21, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout2->addItem(spacer7, 5, 0, 1, 1);

        spacer8 = new QSpacerItem(21, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout2->addItem(spacer8, 5, 1, 1, 1);


        gridLayout->addWidget(bgColors, 0, 1, 2, 1);

        buttonGroup5_2 = new Q3ButtonGroup(Widget9);
        buttonGroup5_2->setObjectName(QString::fromUtf8("buttonGroup5_2"));
        buttonGroup5_2->setColumnLayout(0, Qt::Vertical);
        buttonGroup5_2->layout()->setSpacing(6);
        buttonGroup5_2->layout()->setContentsMargins(11, 11, 11, 11);
        gridLayout3 = new QGridLayout();
        QBoxLayout *boxlayout2 = qobject_cast<QBoxLayout *>(buttonGroup5_2->layout());
        if (boxlayout2)
            boxlayout2->addLayout(gridLayout3);
        gridLayout3->setAlignment(Qt::AlignTop);
        gridLayout3->setObjectName(QString::fromUtf8("gridLayout3"));
        lSizeStatus = new QLabel(buttonGroup5_2);
        lSizeStatus->setObjectName(QString::fromUtf8("lSizeStatus"));
        lSizeStatus->setWordWrap(false);

        gridLayout3->addWidget(lSizeStatus, 0, 0, 1, 1);

        cbSizeStatus = new QComboBox(buttonGroup5_2);
        cbSizeStatus->setObjectName(QString::fromUtf8("cbSizeStatus"));
        cbSizeStatus->setEditable(true);

        gridLayout3->addWidget(cbSizeStatus, 0, 1, 1, 1);

        spacer11_3 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout3->addItem(spacer11_3, 1, 0, 1, 1);

        spacer12 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout3->addItem(spacer12, 1, 1, 1, 1);


        gridLayout->addWidget(buttonGroup5_2, 2, 1, 1, 1);

        bgGraphicView = new Q3ButtonGroup(Widget9);
        bgGraphicView->setObjectName(QString::fromUtf8("bgGraphicView"));
        bgGraphicView->setColumnLayout(0, Qt::Vertical);
        bgGraphicView->layout()->setSpacing(6);
        bgGraphicView->layout()->setContentsMargins(11, 11, 11, 11);
        gridLayout4 = new QGridLayout();
        QBoxLayout *boxlayout3 = qobject_cast<QBoxLayout *>(bgGraphicView->layout());
        if (boxlayout3)
            boxlayout3->addLayout(gridLayout4);
        gridLayout4->setAlignment(Qt::AlignTop);
        gridLayout4->setObjectName(QString::fromUtf8("gridLayout4"));
        cbShowCrosshairs = new QCheckBox(bgGraphicView);
        cbShowCrosshairs->setObjectName(QString::fromUtf8("cbShowCrosshairs"));

        gridLayout4->addWidget(cbShowCrosshairs, 0, 0, 1, 2);

        cbScaleGrid = new QCheckBox(bgGraphicView);
        cbScaleGrid->setObjectName(QString::fromUtf8("cbScaleGrid"));

        gridLayout4->addWidget(cbScaleGrid, 1, 0, 1, 2);

        spacer29 = new QSpacerItem(21, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout4->addItem(spacer29, 4, 0, 1, 1);

        spacer29_2 = new QSpacerItem(21, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout4->addItem(spacer29_2, 4, 1, 1, 1);

        lMaxPreview = new QLabel(bgGraphicView);
        lMaxPreview->setObjectName(QString::fromUtf8("lMaxPreview"));
        lMaxPreview->setWordWrap(false);

        gridLayout4->addWidget(lMaxPreview, 3, 0, 1, 1);

        cbMaxPreview = new QComboBox(bgGraphicView);
        cbMaxPreview->setObjectName(QString::fromUtf8("cbMaxPreview"));
        cbMaxPreview->setEditable(true);

        gridLayout4->addWidget(cbMaxPreview, 3, 1, 1, 1);

        lMinGridSpacing = new QLabel(bgGraphicView);
        lMinGridSpacing->setObjectName(QString::fromUtf8("lMinGridSpacing"));
        lMinGridSpacing->setWordWrap(false);

        gridLayout4->addWidget(lMinGridSpacing, 2, 0, 1, 1);

        cbMinGridSpacing = new QComboBox(bgGraphicView);
        cbMinGridSpacing->setObjectName(QString::fromUtf8("cbMinGridSpacing"));
        cbMinGridSpacing->setEditable(true);

        gridLayout4->addWidget(cbMinGridSpacing, 2, 1, 1, 1);


        gridLayout->addWidget(bgGraphicView, 1, 0, 2, 1);

        spacer9 = new QSpacerItem(31, 16, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacer9, 3, 1, 1, 1);

        spacer4 = new QSpacerItem(31, 16, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacer4, 3, 0, 1, 1);

        tabWidget->addTab(Widget9, QString());
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        gridLayout5 = new QGridLayout(tab);
        gridLayout5->setSpacing(6);
        gridLayout5->setContentsMargins(11, 11, 11, 11);
        gridLayout5->setObjectName(QString::fromUtf8("gridLayout5"));
        textLabel2 = new QLabel(tab);
        textLabel2->setObjectName(QString::fromUtf8("textLabel2"));
        textLabel2->setWordWrap(false);

        gridLayout5->addWidget(textLabel2, 0, 0, 1, 1);

        textLabel3 = new QLabel(tab);
        textLabel3->setObjectName(QString::fromUtf8("textLabel3"));
        textLabel3->setWordWrap(false);

        gridLayout5->addWidget(textLabel3, 1, 0, 1, 1);

        textLabel4 = new QLabel(tab);
        textLabel4->setObjectName(QString::fromUtf8("textLabel4"));
        textLabel4->setWordWrap(false);

        gridLayout5->addWidget(textLabel4, 2, 0, 1, 1);

        textLabel5 = new QLabel(tab);
        textLabel5->setObjectName(QString::fromUtf8("textLabel5"));
        textLabel5->setWordWrap(false);

        gridLayout5->addWidget(textLabel5, 3, 0, 1, 1);

        textLabel6 = new QLabel(tab);
        textLabel6->setObjectName(QString::fromUtf8("textLabel6"));
        textLabel6->setWordWrap(false);

        gridLayout5->addWidget(textLabel6, 4, 0, 1, 1);

        lePathTranslations = new QLineEdit(tab);
        lePathTranslations->setObjectName(QString::fromUtf8("lePathTranslations"));

        gridLayout5->addWidget(lePathTranslations, 0, 1, 1, 1);

        lePathHatch = new QLineEdit(tab);
        lePathHatch->setObjectName(QString::fromUtf8("lePathHatch"));

        gridLayout5->addWidget(lePathHatch, 1, 1, 1, 1);

        lePathFonts = new QLineEdit(tab);
        lePathFonts->setObjectName(QString::fromUtf8("lePathFonts"));

        gridLayout5->addWidget(lePathFonts, 2, 1, 1, 1);

        lePathScripts = new QLineEdit(tab);
        lePathScripts->setObjectName(QString::fromUtf8("lePathScripts"));

        gridLayout5->addWidget(lePathScripts, 3, 1, 1, 1);

        lePathLibrary = new QLineEdit(tab);
        lePathLibrary->setObjectName(QString::fromUtf8("lePathLibrary"));

        gridLayout5->addWidget(lePathLibrary, 4, 1, 1, 1);

        spacer5 = new QSpacerItem(31, 71, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout5->addItem(spacer5, 5, 0, 1, 1);

        spacer5_2 = new QSpacerItem(31, 71, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout5->addItem(spacer5_2, 5, 1, 1, 1);

        tabWidget->addTab(tab, QString());
        tab1 = new QWidget();
        tab1->setObjectName(QString::fromUtf8("tab1"));
        gridLayout6 = new QGridLayout(tab1);
        gridLayout6->setSpacing(6);
        gridLayout6->setContentsMargins(11, 11, 11, 11);
        gridLayout6->setObjectName(QString::fromUtf8("gridLayout6"));
        buttonGroup5 = new Q3ButtonGroup(tab1);
        buttonGroup5->setObjectName(QString::fromUtf8("buttonGroup5"));
        buttonGroup5->setColumnLayout(0, Qt::Vertical);
        buttonGroup5->layout()->setSpacing(6);
        buttonGroup5->layout()->setContentsMargins(11, 11, 11, 11);
        hboxLayout = new QHBoxLayout();
        QBoxLayout *boxlayout4 = qobject_cast<QBoxLayout *>(buttonGroup5->layout());
        if (boxlayout4)
            boxlayout4->addLayout(hboxLayout);
        hboxLayout->setAlignment(Qt::AlignTop);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lUnit = new QLabel(buttonGroup5);
        lUnit->setObjectName(QString::fromUtf8("lUnit"));
        lUnit->setWordWrap(false);

        hboxLayout->addWidget(lUnit);

        cbUnit = new QComboBox(buttonGroup5);
        cbUnit->setObjectName(QString::fromUtf8("cbUnit"));

        hboxLayout->addWidget(cbUnit);


        gridLayout6->addWidget(buttonGroup5, 0, 0, 1, 1);

        spacer11 = new QSpacerItem(21, 61, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout6->addItem(spacer11, 1, 0, 1, 1);

        spacer11_2 = new QSpacerItem(211, 21, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout6->addItem(spacer11_2, 0, 1, 1, 1);

        tabWidget->addTab(tab1, QString());

        vboxLayout->addWidget(tabWidget);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        Horizontal_Spacing2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(Horizontal_Spacing2);

        buttonOk = new QPushButton(QG_DlgOptionsGeneral);
        buttonOk->setObjectName(QString::fromUtf8("buttonOk"));
        buttonOk->setAutoDefault(true);
        buttonOk->setDefault(true);

        hboxLayout1->addWidget(buttonOk);

        buttonCancel = new QPushButton(QG_DlgOptionsGeneral);
        buttonCancel->setObjectName(QString::fromUtf8("buttonCancel"));
        buttonCancel->setAutoDefault(true);

        hboxLayout1->addWidget(buttonCancel);


        vboxLayout->addLayout(hboxLayout1);

#ifndef QT_NO_SHORTCUT
        lLanguage->setBuddy(cbLanguage);
        lLanguageCmd->setBuddy(cbLanguageCmd);
        lBackground->setBuddy(cbBackgroundColor);
        lGridColor->setBuddy(cbGridColor);
        lMetaGridColor->setBuddy(cbMetaGridColor);
        lSelectedColor->setBuddy(cbMetaGridColor);
        lHighlightedColor->setBuddy(cbMetaGridColor);
        lMaxPreview->setBuddy(cbMaxPreview);
        lMinGridSpacing->setBuddy(cbMaxPreview);
        lUnit->setBuddy(cbUnit);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(tabWidget, cbLanguage);
        QWidget::setTabOrder(cbLanguage, cbLanguageCmd);
        QWidget::setTabOrder(cbLanguageCmd, cbShowCrosshairs);
        QWidget::setTabOrder(cbShowCrosshairs, cbScaleGrid);
        QWidget::setTabOrder(cbScaleGrid, cbMinGridSpacing);
        QWidget::setTabOrder(cbMinGridSpacing, cbMaxPreview);
        QWidget::setTabOrder(cbMaxPreview, cbBackgroundColor);
        QWidget::setTabOrder(cbBackgroundColor, cbGridColor);
        QWidget::setTabOrder(cbGridColor, cbMetaGridColor);
        QWidget::setTabOrder(cbMetaGridColor, cbSelectedColor);
        QWidget::setTabOrder(cbSelectedColor, cbHighlightedColor);
        QWidget::setTabOrder(cbHighlightedColor, cbSizeStatus);
        QWidget::setTabOrder(cbSizeStatus, buttonOk);
        QWidget::setTabOrder(buttonOk, buttonCancel);
        QWidget::setTabOrder(buttonCancel, lePathTranslations);
        QWidget::setTabOrder(lePathTranslations, lePathHatch);
        QWidget::setTabOrder(lePathHatch, lePathFonts);
        QWidget::setTabOrder(lePathFonts, lePathScripts);
        QWidget::setTabOrder(lePathScripts, lePathLibrary);
        QWidget::setTabOrder(lePathLibrary, cbUnit);

        retranslateUi(QG_DlgOptionsGeneral);
        QObject::connect(buttonOk, SIGNAL(clicked()), QG_DlgOptionsGeneral, SLOT(ok()));
        QObject::connect(buttonCancel, SIGNAL(clicked()), QG_DlgOptionsGeneral, SLOT(reject()));
        QObject::connect(cbSizeStatus, SIGNAL(activated(int)), QG_DlgOptionsGeneral, SLOT(setRestartNeeded()));
        QObject::connect(cbLanguageCmd, SIGNAL(activated(int)), QG_DlgOptionsGeneral, SLOT(setRestartNeeded()));
        QObject::connect(cbLanguage, SIGNAL(activated(int)), QG_DlgOptionsGeneral, SLOT(setRestartNeeded()));

        QMetaObject::connectSlotsByName(QG_DlgOptionsGeneral);
    } // setupUi

    void retranslateUi(QDialog *QG_DlgOptionsGeneral)
    {
        QG_DlgOptionsGeneral->setWindowTitle(QApplication::translate("QG_DlgOptionsGeneral", "Application Preferences", 0, QApplication::UnicodeUTF8));
        bgLanguage->setTitle(QApplication::translate("QG_DlgOptionsGeneral", "Language", 0, QApplication::UnicodeUTF8));
        lLanguage->setText(QApplication::translate("QG_DlgOptionsGeneral", "&GUI Language:", 0, QApplication::UnicodeUTF8));
        lLanguageCmd->setText(QApplication::translate("QG_DlgOptionsGeneral", "&Command Language:", 0, QApplication::UnicodeUTF8));
        bgColors->setTitle(QApplication::translate("QG_DlgOptionsGeneral", "Colors", 0, QApplication::UnicodeUTF8));
        lBackground->setText(QApplication::translate("QG_DlgOptionsGeneral", "Backgr&ound:", 0, QApplication::UnicodeUTF8));
        lGridColor->setText(QApplication::translate("QG_DlgOptionsGeneral", "G&rid Color:", 0, QApplication::UnicodeUTF8));
        cbBackgroundColor->clear();
        cbBackgroundColor->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsGeneral", "#000000", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "#ffffff", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "#c0c0c0", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "#808080", 0, QApplication::UnicodeUTF8)
        );
        cbGridColor->clear();
        cbGridColor->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsGeneral", "#c0c0c0", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "#ffffff", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "#000000", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "#808080", 0, QApplication::UnicodeUTF8)
        );
        lMetaGridColor->setText(QApplication::translate("QG_DlgOptionsGeneral", "&Meta Grid Color:", 0, QApplication::UnicodeUTF8));
        cbMetaGridColor->clear();
        cbMetaGridColor->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsGeneral", "#404040", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "#000000", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "#ffffff", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "#c0c0c0", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "#808080", 0, QApplication::UnicodeUTF8)
        );
        lSelectedColor->setText(QApplication::translate("QG_DlgOptionsGeneral", "S&elected Color:", 0, QApplication::UnicodeUTF8));
        cbSelectedColor->clear();
        cbSelectedColor->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsGeneral", "#a54747", 0, QApplication::UnicodeUTF8)
        );
        cbHighlightedColor->clear();
        cbHighlightedColor->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsGeneral", "#739373", 0, QApplication::UnicodeUTF8)
        );
        lHighlightedColor->setText(QApplication::translate("QG_DlgOptionsGeneral", "&Highlighted Color:", 0, QApplication::UnicodeUTF8));
        buttonGroup5_2->setTitle(QApplication::translate("QG_DlgOptionsGeneral", "Fontsize", 0, QApplication::UnicodeUTF8));
        lSizeStatus->setText(QApplication::translate("QG_DlgOptionsGeneral", "Statusbar:", 0, QApplication::UnicodeUTF8));
        cbSizeStatus->clear();
        cbSizeStatus->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsGeneral", "5", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "6", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "7", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "8", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "9", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "10", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "11", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "12", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "14", 0, QApplication::UnicodeUTF8)
        );
        bgGraphicView->setTitle(QApplication::translate("QG_DlgOptionsGeneral", "Graphic View", 0, QApplication::UnicodeUTF8));
        cbShowCrosshairs->setText(QApplication::translate("QG_DlgOptionsGeneral", "&Show large crosshairs", 0, QApplication::UnicodeUTF8));
        cbShowCrosshairs->setShortcut(QApplication::translate("QG_DlgOptionsGeneral", "Alt+S", 0, QApplication::UnicodeUTF8));
        cbScaleGrid->setText(QApplication::translate("QG_DlgOptionsGeneral", "A&utomatically scale grid", 0, QApplication::UnicodeUTF8));
        cbScaleGrid->setShortcut(QApplication::translate("QG_DlgOptionsGeneral", "Alt+U", 0, QApplication::UnicodeUTF8));
        lMaxPreview->setText(QApplication::translate("QG_DlgOptionsGeneral", "Number of p&review entities:", 0, QApplication::UnicodeUTF8));
        cbMaxPreview->clear();
        cbMaxPreview->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsGeneral", "0", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "50", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "100", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "200", 0, QApplication::UnicodeUTF8)
        );
        lMinGridSpacing->setText(QApplication::translate("QG_DlgOptionsGeneral", "Minimal Grid Spacing (px):", 0, QApplication::UnicodeUTF8));
        cbMinGridSpacing->clear();
        cbMinGridSpacing->insertItems(0, QStringList()
         << QApplication::translate("QG_DlgOptionsGeneral", "0", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "4", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "5", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "6", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "8", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "10", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "15", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("QG_DlgOptionsGeneral", "20", 0, QApplication::UnicodeUTF8)
        );
        tabWidget->setTabText(tabWidget->indexOf(Widget9), QApplication::translate("QG_DlgOptionsGeneral", "&Appearance", 0, QApplication::UnicodeUTF8));
        textLabel2->setText(QApplication::translate("QG_DlgOptionsGeneral", "Translations:", 0, QApplication::UnicodeUTF8));
        textLabel3->setText(QApplication::translate("QG_DlgOptionsGeneral", "Hatch Patterns:", 0, QApplication::UnicodeUTF8));
        textLabel4->setText(QApplication::translate("QG_DlgOptionsGeneral", "Fonts:", 0, QApplication::UnicodeUTF8));
        textLabel5->setText(QApplication::translate("QG_DlgOptionsGeneral", "Scripts:", 0, QApplication::UnicodeUTF8));
        textLabel6->setText(QApplication::translate("QG_DlgOptionsGeneral", "Part Libraries:", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("QG_DlgOptionsGeneral", "&Paths", 0, QApplication::UnicodeUTF8));
        buttonGroup5->setTitle(QApplication::translate("QG_DlgOptionsGeneral", "Defaults for new drawings", 0, QApplication::UnicodeUTF8));
        lUnit->setText(QApplication::translate("QG_DlgOptionsGeneral", "&Unit:", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab1), QApplication::translate("QG_DlgOptionsGeneral", "&Defaults", 0, QApplication::UnicodeUTF8));
        buttonOk->setText(QApplication::translate("QG_DlgOptionsGeneral", "&OK", 0, QApplication::UnicodeUTF8));
        buttonOk->setShortcut(QApplication::translate("QG_DlgOptionsGeneral", "Alt+O", 0, QApplication::UnicodeUTF8));
        buttonCancel->setText(QApplication::translate("QG_DlgOptionsGeneral", "Cancel", 0, QApplication::UnicodeUTF8));
        buttonCancel->setShortcut(QApplication::translate("QG_DlgOptionsGeneral", "Esc", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class QG_DlgOptionsGeneral: public Ui_QG_DlgOptionsGeneral {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DlgOptionsGeneral : public QDialog, public Ui::QG_DlgOptionsGeneral
{
    Q_OBJECT

public:
    QG_DlgOptionsGeneral(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgOptionsGeneral();

public slots:
    virtual void setRestartNeeded();
    virtual void ok();

protected slots:
    virtual void languageChange();

private:
    bool restartNeeded;

    void init();
    void destroy();

};

#endif // QG_DLGOPTIONSGENERAL_H
