/****************************************************************************
*  dividedlg.cpp - divide lines, circles and arcs                           *
*                                                                           *
*  Copyright (C) 2018 mad-hatter                                            *
*                                                                           *
*  This library is free software, licensed under the terms of the GNU       *
*  General Public License as published by the Free Software Foundation,     *
*  either version 2 of the License, or (at your option) any later version.  *
*  You should have received a copy of the GNU General Public License        *
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
****************************************************************************/

#include <QtGui>
#include <QApplication>
#include <QDialog>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QMessageBox>
#include <QLabel>
#include <complex>

#include "dividedlg.h"

//#include <QDebug>

dividedlg::dividedlg( Document_Interface *doc, QString passedData,
                      QWidget *parent ) : QDialog( parent )
{
    //hide titie bar '?'
    this->setWindowFlags( this->windowFlags() & ~Qt::WindowContextHelpButtonHint );

    QFont font1, font2, font3, font4;
    font1.setPointSize( 14 );
    font2.setPointSize( 12 );
    font3.setPointSize( 11 );
    font4.setPointSize(  9 );

    d = doc; //Document_Interface

    QList<QString> data = ( passedData.split
                            ( QRegularExpression ( "[\\n\\t\\r]" ) ) );

    QList<QString> splitList = data.at( 0 ).split( ": " );
    QString entType = splitList.at( 1 );
    this->setWindowTitle( tr("Divide - ") + entType );
    entType = entType.toLower();

    QHBoxLayout *mainLayout = new QHBoxLayout;
    setLayout(mainLayout);

    QVBoxLayout *leftcolayout = new QVBoxLayout;
    mainLayout->addLayout(leftcolayout);

    QFrame *Fr1 = new QFrame();
    Fr1->setFrameStyle( QFrame::Box );
    Fr1->setLineWidth( 1 );
    leftcolayout->addWidget(Fr1);

    QFrame *Fr2 = new QFrame();
    Fr2->setFrameStyle( QFrame::Box );
    Fr2->setLineWidth( 1 );
    leftcolayout->addWidget(Fr2);

    QGroupBox *G1 = new QGroupBox();
    G1->setFont( font2 );
    G1->setTitle(tr("Layers"));
    G1->setVisible( true );
    leftcolayout->addWidget(G1);

    QDialogButtonBox *BB1 = new QDialogButtonBox();
    leftcolayout->addWidget(BB1, 0, Qt::AlignRight);
    BB1->addButton( tr( "OK" ), QDialogButtonBox::AcceptRole );
    BB1->addButton( tr( "Cancel" ), QDialogButtonBox::RejectRole );
    QObject::connect( BB1, SIGNAL( accepted() ), this, SLOT( onOkClickedSlot() ) );
    QObject::connect( BB1, SIGNAL( rejected() ), this, SLOT( reject() ) );

    QHBoxLayout* fr1layout = new QHBoxLayout;
    Fr1->setLayout(fr1layout);
    QLabel *L1 = new QLabel();
    fr1layout->addWidget(L1);
    L1->setFont( font1 );
    L1->setText(tr("Divide at"));

    QHBoxLayout* fr2layout = new QHBoxLayout;
    Fr2->setLayout(fr2layout);
    QLabel *L2 = new QLabel();
    fr2layout->addWidget(L2);
    L2->setFont( font1 );
    L2->setText(tr("Size"));

    Sp1 = new QSpinBox(); // quantity
    fr1layout->addWidget(Sp1);
    Sp1->setFont( font1 );
    Sp1->setMaximum( 99 );
    Sp1->setMinimum( 0 );
    Sp1->setValue( 0 );
    QObject::connect( Sp1, SIGNAL( valueChanged( int ) ), this,
                      SLOT( onQtyChangedSlot( int ) ) );

    Sp2 = new QSpinBox(); // size
    fr2layout->addWidget(Sp2);
    Sp2->setFont(font1);
    Sp2->setMaximum( 10 );
    Sp2->setMinimum( 1 );
    Sp2->setValue(5);
    QObject::connect( Sp2, SIGNAL( valueChanged( int ) ), this,
                      SLOT( onSizeChangedSlot( int ) ) );

    QLabel* L3 = new QLabel();
    fr1layout->addWidget(L3);
    L3->setFont( font1 );
    L3->setText(tr("places"));
    fr1layout->addSpacing(10); // otherwise text runs too close to R1 below

    QLabel* L4 = new QLabel();
    fr2layout->addWidget(L4);
    L4->setFont( font1 );
    L4->setText("%");

    R1 = new QRadioButton(Fr1); // ticks on/off
    fr1layout->addWidget(R1);
    R1->setFont( font2 );
    R1->setText(tr("Ticks - Off"));
    ticksShowHideFlag = false;
    QObject::connect( R1, SIGNAL( toggled( bool ) ), this,
                      SLOT( onOffTicksSlot( bool ) ) );

    R2 = new QRadioButton(Fr2); // breaks on/off
    fr2layout->addWidget(R2);
    R2->setFont( font2 );
    R2->setText(tr("Breaks - Off"));
    breaksOnOffFlag = false;
    QObject::connect (R2, SIGNAL( toggled( bool ) ), this,
                      SLOT( onOffBreaksSlot( bool ) ) );

    QList<QString> layerList = doc->getAllLayer();
    QString activeLayerName = doc->getCurrentLayer();
    QString num;
    QGridLayout *laylayout = new QGridLayout;

    for( int i = 0; i < layerList.size(); i++ )
    {
        QRadioButton *RB = new QRadioButton;
        RB->setFont( font4 );
        num = QString::number( i );
        RB->setObjectName( num );
        RB->setText( num + " - " + layerList.at( i ) );

        QObject::connect( RB, SIGNAL( toggled( bool ) ), this,
                          SLOT( onWhichButtonSlot( bool ) ) );

        if ( activeLayerName == layerList.at( i ) )
        {
            RB->setChecked( true );
            activeLayer = i;
        }

        laylayout->addWidget( RB );
    }

    QVBoxLayout* g1layout = new QVBoxLayout;
    G1->setLayout(g1layout);

    QScrollArea *Sa1 = new QScrollArea( G1 );
    g1layout->addWidget(Sa1);
    Sa1->setLineWidth( 1 );
    Sa1->setWidgetResizable(true);
    QWidget *containerWidget = new QWidget;
    containerWidget->setLayout( laylayout );
    Sa1->setWidget( containerWidget );

    QHBoxLayout* newlaylayout = new QHBoxLayout;
    QLabel *L5 = new QLabel( G1 );
    newlaylayout->addWidget(L5);
    L5->setFont( font4 );
    L5->setText(tr("<i>New</i>"));

    Le2 = new QLineEdit( G1 ); // new layer
    newlaylayout->addWidget(Le2);
    Le2->setFont( font4 );
    g1layout->addLayout(newlaylayout);

    QVBoxLayout* rtcolayout = new QVBoxLayout;
    mainLayout->addLayout(rtcolayout);

    if ( entType == "line" )
    {
        dataToReturn.append( "LINE:" );
	rtcolayout->addWidget(choice(STRAIGHT, font4));
    } //end line

    else if ( entType == "circle" )
    {
        dataToReturn.append( "CIRCLE:" );

        QFrame *Cr2 = new QFrame();
        Cr2->setFrameStyle( QFrame::Box );
        Cr2->setLineWidth(1);
	rtcolayout->addWidget(Cr2);

	QVBoxLayout* anglayout = new QVBoxLayout();
	Cr2->setLayout(anglayout);
	
        QLabel *C4 = new QLabel();
	anglayout->addWidget(C4);
        C4->setFont( font4 );
        C4->setText(tr("Enter start\nangle, in\ndecimal degrees\n"
                       "0° at 3 o'clock\ngoes\nanti-clockwise."));

        Le1 = new QLineEdit();
	anglayout->addWidget(Le1);

        QDoubleValidator *doubleVal = new QDoubleValidator
                ( -999999.999, 999999.999, 3, this ); //± 1 million - 0.001
        doubleVal->setNotation( QDoubleValidator::StandardNotation );
        Le1->setValidator( doubleVal );
        Le1->installEventFilter( this );

        QObject::connect( Le1, SIGNAL( textChanged( const QString & ) ), this,
                          SLOT( onStartAngleChangedSlot ( const QString & ) ) );

        rtcolayout->addWidget(choice(CURVED, font4));
    } //end circle

    else if ( entType == "arc" )
    {
        dataToReturn.append( "ARC:" );
	rtcolayout->addWidget(choice(CURVED, font4));
    }

    else if ( entType == "polyline" )
    {
        dataToReturn.append( "POLYLINE:" );
    }
    rtcolayout->addStretch();

    QLabel *C2 = new QLabel();
    rtcolayout->addWidget(C2);
    C2->setStyleSheet("border: 0.2ex solid black");
    C2->setFont(font4);
    C2->setContentsMargins( 0, 3, 0, 0 );
    C2->setAlignment( Qt::Alignment( Qt::AlignTop ) );
    C2->setText(tr("<i>\"New\"</i><br>Enter name<br>for a new layer<br>"
                   "(if required?) to<br>draw ticks on."));
}

QFrame* dividedlg::choice(ElementKind ek, QFont font )
{
    QString msg1 = tr("Ticks");
    QString msg2;
    QString msg3;

    switch ( ek ) {
    case STRAIGHT:
        msg2 = tr("Above");
        msg3 = tr("Below");
        break;

    case CURVED:
        msg2 = tr("Outside");
        msg3 = tr("Inside");
        break;

    default:
        break;
    }

    QFrame *Cr1= new QFrame();
    Cr1->setFrameStyle( QFrame::Box );
    Cr1->setLineWidth(1);

    QVBoxLayout* tickloclayout = new QVBoxLayout;
    Cr1->setLayout(tickloclayout);

    QLabel *C1 = new QLabel( Cr1 );
    tickloclayout->addWidget(C1);
    C1->setFont( font );
    C1->setContentsMargins( 0, 3, 0, 0 );
    C1->setAlignment( Qt::Alignment( Qt::AlignTop ) );
    C1->setText( msg1 );

    QHBoxLayout* tickchoiceslayout = new QHBoxLayout;
    tickloclayout->addLayout(tickchoiceslayout);

    QRadioButton *R1 = new QRadioButton( C1 );
    tickchoiceslayout->addWidget(R1);
    R1->setText( msg2 );
    R1->setObjectName( "o" );
    QObject::connect(R1,SIGNAL( toggled( bool ) ), this,
                     SLOT( onInOutSlot( bool ) ) );
    R1->setChecked( true );

    QRadioButton *R2 = new QRadioButton( C1 );
    tickchoiceslayout->addWidget(R2);
    R2->setText( msg3 );
    R2->setObjectName( "i" );
    QObject::connect(R2, SIGNAL( toggled( bool ) ), this,
                     SLOT( onInOutSlot( bool ) ) );

    return Cr1;
}

bool dividedlg::eventFilter( QObject * obj, QEvent * event )
{
    if ( Le1->hasFocus() ) //QlineEdit - start angle
    {
        backSpace = true; //see onStartAngleChangedSlot

        if ( event->type() == QEvent::KeyPress )
        {
            QKeyEvent * keyEvent = static_cast<QKeyEvent *>( event );
            if ( keyEvent->key() == Qt::Key_Backspace )
                backSpace = false;
        }
    }

    return ( QWidget::eventFilter( obj, event ) );
}

//auto add decinal point after 6 digits
void dividedlg::onStartAngleChangedSlot( const QString & passed )
{
    if ( backSpace ) //allows backspace on len = 6 or -7 & decimal point
    {                //see eventFilter
        int len = passed.length();
        bool noPoint = ( ! passed.contains( "." ) );
        bool addPoint = false;
        if (QChar('-') == passed.at( 0)) { //minus
            if ( ( len == 7 ) && noPoint ) addPoint = true;
        }
        else
            if ( ( len == 6 ) && noPoint ) addPoint = true;

        if ( addPoint ) Le1->setText( Le1->text() + "." );
    }
}

void dividedlg::onInOutSlot( bool state )
{
    QObject *object = QObject::sender();
    QRadioButton *btn = qobject_cast<QRadioButton *>( object );

    inOut = ( state && btn->objectName() == "o" ) ? true : false;
}

void dividedlg::onSizeChangedSlot( int s )
{
    size = s;
}

void dividedlg::onQtyChangedSlot( int q )
{
    qty = q;
}

void dividedlg::onOffTicksSlot( bool state )
{
    state ? R1->setText( "Ticks - On" ) : R1->setText( "Ticks - Off" );

    ticksShowHideFlag = state;
}

void dividedlg::onOffBreaksSlot( bool state )
{
    state ? R2->setText( "Breaks - On" ) : R2->setText( "Breaks - Off" );

    breaksOnOffFlag = state;
}

void dividedlg::onWhichButtonSlot( bool state )
{
    QObject *object = QObject::sender();
    QRadioButton *btn = qobject_cast<QRadioButton *>(object);

    if ( state )
    {
        d->setLayer( ( btn->text() ).split( "- " ).at( 1 ) );
        btn->setText( btn->text() + " (ticks)" );
        activeLayer = btn->objectName().toInt();
    }
    else
        btn->setText( btn->text().remove( " (ticks)" ) );
}

void dividedlg::onOkClickedSlot() //OK quit dividedlg
{
    dataToReturn.append( QString::number( Sp1->value() ).append(":") ); //ticks
    dataToReturn.append( QString::number( Sp2->value() ).append(":") ); //size

    dataToReturn.append( R1->isChecked() ? "t:" : "f:" ); //ticks on/off
    dataToReturn.append( R2->isChecked() ? "t:" : "f:" ); //breaks on/off

    if ( dataToReturn.startsWith( "CI" ) ) //circle
        dataToReturn.append( Le1->text().simplified().append( ":" ) );

    dataToReturn.append( inOut ? "o:" : "i:" ); //Ternary Operator '?'

    QString test = Le2->text().simplified(); //QLineEdit, new layer name

    dataToReturn.append( test.isEmpty() ? ( d->getCurrentLayer()
                                            .remove( " (ticks)" ).append( ":" ) )
                                        : ( test.append( ":" ).append( "lay" ) ) );
    emit returnData( dataToReturn );
    this->accept();
}

dividedlg::~dividedlg()
{
    this->deleteLater();
}
