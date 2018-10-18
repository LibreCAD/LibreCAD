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
#include <QDesktopWidget>
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

    //change text colour
    //this->setStyleSheet( "QLabel, QRadioButton, QSpinBox, QGroupBox, QPushButton,"
    //                     "QTextBox, QLineEdit { color: blue }" );

    this->setFixedSize( 405, 400 );

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
    this->setWindowTitle( "Divide - " + entType );
    entType = entType.toLower();

    QFrame *Fr1 = new QFrame( this );
    Fr1->setFrameStyle( QFrame::Box );
    Fr1->setLineWidth( 1 );
    Fr1->setGeometry( 5, 5, this->width() - 115, 30 );

    QFrame *Fr2 = new QFrame( this );
    Fr2->setFrameStyle( QFrame::Box );
    Fr2->setLineWidth( 1 );
    Fr2->setGeometry( 5, Fr1->height() + Fr1->y() + 5,
                      this->width() - 115, 30 );

    QDialogButtonBox *BB1 = new QDialogButtonBox( this );
    BB1->setGeometry( ( this->width() / 2 ) - 105, this->height() - 30,
                      ( this->width() / 2) - 5, 30 );
    BB1->addButton( tr( "OK" ), QDialogButtonBox::AcceptRole );
    BB1->addButton( tr( "Cancel" ), QDialogButtonBox::RejectRole );
    QObject::connect( BB1, SIGNAL( accepted() ), this, SLOT( onOkClickedSlot() ) );
    QObject::connect( BB1, SIGNAL( rejected() ), this, SLOT( reject() ) );

    QGroupBox *G1 = new QGroupBox( this );
    G1->setFont( font2 );
    G1->setTitle( "Layers" );
    G1->setVisible( true );
    G1->setGeometry( 5, Fr2->height() + Fr2->y() + 2,
                     this->width() - 115,
                     this->height() - ( Fr2->height() + Fr2->y() + 2 ) - 30 );

    QLabel *L1 = new QLabel( Fr1 );
    L1->setGeometry( 5, 0, 50, Fr1->height() );
    L1->setFont( font1 );
    L1->setText( "Ticks" );

    QLabel *L2 = new QLabel( Fr2 );
    L2->setGeometry( 5, 0, 50, Fr2->height() );
    L2->setFont( font1 );
    L2->setText( "Size" );

    Sp1 = new QSpinBox( Fr1 ); //size
    Sp1->setGeometry( 60, 0, 45, 30 );
    Sp1->setFont( font1 );
    Sp1->setMaximum( 99 );
    Sp1->setMinimum( 0 );
    Sp1->setValue( 0 );
    QObject::connect( Sp1, SIGNAL( valueChanged( int ) ), this,
                      SLOT( onSizeChangedSlot( int ) ) );

    Sp2 = new QSpinBox( Fr2 ); //qty
    Sp2->setGeometry( 60, 0, 45, 30 );
    Sp2->setFont(font1);
    Sp2->setMaximum( 10 );
    Sp2->setMinimum( 1 );
    Sp2->setValue(5);
    QObject::connect( Sp2, SIGNAL( valueChanged( int ) ), this,
                      SLOT( onQtyChangedSlot( int ) ) );

    QLabel *L3 = new QLabel( Fr1 );
    L3->setGeometry( Sp1->x() + Sp1->width() + 5, 0, 45, Fr1->height() );
    L3->setFont( font1 );
    L3->setText( "Qty." );

    QLabel *L4 = new QLabel( Fr2 );
    L4->setGeometry( Sp2->x() + Sp2->width() + 5, 0, 45, Fr2->height() );
    L4->setFont( font1 );
    L4->setText("%");

    R1 = new QRadioButton( Fr1 ); // ticks on/off
    R1->setGeometry( L3->x() + L3->width() + 10, 1, 150, Fr1->height() );
    R1->setFont( font2 );
    R1->setText("Ticks - Off");
    ticksShowHideFlag = false;
    QObject::connect( R1, SIGNAL( toggled( bool ) ), this,
                      SLOT( onOffTicksSlot( bool ) ) );

    R2 = new QRadioButton( Fr2 ); // breaks on/off
    R2->setGeometry( L4->x() + L4->width() + 10, 1, 150, Fr2->height() );
    R2->setFont( font2 );
    R2->setText( "Breaks - Off" );
    breaksOnOffFlag = false;
    QObject::connect (R2, SIGNAL( toggled( bool ) ), this,
                      SLOT( onOffBreaksSlot( bool ) ) );

    QList<QString> layerList = doc->getAllLayer();
    QString activeLayerName = doc->getCurrentLayer();
    QString num;
    QGridLayout *layout = new QGridLayout;

    for( int i = 0; i < layerList.size(); i++ )
    {
        QRadioButton *RB = new QRadioButton;
        RB->setFixedHeight( 14 );
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

        layout->addWidget( RB );
    }

    QScrollArea *Sa1 = new QScrollArea( G1 );
    Sa1->setGeometry( 0, 20, G1->width(), G1->height() - 39 );
    Sa1->setLineWidth( 1 );

    QLabel *L5 = new QLabel( G1 );
    L5->setGeometry( 0, G1->height() - 20, 30, 20 );
    L5->setFont( font4 );
    L5->setText( "<i>New</i>" );

    Le2 = new QLineEdit( G1 ); // new layer
    Le2->setGeometry( 30, L5->y(), G1->width() - 30, 20 );
    Le2->setFont( font4 );

    QWidget *containerWidget = new QWidget;
    containerWidget->setLayout( layout );

    Sa1->setWidgetResizable(true);
    Sa1->setWidget( containerWidget );

    if ( entType == "line" )
    {
        dataToReturn.append( "LINE:" );
        choice( 5, 1, font4 );
    } //end line

    else if ( entType == "circle" )
    {
        dataToReturn.append( "CIRCLE:" );

        QFrame *Cr2 = new QFrame( this );
        Cr2->setFrameStyle( QFrame::Box );
        Cr2->setLineWidth(1);
        Cr2->setGeometry( this->width() - 105, 5, 100, 90 );

        QLabel *C4 = new QLabel( Cr2 );
        C4->setGeometry( 5, 0, Cr2->width() - 10, Cr2->height() );
        C4->setFont( font4 );
        C4->setText( "Enter start\nangle, in\ndecimal degrees\n"
                     "0° at 3 o'clock\ngoes\nanti-clockwise." );

        Le1 = new QLineEdit( this );
        Le1->setGeometry( Cr2->x(), C4->height() + 10, Cr2->width(), 20 );

        QDoubleValidator *doubleVal = new QDoubleValidator
                ( -999999.999, 999999.999, 3, this ); //± 1 million - 0.001
        doubleVal->setNotation( QDoubleValidator::StandardNotation );
        Le1->setValidator( doubleVal );
        Le1->installEventFilter( this );

        QObject::connect( Le1, SIGNAL( textChanged( const QString & ) ), this,
                          SLOT( onStartAngleChangedSlot ( const QString & ) ) );

        choice( ( Le1->y() + Le1->height() + 5 ), 2, font4 );
    } //end circle

    else if ( entType == "arc" )
    {
        dataToReturn.append( "ARC:" );
        choice( 5, 2, font4 );
    }

    else if ( entType == "poyline" )
    {
        dataToReturn.append( "POLYLINE:" );
    }
}

void dividedlg::choice( int yPos, int msgText, QFont font )
{
    QString msg1;
    QString msg2;
    QString msg3;

    switch ( msgText ) {
    case ( 1 ): { // line
        msg1 = ( "Ticks\nAbove/Below." );
        msg2 = ( "Ab~" );
        msg3 = ( "Be~" );
        break;
    }
    case ( 2 ): { //circle, arc
        msg1 = ( "Ticks\nOutside/Inside." );
        msg2 = ( "Out" );
        msg3 = ( "In" );
        break;
    }
    default:
        break;
    }

    QFrame *Cr1= new QFrame( this );
    Cr1->setFrameStyle( QFrame::Box );
    Cr1->setLineWidth(1);
    Cr1->setGeometry( this->width() - 105, yPos, 100, 55 );

    QLabel *C1 = new QLabel( Cr1 );
    C1->setGeometry( 5, 0, Cr1->width() - 10, Cr1->height() );
    C1->setFont( font );
    C1->setContentsMargins( 0, 3, 0, 0 );
    C1->setAlignment( Qt::Alignment( Qt::AlignTop ) );
    C1->setText( msg1 );

    QRadioButton *R1 = new QRadioButton( C1 );
    R1->setGeometry( 7, C1->height() - 20, 50, 20 );
    R1->setText( msg2 );
    R1->setObjectName( "o" );
    QObject::connect(R1,SIGNAL( toggled( bool ) ), this,
                     SLOT( onInOutSlot( bool ) ) );
    R1->setChecked( true );

    QRadioButton *R2 = new QRadioButton( C1 );
    R2->setGeometry( 51, C1->height() - 20, 50, 20 );
    R2->setText( msg3 );
    R2->setObjectName( "i" );
    QObject::connect(R2, SIGNAL( toggled( bool ) ), this,
                     SLOT( onInOutSlot( bool ) ) );

    QFrame *Cr2= new QFrame( this );
    Cr2->setFrameStyle( QFrame::Box );
    Cr2->setLineWidth( 1 );
    Cr2->setGeometry( this->width() - 105, this->height() - 82, 100, 77 );

    QLabel *C2 = new QLabel( Cr2 );
    C2->setGeometry( 5, 0, Cr2->width() - 10, Cr2->height() );
    C2->setFont( font );
    C2->setContentsMargins( 0, 3, 0, 0 );
    C2->setAlignment( Qt::Alignment( Qt::AlignTop ) );
    C2->setText( "<i>\"New\"</i><br>Enter name<br>for a new layer<br>"
                 "(if required?) to<br>draw ticks on." );
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
