#ifndef QG_DLGINITIAL_H
#define QG_DLGINITIAL_H

#include <qvariant.h>


#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include "rs.h"
#include "rs_settings.h"
#include "rs_system.h"
#include "rs_units.h"

QT_BEGIN_NAMESPACE

class Ui_QG_DlgInitial
{
public:
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QLabel *lImage;
    QVBoxLayout *vboxLayout1;
    QLabel *lWelcome;
    QGridLayout *gridLayout;
    QComboBox *cbLanguageCmd;
    QComboBox *cbLanguage;
    QComboBox *cbUnit;
    QLabel *lUnit;
    QLabel *lLanguage;
    QLabel *lCmdLanguage;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *Horizontal_Spacing2;
    QPushButton *buttonOk;

    void setupUi(QDialog *QG_DlgInitial)
    {
        if (QG_DlgInitial->objectName().isEmpty())
            QG_DlgInitial->setObjectName(QString::fromUtf8("QG_DlgInitial"));
        QG_DlgInitial->resize(413, 287);
        QG_DlgInitial->setSizeGripEnabled(false);
        vboxLayout = new QVBoxLayout(QG_DlgInitial);
        vboxLayout->setSpacing(6);
        vboxLayout->setContentsMargins(11, 11, 11, 11);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(19);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        lImage = new QLabel(QG_DlgInitial);
        lImage->setObjectName(QString::fromUtf8("lImage"));
        QSizePolicy sizePolicy(static_cast<QSizePolicy::Policy>(0), static_cast<QSizePolicy::Policy>(5));
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(lImage->sizePolicy().hasHeightForWidth());
        lImage->setSizePolicy(sizePolicy);
        lImage->setFrameShape(QFrame::WinPanel);
        lImage->setFrameShadow(QFrame::Sunken);
        lImage->setLineWidth(1);
        lImage->setPixmap(qt_get_icon(image0_ID));
        lImage->setScaledContents(true);
        lImage->setWordWrap(false);

        hboxLayout->addWidget(lImage);

        vboxLayout1 = new QVBoxLayout();
        vboxLayout1->setSpacing(6);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        lWelcome = new QLabel(QG_DlgInitial);
        lWelcome->setObjectName(QString::fromUtf8("lWelcome"));
        QSizePolicy sizePolicy1(static_cast<QSizePolicy::Policy>(5), static_cast<QSizePolicy::Policy>(5));
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lWelcome->sizePolicy().hasHeightForWidth());
        lWelcome->setSizePolicy(sizePolicy1);
        lWelcome->setAlignment(Qt::AlignVCenter);
        lWelcome->setWordWrap(true);

        vboxLayout1->addWidget(lWelcome);

        gridLayout = new QGridLayout();
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(14, 14, 14, 14);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        cbLanguageCmd = new QComboBox(QG_DlgInitial);
        cbLanguageCmd->setObjectName(QString::fromUtf8("cbLanguageCmd"));

        gridLayout->addWidget(cbLanguageCmd, 2, 1, 1, 1);

        cbLanguage = new QComboBox(QG_DlgInitial);
        cbLanguage->setObjectName(QString::fromUtf8("cbLanguage"));

        gridLayout->addWidget(cbLanguage, 1, 1, 1, 1);

        cbUnit = new QComboBox(QG_DlgInitial);
        cbUnit->setObjectName(QString::fromUtf8("cbUnit"));

        gridLayout->addWidget(cbUnit, 0, 1, 1, 1);

        lUnit = new QLabel(QG_DlgInitial);
        lUnit->setObjectName(QString::fromUtf8("lUnit"));
        lUnit->setWordWrap(false);

        gridLayout->addWidget(lUnit, 0, 0, 1, 1);

        lLanguage = new QLabel(QG_DlgInitial);
        lLanguage->setObjectName(QString::fromUtf8("lLanguage"));
        lLanguage->setWordWrap(false);

        gridLayout->addWidget(lLanguage, 1, 0, 1, 1);

        lCmdLanguage = new QLabel(QG_DlgInitial);
        lCmdLanguage->setObjectName(QString::fromUtf8("lCmdLanguage"));
        lCmdLanguage->setWordWrap(false);

        gridLayout->addWidget(lCmdLanguage, 2, 0, 1, 1);


        vboxLayout1->addLayout(gridLayout);


        hboxLayout->addLayout(vboxLayout1);


        vboxLayout->addLayout(hboxLayout);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setSpacing(6);
        hboxLayout1->setContentsMargins(0, 0, 0, 0);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        Horizontal_Spacing2 = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(Horizontal_Spacing2);

        buttonOk = new QPushButton(QG_DlgInitial);
        buttonOk->setObjectName(QString::fromUtf8("buttonOk"));
        buttonOk->setAutoDefault(true);
        buttonOk->setDefault(true);

        hboxLayout1->addWidget(buttonOk);


        vboxLayout->addLayout(hboxLayout1);

        QWidget::setTabOrder(cbUnit, cbLanguage);
        QWidget::setTabOrder(cbLanguage, cbLanguageCmd);
        QWidget::setTabOrder(cbLanguageCmd, buttonOk);

        retranslateUi(QG_DlgInitial);
        QObject::connect(buttonOk, SIGNAL(clicked()), QG_DlgInitial, SLOT(ok()));

        QMetaObject::connectSlotsByName(QG_DlgInitial);
    } // setupUi

    void retranslateUi(QDialog *QG_DlgInitial)
    {
        QG_DlgInitial->setWindowTitle(QApplication::translate("QG_DlgInitial", "Welcome", 0, QApplication::UnicodeUTF8));
        lWelcome->setText(QApplication::translate("QG_DlgInitial", "<font size=\"+1\"><b>Welcome to QCad</b>\n"
"</font>\n"
"<br>\n"
"Please choose the unit you want to use for new drawings and your preferred language.<br>\n"
"You can changes these settings later in the Options Dialog of QCad.", 0, QApplication::UnicodeUTF8));
        lUnit->setText(QApplication::translate("QG_DlgInitial", "Default Unit:", 0, QApplication::UnicodeUTF8));
        lLanguage->setText(QApplication::translate("QG_DlgInitial", "GUI Language:", 0, QApplication::UnicodeUTF8));
        lCmdLanguage->setText(QApplication::translate("QG_DlgInitial", "Command Language:", 0, QApplication::UnicodeUTF8));
        buttonOk->setText(QApplication::translate("QG_DlgInitial", "OK", 0, QApplication::UnicodeUTF8));
        buttonOk->setShortcut(QApplication::translate("QG_DlgInitial", "Enter", 0, QApplication::UnicodeUTF8));
    } // retranslateUi


protected:
    enum IconID
    {
        image0_ID,
        unknown_ID
    };
    static QPixmap qt_get_icon(IconID id)
    {
    static const char* const image0_data[] = { 
"100 226 465 2",
"Qt c #000000",
".F c #000400",
"aA c #0079ff",
"aB c #007dff",
".J c #080408",
".H c #080808",
".N c #080c08",
".f c #100c10",
".o c #101010",
".D c #101410",
"az c #105dee",
"ba c #1071e6",
"#H c #181418",
".K c #181818",
"#. c #181c18",
"b# c #1861ee",
"bb c #1889ff",
"aC c #18a1ff",
".u c #201c20",
".G c #202020",
".4 c #202420",
"a1 c #292429",
".I c #292829",
".L c #292c29",
"d6 c #297941",
"bc c #2999ff",
"cd c #312c31",
".O c #313031",
".M c #313431",
"aD c #31a5ff",
"aW c #393439",
".g c #393839",
"#u c #393c39",
"dO c #413841",
"#r c #413c41",
".3 c #414041",
"#M c #414441",
"bl c #417dbd",
".W c #4a244a",
"dH c #4a284a",
"a0 c #4a444a",
".B c #4a484a",
"#v c #4a4c4a",
"bd c #4abeff",
"#w c #522c52",
".# c #523052",
".m c #524c52",
"an c #525052",
".w c #525552",
"bm c #527de6",
".9 c #5a345a",
"#D c #5a385a",
"aO c #5a3862",
".l c #5a5552",
"cH c #5a555a",
"b9 c #5a5952",
".e c #5a595a",
"br c #5a5d5a",
"eo c #5a9562",
"#1 c #5a99e6",
"aL c #5a99f6",
"be c #5abeff",
"ff c #623862",
".V c #623c62",
"aS c #624062",
"cF c #625d5a",
"bS c #625d62",
".C c #626162",
"#V c #626562",
"cw c #626962",
"d5 c #62955a",
"dM c #629941",
"bk c #629df6",
"b3 c #62a57b",
"#2 c #62baff",
"#N c #6a446a",
"aV c #6a486a",
"#B c #6a4c6a",
"a6 c #6a5d5a",
"dX c #6a656a",
"ar c #6a696a",
"#d c #6a6d6a",
"d4 c #6a9d41",
"#3 c #6ab2f6",
"aF c #6aceff",
"#L c #734c73",
"#I c #735073",
"fn c #736573",
".a c #73696a",
".Z c #736d73",
".c c #737173",
".n c #737573",
"bn c #737583",
"aM c #7395e6",
"#0 c #739dff",
"cg c #73b294",
"aE c #73caff",
".T c #7b6573",
"dg c #7b657b",
"cq c #7b697b",
"#T c #7b6d7b",
"bs c #7b757b",
"#x c #7b7973",
"ae c #7b797b",
"au c #7b7d7b",
"dK c #7baa52",
"cs c #7bba8b",
"bj c #7bbade",
"ch c #7bbe94",
"ct c #7bd2b4",
"bf c #7be2ff",
"dB c #830c94",
"dS c #831c94",
"cl c #83696a",
".v c #836d83",
"#a c #837183",
"c2 c #837d7b",
"dY c #837d83",
".1 c #838183",
"a5 c #838583",
"bN c #83a1f6",
"#4 c #83d6ff",
"#E c #8b758b",
"#F c #8b798b",
"dh c #8b7d8b",
"cb c #8b857b",
"cG c #8b858b",
"#i c #8b898b",
"#O c #8b8d8b",
"bE c #8b8d94",
"bG c #8b9194",
"ab c #8b919c",
"bL c #8b9dac",
"b2 c #8b9db4",
"dy c #8bb662",
"dN c #8bbe8b",
"dw c #8bc26a",
"#5 c #8bcaf6",
"cJ c #8bce9c",
"eA c #946d94",
"#U c #947194",
"fo c #947594",
"#P c #947d94",
"ef c #948194",
"eO c #948594",
"b1 c #948d94",
"#C c #949194",
"bU c #949594",
"dL c #94a141",
"b. c #94a1cd",
"aN c #94aecd",
"cu c #94d2a4",
"aJ c #94d6ff",
"bg c #94e2ff",
"dQ c #9c08a4",
"cc c #9c759c",
"c7 c #9c7d9c",
".b c #9c8194",
".d c #9c859c",
"a2 c #9c899c",
"bx c #9c8d9c",
"eT c #9c9583",
"ec c #9c9594",
"dZ c #9c999c",
"fc c #9c9d94",
"eK c #9c9d9c",
"aK c #9ccaff",
"dk c #9cd28b",
"cO c #a40808",
"c# c #a47518",
"ac c #a47da4",
"#p c #a485a4",
"fg c #a489a4",
"fm c #a48da4",
"by c #a491a4",
"bV c #a495a4",
"e7 c #a4998b",
"#A c #a49d94",
"eg c #a49da4",
"fe c #a4a18b",
"## c #a4a1a4",
"#K c #a4a5a4",
"cf c #a4beac",
"a# c #a4c2d5",
"dx c #a4c683",
"aH c #a4f2ff",
"dC c #ac30bd",
"dp c #ac7db4",
"#l c #ac85ac",
".E c #ac89ac",
"fl c #ac8dac",
"d2 c #ac95ac",
".U c #ac99ac",
".6 c #acaaac",
"#k c #acaeac",
"dI c #acbaa4",
"dm c #acce94",
"dR c #b404bd",
"cy c #b40c08",
"e. c #b418b4",
"ea c #b43cb4",
"fi c #b44cb4",
"ev c #b47db4",
"ag c #b48db4",
"#n c #b491b4",
"dn c #b4958b",
"#s c #b495b4",
"eX c #b499b4",
"bZ c #b49db4",
"cr c #b49dbd",
"#R c #b4a1b4",
"bF c #b4a1bd",
"en c #b4a58b",
".7 c #b4a5b4",
"bK c #b4a5bd",
".t c #b4aaac",
"ei c #b4aeb4",
"#t c #b4b2b4",
"bM c #b4b2de",
"ej c #b4b6b4",
"aa c #b4bade",
"d# c #b4e6a4",
"#7 c #b4e6ff",
"cW c #b4eabd",
"bh c #b4f2ff",
"aG c #b4f6ff",
"e9 c #bd55bd",
"as c #bd95bd",
".A c #bd9983",
"bv c #bd9dbd",
"cm c #bda573",
"bY c #bda5bd",
"#c c #bdaabd",
"eJ c #bdae94",
"#b c #bdaebd",
"bp c #bdbabd",
"ap c #bdbebd",
"dl c #bdda9c",
"bi c #bdeaff",
"cL c #bdeebd",
"cN c #c51810",
"eu c #c56dc5",
"cx c #c5898b",
"cC c #c59108",
"cB c #c5998b",
"d0 c #c59dc5",
"ez c #c5a1bd",
"ak c #c5a1c5",
"aT c #c5b2c5",
"bD c #c5b2cd",
"eh c #c5b6c5",
"aX c #c5bec5",
"dt c #c5c2bd",
"bq c #c5c2c5",
"al c #c5c6c5",
"d3 c #c5ceac",
"d7 c #c5ced5",
"dJ c #c5da8b",
"#6 c #c5e6ff",
"cK c #c5eecd",
"#9 c #c5eef6",
"cY c #c5fac5",
"e# c #cd10cd",
"es c #cd2ccd",
"f# c #cd5dcd",
"do c #cd85d5",
"eb c #cda1d5",
"bt c #cda5cd",
"bQ c #cdb6cd",
"c0 c #cdbaa4",
"b0 c #cdbacd",
"eL c #cdbecd",
"e3 c #cdc6cd",
"ep c #cdc6d5",
"dz c #cdcabd",
".8 c #cdcacd",
"eN c #cdcecd",
"dv c #cde2b4",
"di c #cdeac5",
"dj c #cdf2bd",
"aI c #cdffff",
"c1 c #d55d52",
"cA c #d56162",
"fh c #d569d5",
"ca c #d58931",
"cP c #d58da4",
"eF c #d599d5",
"cR c #d5a108",
".P c #d5aea4",
"aj c #d5bed5",
".5 c #d5c2d5",
"aR c #d5c6d5",
"bO c #d5c6ee",
"c8 c #d5ced5",
"#m c #d5d2d5",
"ad c #d5d6d5",
"cI c #d5d6de",
"b4 c #d5dade",
"da c #d5f2c5",
"cz c #de0000",
"f. c #de4cde",
"dA c #de6dde",
"co c #de8d18",
"eq c #de91e6",
"cn c #de9510",
".h c #de9583",
".p c #de958b",
"eV c #deae20",
"c5 c #deb220",
"cU c #deb6de",
"cp c #debaa4",
"cT c #debaac",
"bX c #debade",
"cE c #debe6a",
"eW c #debeb4",
"e4 c #dec2cd",
"bW c #dec6de",
"du c #decab4",
".0 c #decade",
"bP c #decede",
"bo c #dedade",
"ce c #dedede",
"cV c #dee2e6",
"a. c #deeeff",
"et c #e62ce6",
"dP c #e648e6",
"b5 c #e6797b",
"c. c #e69962",
"eY c #e69de6",
"ek c #e6bee6",
"ay c #e6c2cd",
"aY c #e6c6e6",
"c3 c #e6ce8b",
"a3 c #e6cee6",
".Y c #e6d2e6",
"el c #e6d6e6",
"ai c #e6e2e6",
"#q c #e6e6e6",
"cv c #e6eaee",
"d. c #e6ffde",
"cX c #e6ffe6",
"#8 c #e6ffff",
"d9 c #ee34ee",
"eR c #ee81ee",
"fj c #ee85ee",
"cD c #ee9508",
"fk c #eea1ee",
"fb c #eea5ee",
"#h c #eec2ac",
"dr c #eec641",
"aQ c #eec6ee",
"c6 c #eecabd",
".X c #eecaee",
".2 c #eeceee",
"bT c #eed6ee",
"db c #eedab4",
"aZ c #eedaee",
"eM c #eedeee",
"bu c #eeeaee",
"aq c #eeeeee",
"bw c #eeeef6",
"cZ c #eeffe6",
"cj c #f60000",
"b7 c #f60808",
"er c #f648f6",
"b8 c #f689ac",
"cM c #f6b2ac",
"ey c #f6ce5a",
"em c #f6cef6",
"eG c #f6d2bd",
"bI c #f6d2c5",
"#S c #f6d2f6",
"#e c #f6d67b",
"#W c #f6d6d5",
"dG c #f6dacd",
"d1 c #f6def6",
"ew c #f6e2a4",
"#o c #f6e2f6",
"am c #f6e6f6",
"af c #f6f2f6",
"ah c #f6f6f6",
"c9 c #f6faf6",
"b6 c #ff0400",
"bB c #ff1c00",
"bA c #ff2418",
"ck c #ff3031",
"ci c #ff385a",
".r c #ff4000",
".j c #ff4029",
"a8 c #ff4400",
".i c #ff4829",
"eD c #ff50ff",
"eE c #ff65ff",
".y c #ff6900",
"a7 c #ff6d4a",
".q c #ff7120",
"a9 c #ff7131",
".z c #ff7520",
"bC c #ff796a",
"fa c #ff79ff",
"eC c #ff7dff",
"aw c #ff8100",
"ax c #ff8510",
"e0 c #ff89ff",
".s c #ff8d7b",
"eS c #ff91ff",
"av c #ff956a",
"eB c #ff95ff",
".R c #ff9900",
".x c #ff9952",
"#X c #ff9d08",
".Q c #ff9d18",
"d8 c #ff9dff",
".S c #ffa152",
"eZ c #ffa1ff",
"#g c #ffa500",
".k c #ffa5c5",
"e8 c #ffa5ff",
"eQ c #ffaaff",
"e1 c #ffaeff",
"e5 c #ffb239",
"#Y c #ffba00",
"c4 c #ffbe20",
"dd c #ffbe41",
"e6 c #ffbe52",
"#f c #ffc200",
"cQ c #ffc239",
"eH c #ffc652",
"df c #ffc67b",
"bz c #ffc6d5",
"eP c #ffc6ff",
"cS c #ffca62",
"dD c #ffcaa4",
"bH c #ffcab4",
"de c #ffce31",
"eU c #ffce41",
"bJ c #ffceb4",
"eI c #ffd241",
"#Z c #ffd273",
"fd c #ffd27b",
"e2 c #ffd2ff",
"dF c #ffd662",
"dT c #ffd6f6",
"a4 c #ffd6ff",
"#z c #ffda31",
"dU c #ffda6a",
"#y c #ffde62",
"aU c #ffdeff",
"ds c #ffe26a",
"dE c #ffe27b",
"dq c #ffe294",
"dc c #ffe2de",
"bR c #ffe2ff",
"ex c #ffe65a",
"aP c #ffe6ff",
"dW c #ffeaac",
"#Q c #ffeaff",
"dV c #ffee6a",
"ao c #ffeeff",
"at c #fff2ff",
"ee c #fff6ac",
"#G c #fff6ff",
"#J c #fffaff",
"ed c #ffffbd",
"#j c #ffffff",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQt.#.a.b.c.d.e.fQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQtQt.g.h.i.j.k.l.m.n.fQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQt.o.p.q.r.s.t.uQt.v.wQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQtQt.e.x.y.z.A.B.CQt.D.E.FQtQtQtQtQtQtQtQtQtQtQtQtQtQt.F.D.G.u.G.D.FQtQt.H.I.F.J.K.L.HQtQtQt.D.I.oQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.o.g.M.K.K.JQtQtQtQtQtQtQtQt.N.O.L.JQtQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQt.J.P.Q.R.S.T.N.U.f.F.C.VQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.W.X.Y.Z.0.1.HQt.v.2.3.4.5.6.KQtQt.H.7.8.9QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt#..5###a###b.oQtQtQtQtQtQtQt.D#c#d.5.G.JQtQtQtQtQt",
"QtQtQtQtQtQtQtQtQtQtQt.G#e#f#g#h.DQt.B.wQt.o#i.JQtQtQtQtQtQtQtQtQtQtQtQtQtQt.u#j#kQt.6#j.MQt.O#l.G.4#m#n.FQtQt.J.6#o.HQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt#p#q.3Qt#r#i.HQtQtQtQtQtQt.H#s#t.K#u#v#uQtQtQtQtQt",
"QtQtQtQtQtQt.D.G#w.O.M#x#y#f#z#A.9.O#B#C#D.O#c.B.9.O.I.K.HQtQtQtQtQtQtQtQtQt#E#j#F.H.Y#G#w#H.3.3.J.B#j.n#I.G.F.N#J#K#L.O.JQt.H.I.V.g.J.H.##M#H.M#NQt.J#O#j.c.F.N.#Qt.J.I#D.g.H.N#P#Q#R#.#E#S#TQtQtQtQtQt",
"QtQtQtQt.H#U.1.c#V.C#V#W#X#Y#Z#0#1#2#3#4#5#6#7#8#9a.a#aaab.gQtQtQtQtQtQtQt.H###j.eacadaeQt.gafagQt#lah#iaiaj.H.3#jakalam.I.H.c#nanao.g.Dap#Q.e#oaq.HQt#raq#j#M.FQt.Haras#Mat#M.uap#jau.Bad#Q.wQtQtQtQtQt",
"QtQtQt.N.U.M.H.F.FQt.#avawaxayazaAaBaCaDaEaFaGaHaIaJaKaLaMaNaOQtQtQtQtQtQt.4aPaQaR#JaSQtQt.B#j#M.J#m.0.JaTaU#H#Oao.I#EaPaV#d#G.MaWat#P.oataX#DaYaZQtQtQt.v#Q#Ja0Qt.C#j.Oa1#Ja2Qta3a4.H.Gata5.JQtQtQtQtQt",
"QtQtQt.L##.H.H.o.o.Da6a7a8a9b.b#babbbcbdbebfbgaIbhbibjbkblbmbnQtQtQtQtQtQta0ah.E.e#j.cQtQt#nbo.G.4#j#d.Jbpa3.ua3#K.faeao.OaPbq.Jbr#j.3.g#J#i.u#j.1.N.OQt.Hbs#jbt.HaZ.8.Jan#j#v.obubvQt.B#J.3QtQtQtQtQtQt",
"QtQtQtbrbwa5bxaebya5bzbAbBbCbDbEbFbEbFbGaZbHbIbJ.YbGbKbLbMbNbO.DQtQtQtQtQt#i#j#r.G#QbP.H.K.XbQ.G#PbR.g.gaobS.gaP#F.DbTbU.u#JbV.FbWbX.H.1#G.I.vaP.vaWbY.FQt#H#j#k.GaPbZ.Hb0bX#Ha0#jbrQtb1at#..JQtQtQtQtQt",
"QtQtQtb2b3b4.B.N.F.Gb5b6b7b8.N.FQt.FQt.Fb9c.c#cacb.H.F#..1cc###vQtQtQtQt.KaZaq.I.fajad.3.Oao#K.O#O.Y.gbv.1.N.w#J.3#U.6.u.F#o.1an#Kcd.N.5.6.oauao#d.e#marcdccad.B.N.Y#i#v.6cdQt.1ce.OQtbWal.B.FQtQtQtQtQt",
"QtQt.fcfcgch#m.O.F#VcicjckclQtQtQtQtQtQtcdcmcncocp#H.H.c#aQt.9#dQtQtQtQt.I.m#Icd.Hcd#I.g#H.eaS.J.G.maV.3.HQt#H.BaV.B#HQtQt.Ocq.3#HQt.f.3.OQt.W.B.o.I#Nan.v.C#w.FQt.Icq.3.KQt.o#taj.HQt#M.v.oQtQtQtQtQtQt",
"QtQtQtcrcsctcucvcwcxcyczcA.gQtQtQtQtQtQt.HcBcCcDcEcFcG.BQt.F#V.3QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.HcH#uaP#uQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQt.McIcJcKcL#jcMcNcOcP.KQtQtQtQtQtQtQt.CcQcRcScT#uQt.Ja0.d.HQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt#HcU.Ubr.JQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQt.gcVcWcXcYcZc0c1c2.JQtQtQtQtQtQtQt#Hc3c4c5c6.o.Janc7.NQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQt.gc8c9d.d#dadbbV.oQtQtQtQtQtQt.J.Idcdddedfdg#ddh.NQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQt.Dbvdidjdkdldmdn#.QtQtQtQt.FcHdodpdqdrdsdt#v.FQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt",
"QtQtQtQtQtQt.FbS#Rdudvdwdxdydz#v.HQt.G#FdAdBdCdDdEdFdG.HQtQtQtQtQtQtQtQtQtQt.J.F.J.J.H.H.f.H.f.N.o.o.o.o#H.D.K#H.K.K.u#..u.u.G.G.W.Ga1.4dH.I.I.I#w.L.Ocd.#.OaW.M.9aW.g.g.V.g#r#uaS.3.3.3#N#MaVa0aV.B#B#v",
"QtQtQtQtQtQt.Fae.G.OdIdJdKdLdMdNaudO.EdPdQdRdSdTdUdVdW.LQtQtQtQtQtQtQtQtQt.J.F.N.N.D.D#..ua1.4.LcdaW.M#u#ra0#M#vancH.ebSbSdXar.Z.cccaedY.1#l#iag#CasdZd0##bt.6aj#tbWbpa3bqbT.8d1#maPboaoai#Jbu#jaf#j#J#j",
"QtQtQtQtQtQt.o.1.NQtd2#id3d4d5d6d7d8d9e.e#eaebeceddsee.cQtQtQtQtQtQtQtQtQt.F.H.N.o.o.K#..G.4dH.L.O.M#D#u.3#MaV#v#I.wcqbr#a#V#F#def.na2aubya5.U#O#RbU#cegeh#Kajei.5eja3ekelald1emamadaoaU#GaP#jao#j#G#j#j",
"QtQtQtQtQtQt.GccQt.N#i.H#Meneoepeqereseteuev.K.IewexeyezQtQtQtQtQtQtQtQtQt.F.F.f.N.D.D.u#..4a1cd.L.MaW#r#u#Ma0.mancH.ebSbrdXareA.cbsaeac.1cG#iag#CasdZd0##bt.6aj#tbWbpa3bqbT.8d1#maPboaoai#Jbu#jaf#j#J#j",
"QtQtQtQtQtQt#N.wQt#u.vQt.J.C#QeBeCeDeEeFbW.GQt.oeGeHeIeJ.oQtQtQtQtQtQtQtQt.F.H.N.o.o.K#..G.4.I.L.#.M.g#uaS#MaV#v#I.wcqbr#a#V#F#def.na2aubya5.U#O#RbU#ceKbQ#KeL#k.5cUbPapbTaQeMeNaPa4aoaU#GaP#jao#j#G#j#j",
"QtQtQtQtQtQt.B.wQt#d.L.IeOePeQeBeReSd2.L.M.E#M.KeTeUeVeW.DQtQtQtQtQtQtQtQt.J.F.N.N.D.D#..ua1.4.LcdaW.M#u#ra0#M#vancH.ebSbSdXar.Z.cccaedY.1#l#iag#CasdZd0##bt.6aj#tbWbpa3bqbT.8d1#maPboaoai#Jbu#jaf#j#J#j",
"QtQtQtQtQtQtdHdY.fcGeXeYeZe0e1eQe2e3#IQtQt.o#Eb1e4e5e6e7.HQtQtQtQtQtQtQtQt.F.H.N.o.o.K#..G.4dH.L.O.M#D#u.3#MaV#v#I.wcqbr#a#V#F#def.na2aubya5.U#O#RbU#cegeh#Kajei.5eja3ekelald1emamadaoaU#GaP#jao#j#G#j#j",
"QtQtQtQtQtQt.F.Cc8e8e9f.f#fafbek#M.o.neA.KQtQt.ffcfdfe.uQtQtQtQtQtQtQtQtQt.F.F.f.N.D.D.u#..4a1cd.L.MaW#r#u#Ma0.mancH.ebSbrdXareA.cbsaeac.1cG#iag#CasdZd0##bt.6aj#tbWbpa3bqbT.8d1#maPboaoai#Jbu#jaf#j#J#j",
"QtQtQtQtQtQtQt.Ffffgfhfifjfkfl.u.JQt.H.3.U.e.G.4b0.c#HQtQtQtQtQtQtQtQtQtQt.F.H.N.o.o.K#..G.4.I.L.#.M.g#uaS#MaV#v#I.wcqbr#a#V#F#def.na2aubya5.U#O#RbU#ceKbQ#KeL#k.5cUbPapbTaQeMeNaPa4aoaU#GaP#jao#j#G#j#j",
"QtQtQtQtQtQtQtQtQt.o.Zevar.IQtQtQtQtQtQt.H.m.nbs#r.JQtQtQtQtQtQtQtQtQtQtQt.J.F.N.N.D.D#..ua1.4.LcdaW.M#u#ra0#M#vancH.ebSbSdXar.Z.cccaedY.1#l#iag#CasdZd0##bt.6aj#tbWbpa3bqbT.8d1#maPboaoai#Jbu#jaf#j#J#j",
"QtQtQtQtQtQtQtQtQtQt.J.H.FQtQtQtQtQtQtQtQtQt.J.JQtQtQtQtQtQtQtQtQtQtQtQtQt.F.H.N.o.o.K#..G.4dH.L.O.M#D#u.3#MaV#v#I.wcqbr#a#V#F#def.na2aubya5.U#O#RbU#cegeh#Kajei.5eja3ekelald1emamadaoaU#GaP#jao#j#G#j#j",
"QtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQtQt.F.F.f.N.D.D.u#..4a1cd.L.MaW#r#u#Ma0.mancH.ebSbrdXareA.cbsaeac.1cG#iag#CasdZd0##bt.6aj#tbWbpa3bqbT.8d1#maPboaoai#Jbu#jaf#j#J#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#J#jbu#J#qaoceaoceaoceaobu#Jaq#j#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#Jao#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#G#JaUataUataUataUataUataUataUataUataUatbR#j#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"ceaoce#Gaf#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#J#jaiaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoai#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"ataUataUatbR#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaoataUataUataUataUataUataUataUataUataUataUataUataUataUataU#J#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"ceaoceaoceaoce#J#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaiaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoaf#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"ataUataUataUataU#G#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUat#Q#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"ceaoceaoceaoceaoceaoah#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaobu#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"ataUataUataUataUataUat#G#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUatat#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"ceaoceaoceaoceaoceaoceao#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceao#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"ataUataUataUataUataUataU#G#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jbRataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataU#G#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"ceaoceaoceaoceaoceaoceaoce#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#qaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoce#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"ataUataUataUataUataUataUataU#j#j#j#j#j#j#j#j#j#j#j#j#j#GataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataUataU#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"ceaoceaoceaoceaoceaoceaoceao#q#j#j#j#j#j#j#j#j#j#j#j#jatceaoceaoceaoceaoceaoceaoceaoceaoceaoai#jah#j#j#j#J#j#qaoceaoceaoceaoceaoceaoceaoceaoceaoaq#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"ataUataUataUataUataUataUataUat#J#j#j#j#j#j#j#j#j#j#j#jaUataUataUataUataUataUataUataUataUatat#j#j#j#j#j#j#j#j#j#J#GaUataUataUataUataUataUataUataUat#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"ceaoceaoceaoceaoceaoceaoceaoce#j#j#j#j#j#j#j#j#j#j#j#qaoceaoceaoceaoceaoceaoceaoceaoceat#J#j#j#j#j#j#j#j#j#j#j#j#j#Jceaoceaoceaoceaoceaoceaoceaoce#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jah#jbu#Jbu#J",
"ataUataUataUataUataUataUataUataU#j#j#j#j#j#j#j#j#j#jataUataUataUataUataUataUataUataUat#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#JaUataUataUataUataUataUataUataP#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#G#JaPataUataUataUataU",
"ceaoceaoceaoceaoceaoceaoceaoceaoah#j#j#j#j#j#j#j#j#jceaoceaoceaoceaoceaoceaoceaoceaoaf#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#Jaoceaoceaoceaoceaoceaoai#Jaf#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaiaoceaoceaoceaoceaoceao",
"#GaUataUataUataUataUataUataUataU#J#j#j#j#j#j#j#j#j#QataUataUataUataUataUataUataUatbR#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#QataUataUataU#Jao#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#J#GaUataUataUataUataUataUataU",
"afaoceaoceaoceaoceaoceaoceaoceaoai#j#j#j#j#j#j#j#jatceaoceaoceaoceaoceaoceaoceaoce#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jceatbu#j#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jahaoceaoceaoceaoceaoceaoceaoceao",
"#jaUataUataUataUataUataUataUataUat#j#j#j#j#j#j#j#jaUataUataUataUataUataUataUataUat#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#GataUataUataUataUataUataUataUataU",
"#jatceaoceaoceaoceaoceaoceaoceaoce#j#j#j#j#j#j#jahaoceaoceaoceaoceaoceaoceaoceaoai#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#Jaoceaoceaoceaoceaoceaoceaoceaoceao",
"#j#QataUataUataUataUataUataUataUat#G#j#j#j#j#j#j#jaUataUataUataUataUataUataUataU#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#JaUataUataUataUataUataUataUataUataU",
"#j#Jceaoceaoceaoceaoceaoceaoceaoce#j#j#j#j#j#j#jafaoceaoceaoceaoceaoceaoceaoceaobu#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jceaoceaoceaoceaoceaoceaoceaoceaoceao",
"#j#GataUataUataUataUataUataUataUat#Q#j#j#j#j#j#j#JaUataUataUataUataUataUataUataU#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaoataUataUataUataUataUataUataUataP#j#J",
"#j#jceaoceaoceaoceaoceaoceaoceaoce#J#j#j#j#j#j#jbuaoceaoceaoceaoceaoceaoceaoceaoah#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jatceaoceaoceaoceaoceaoceaoceaoah#j#j#j",
"#j#GataUataUataUataUataUataUataUat#Q#j#j#j#j#j#j#JaUataUataUataUataUataUataUataU#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaUataUataUataUataUataUataUat#G#j#j#j#j",
"#j#jceaoceaoceaoceaoceaoceaoceaoce#J#j#j#j#j#j#jbuaoceaoceaoceaoceaoceaoceaoceaoah#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#J#jah#jbu#Jbuatceaoceao#q#j#j#j#j#j",
"#j#GataUataUataUataUataUataUataUat#Q#j#j#j#j#j#j#JaUataUataUataUataUataUataUataU#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#G#j#j#j#j#j#j",
"#j#jceaoceaoceaoceaoceaoceaoceaoce#J#j#j#j#j#j#jbuaoceaoceaoceaoceaoceaoceaoceaoah#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#GataUataUataUataUataUataUataUat#Q#j#j#j#j#j#j#JaUataUataUataUataUataUataUataU#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#Jceaoceaoceaoceaoceaoceaoceaoce#j#j#j#j#j#j#jafaoceaoceaoceaoceaoceaoceaoceaoaf#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jah#J#qao",
"#j#QataUataUataUataUataUataUataUat#G#j#j#j#j#j#j#jaUataUataUataUataUataUataUataU#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#G#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#G#JaPataUataUataU",
"#j#Gceaoceaoceaoceaoceaoceaoceaoce#j#j#j#j#j#j#jahaoceaoceaoceaoceaoceaoceaoceaobu#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jbuaoce#Gaf#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jbuaoceaoceaoceaoceaoceao",
"#jaUataUataUataUataUataUataUataUat#j#j#j#j#j#j#j#jaUataUataUataUataUataUataUataUat#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jataUataUataU#Gao#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaoataUataUataUataUataUataUataU",
"ahaoceaoceaoceaoceaoceaoceaoceaoai#j#j#j#j#j#j#j#jatceaoceaoceaoceaoceaoceaoceaoce#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jceaoceaoceaoceaoceaobu#j#j#j#j#j#j#j#j#j#j#j#j#jceaoceaoceaoceaoceaoceaoceaoceao",
"#JaUataUataUataUataUataUataUataU#J#j#j#j#j#j#j#j#j#QataUataUataUataUataUataUataUatat#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaPataUataUataUataUataUataUataP#j#j#j#j#j#j#j#j#JaUataUataUataUataUataUataUataUataU",
"ceaoceaoceaoceaoceaoceaoceaoceaoah#j#j#j#j#j#j#j#j#jceaoceaoceaoceaoceaoceaoceaoceat#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#Jaoceaoceaoceaoceaoceaoceaoceaoah#j#j#j#j#j#j#jceaoceaoceaoceaoceaoceaoceaoceaoceao",
"ataUataUataUataUataUataUataUataU#j#j#j#j#j#j#j#j#j#jataUataUataUataUataUataUataUataU#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#GaUataUataUataUataUataUataUataU#j#j#j#j#j#j#jaUataUataUataUataUataUataUataUat#Q#j#j",
"ceaoceaoceaoceaoceaoceaoceaoce#J#j#j#j#j#j#j#j#j#j#jbuaoceaoceaoceaoceaoceaoceaoceaoce#j#j#j#j#j#j#j#j#j#j#j#j#j#j#Jceaoceaoceaoceaoceaoceaoceaoce#J#j#j#j#j#j#jaqaoceaoceaoceaoceaoceaoceaoce#J#J#j#j#j",
"ataUataUataUataUataUataUataUat#J#j#j#j#j#j#j#j#j#j#j#jaUataUataUataUataUataUataUataUataU#j#j#j#j#j#j#j#j#j#j#j#j#JaUataUataUataUataUataUataUataUat#G#j#j#j#j#j#jataUataUataUataUataUataUataU#J#j#j#j#j#j",
"ceaoceaoceaoceaoceaoceaoceao#q#j#j#j#j#j#j#j#j#j#j#j#j#Gceaoceaoceaoceaoceaoceaoceaoceaoce#Gah#j#j#j#j#j#j#jafatceaoceaoceaoceaoceaoceaoceaoceaoai#j#j#j#j#j#j#jceaoceaoceaoceaoceaoceaoceao#J#j#j#j#j#j",
"ataUataUataUataUataUataUataU#j#j#j#j#j#j#j#j#j#j#j#j#j#JataUataUataUataUataUataUataUataUataUataU#J#Q#J#Q#GaUataUataUataUataUataUataUataUataUataU#j#j#j#j#j#j#j#GataUataUataUataUataUataUataU#j#j#j#j#j#j",
"ceaoceaoceaoceaoceaoceaoce#G#j#j#j#j#j#j#j#j#j#j#j#j#j#jbuaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceaoceat#j#j#j#j#j#j#j#jceaoceaoceaoceaoceaoceaoceao#J#j#j#j#j#j",
"ataUataUataUataUataUataU#G#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jbRataUataUataUataUataUataUataUataUataUataUbYbUbYbUajaUataUataUataUataUaZbUbYbUbY#kat#G#j#j#j#j#j#j#j#GataUataUataUataUataUataUatbUaT.6aZ#j#j#j",
"ceaoceaoceaoceaoceaoceaoah#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jceaoceaoceaoceaoceaoceaoceaoceaoceao.CQtQtQtQtQtQt.K#Caoceaoceaoal#VQtQtQtQtQtQt.M#G#j#j#j#j#j#j#j#jceaoceaoceaoceaoceaoceaoceQtQtQt.w#j#j#j",
"ataUataUataUataUataUataU#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaUataUataUataUataUataUataUataUat.BQtQtQtQtQtQtQtQtQt#VataUataU.uQtQtQtQtQtQtQtQt.u#G#j#j#j#j#j#j#jataUataUataUataUataUataUatQtQtQt#L#Q#J#Q",
"ceaoceaoceaoceaoceaoceaoceaoaf#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#Jceaoceaoceaoceaoceaoceaoceao.CQtQtQtQt#v.B.OQtQtQtQt#kaoce#VQtQtQtQt.B#v.OQtQtQt.c#j#j#j#j#j#j#jaqaoceaoceaoceaoceaoceaoceQtQtQt.Baoceao",
"ataUataUataUataUataUataUataUataP#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#JaUataUataUataUataUataUataU#.QtQtQtbYaUataU#LQtQtQt#EaUatQtQtQtQtbUataU#j.gQt#.cq#j#j#j#RcHcq.wdg.B#EalataUataUfm.B#L.BajQtQtQt#LaUataU",
"ceaoceaoceaoceaoceaoceaoceaoceaoce#G#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jceaoceaoceaoceaoceaoceajQtQtQt.Kceaoceao#kQtQtQt.Kao#kQtQtQt.Oaoai#j#j#Gai#j#j#j#jcHQtQtQtQtQtQtQt.Kalaoce#vQtQtQtQtQtQtQtQt.Baoceao",
"ataUataUataUataUataUataUataUataUataUatat#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jbRataUataUataUataUatbUQtQtQt.BataUataUatQtQtQtQtaUbYQtQtQt#L#Q#j#j#j#j#j#j#j#jaZQtQtQtcq.6#RQtQtQt#EaUajQtQtQtQtQtQtQtQtQt#LaUataU",
"ceaoceaoceaoceaoceaoceaoceaoceaoceaoce#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jafatceaoceaoceaoce#sQtQtQt#vceaoceaoceQtQtQtQtao#CQtQtQtan#j#j#j#j#j#j#j#j#j#j#j#jeL#j#G#OQtQtQt.Bao.CQtQtQtaeao#CQtQtQt.Baoceao",
"ataU#j#J#jaUataUataUataUataUataUataU#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaPataUataUatbUQtQtQt.OataU#EaQaZQtQtQtQtaUbYQtQtQtcq#j#j#j#j.X#RaP#j#j#jaP#RcH.GQtQtQtQtQtdgaP#LQtQtQtataUatQtQtQt#LaUataU",
"#J#j#j#j#j#jaiaoceaoceaoceaoceaoceao#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaq#JceaoQtQtQtQtal#sQt.K.BQtQtQt#v#j#JQtQtQtQt#G#j#j#j.gQtQt.g#Gai.uQtQtQtcH.6QtQtQt.w#janQtQtQtceaoceQtQtQt.Baoai#J",
"#j#j#j#j#j#j#j#QataUataUataUataUatao#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jcqQtQtQt.u#KfnQtQtQtQtQtaj#j#j.gQtQtQt.gaj.6#RQtQtQt.G#jcqQtQtQt#G#j#GQtQtQtcq#j#RQtQtQtbT#GaoQtQtQtdg#j#j#j",
"#j#j#j#j#j#j#j#jahatceaoceaoceaoce#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jai.uQtQtQtQtQtQtQtQtQt.g#j#j#j#G#.QtQtQtQtQtQtQtQtQt.6#j.wQtQtQt#OeL.wQtQtQt.w#je3QtQtQt#.cH#.QtQtQt.w#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#JaUataUataU#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaP.GQtQtQtQtQtQtQtQtQtcqaP#j#jaj#.QtQtQtQtQtQtQtb1#j#jaZQtQtQtQtQtQt.uQtQtcq#j#j.cQtQtQtQtQt.uQtQtcq#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#jaiaoceao#J#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#n.wcH.wcH.weL#.QtQtaZ#j#j#j#G#OcH.wcH.wfoal#j#j#j#jaZ.wcH.w.naieL.wcH.w#j#j#j#OcH.wcHe3#n.wcH#O#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#jat#Gao#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#G.c.V#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#G.cQtQt.gai#j#j#j#j.uQtaZ#j#j.6QtQtQt#.eL#j#j#j#j#j#G#.Qt.w#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#G#.Qtb1aj#..G#j#j#j.dQtQt.6#j#j.dQt#j#j#RQtaZ#j#j#j#j.6QtaP#G#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j.cQt.6#j#jaZ.w#G#j#GQtfoQt.n#j#j.w.u#j#j#jQt.w#j#j#j.6.uQtcH#jaZ.wcH#O#j#j#G.weL.waZ#j#j#jaZ.wcH#O#je3cH#j#j.6cHai#G.ccH.c#G#jeL.weL.wcH.6#j#jcH#O#j#OcH.waZ#j#jalcHe3cH.w#G#j#j#OcH.waZ#j#j#j#j",
"#j#j#j#j.G.u#j#j#j#j#j#j#j.c.G#j.VcH#j#jQtcH#j#j#jQtcq#j#j#jaZ#..V.6ajQtcqb1Qt.w#j.6Qt.gaj#j#j#jajQt.db1Qtb1ajQtaZ#j.G.g#G.u#u.6.G.g#j.wQt.caZ#.cq#j#RQtaZcHQtb1cqQtaj#j.dQtcqb1QtcH#j.w.Gb1cqQt#G#j#j#j",
"#j#j#j#jQtcH#j#j#j#j#j#je3Qt.6#j.w.u#j#GQt#n#j#jaiQt.w#j#j#j#jQt.c#j#..u#j#j.wQt#jcHQt#G#j#j#j#j#.Qt.6eLQtcH#jQt.6#nQt#G.cQt.ceL.gQt#jQt.g#j#jcH.w#j#..ne3Qt.c#j#jQt.w#j.w.u#j#jQtfoalQt.geL.cQt.6#j#j#j",
"#j#j#j#jQt.w#j#j#j.waj#j#uQtQtQtQtQt#j.6Qt.6#j#j.dQtaj#j#j#jaZQtaj#jQtcH#j#j.VQt#j.g.V#j#j#j#j#jQt#.cqcHcqb1#j.ucqQt.d#jcqQtcqcHcq.w#GQt#R#j#j.cQtb1QtaPajQtaj#j#GQtcq#jQt.w#j.6Qt.6ajQt#u.wcqcHaZ#j#j#j",
"#j#j#j#j.cQte3#j.wQt.6aZQtaZ#j#j.6QtaifoQt#G#jeLQt.g#j#j#j#j#OQtai#j#..uaieLQtfo#jQt.c#j#j#j#j#j.g.u#jeL.w#G#jcHQt.u#j#j#OQte3#G.weL.6Qtal#j#jeLQt.u#O#jalQt.w#j.wQte3aZQteL#jfoQt#GaiQt.c#j.cfo#j#j#j#j",
"#j#j#j#j#j.cQtQt.V.X#j#..V#j#j#jaZQtaj.wQtQtQt#..daP#j#j#j#jcqQt#j#j#G.gQtQt.d#jaZQtaj#j#j#j#j#j#G.gQt#.aj#j#jb1Qt.X#j#j#j.cQtQt.d#j.dQt#j#j#j.8Qt.u#j#j#j.6.GQt.G.6#j.6Qt.X#j.w.G#j#j.6.GQt#uaP#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#jaZQt#n#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j.d.ucq#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j",
"#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j#j"};


    switch (id) {
        case image0_ID: return QPixmap((const char**)image0_data);
        default: return QPixmap();
    } // switch
    } // icon

};

namespace Ui {
    class QG_DlgInitial: public Ui_QG_DlgInitial {};
} // namespace Ui

QT_END_NAMESPACE

class QG_DlgInitial : public QDialog, public Ui::QG_DlgInitial
{
    Q_OBJECT

public:
    QG_DlgInitial(QWidget* parent = 0, const char* name = 0, bool modal = false, Qt::WindowFlags fl = 0);
    ~QG_DlgInitial();

public slots:
    virtual void setText( const QString & t );
    virtual void setPixmap( const QPixmap & p );
    virtual void ok();

protected slots:
    virtual void languageChange();

private:
    void init();

};

#endif // QG_DLGINITIAL_H
