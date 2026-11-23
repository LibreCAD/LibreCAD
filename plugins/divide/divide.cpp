/****************************************************************************
*  divide.cpp - divide lines, circles and arcs                              *
*                                                                           *
*  Copyright (C) 2018 mad-hatter                                            *
*  some code borrowed from                                                  *
*  list.cpp - Copyrighted by Rallaz, rallazz@gmail.com                      *
*                                                                           *
*  This library is free software, licensed under the terms of the GNU       *
*  General Public License as published by the Free Software Foundation,     *
*  either version 2 of the License, or (at your option) any later version.  *
*  You should have received a copy of the GNU General Public License        *
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
****************************************************************************/

#include <cmath>
#include <complex>

#include <QtGui>
#include <QApplication>
#include <QDialog>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QGridLayout>
#include <QMessageBox>
#include <QLabel>

#include "divide.h"

#include "dividedlg.h"


QString divide::name() const
{
    return ( tr( "Divide" ) );
}

PluginCapabilities divide::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
        << PluginMenuLocation( "plugins_menu", tr( "Divide" ),
            tr("Divide test tip..."));
    return pluginCapabilities;
}

void divide::execComm( Document_Interface *doc,
                       [[maybe_unused]] QWidget *parent,
                       [[maybe_unused]] QString cmd )
{
    d = doc;

    QList<Plug_Entity *> obj;
    bool yes = doc->getSelect( &obj, tr( "Select a line, circle or"
                                         " arc and press return" ) );
    if ( ! yes || obj.isEmpty() || ( obj.size() > 1 ) ) //none or multiple
    {                                                   //entity selection
        QMessageBox msgBox;
        msgBox.setStyleSheet( "QLabel, QPushButton { color: blue; }"
                              "QMessageBox { border: none;"
                              //"background: rgb( 255, 0, 0 );" //white is default
                              //"font-family: Arial;" //use default font
                              //text colour is blue from QLabel
                              "font-style: italic; font-size: 11pt; }" );
        msgBox.setWindowTitle( tr( "Error" ) );

        QString text = ( "Select a line, circle or arc.<br>" );
        if ( obj.size() > 1 )
            text.append( "One entity only!<br>" );
        text.append( "Press \"<b>Enter/Return</b>\"." );
        msgBox.setText( text );
        msgBox.setIcon( QMessageBox::Warning );

/*  Why are we messing with screen geometry just to show an error message?
 *    Assume Qt will just take care of it!
 *
 *        msgBox.show(); //need show to get msgBox size
 *        QPoint centerXY = findWindowCentre();
 *        int x = centerXY.rx() - ( msgBox.width() / 2 );
 *        int y = centerXY.ry() - ( msgBox.height() / 2 );
 *
 *        QRect screenGeometry = QApplication::desktop()->availableGeometry();
 *        //in case msgBox is wholly or partially offscreen
 *        if ( x >= ( screenGeometry.width() - msgBox.width() ) )
 *            x = QApplication::desktop()->width() - msgBox.width() - 10;
 *        if ( y >= ( screenGeometry.height() - msgBox.height() ) )
 *            y = QApplication::desktop()->height() - msgBox.height() - 60;
 *
 *        msgBox.move( x, y );
 *********************************/
        msgBox.exec();

        while ( ! obj.isEmpty() )
            delete obj.takeFirst();

        return;
    }

    // The geometry comes from the entity's own data, never from text shown to
    // the user: that text is translated and formatted in the drawing's units.
    QHash<int, QVariant> entData;
    obj.at( 0 )->getData( &entData );
    const int entityType = entData.value( DPI::ETYPE ).toInt();
    const QPointF entStart { entData.value( DPI::STARTX ).toDouble(),
                             entData.value( DPI::STARTY ).toDouble() };

    dividedlg dlg( d, entityType, entityName( entityType ), parent );
    QObject::connect( &dlg, SIGNAL( returnData( QString ) ), this,
                      SLOT( gotReturnedDataSlot( QString ) ) );
    if ( dlg.exec() == QDialog::Accepted )
    {
        QList<QString> data = ( returnedData.split( ":" ) );

        int qty = data.at( 1 ).toInt(); //defaults to zero if
        if ( qty <= 0 ) return;         //qty is empty

        bool ticks = ( data.at( 3 ) == "t" ) ? true : false;
        bool breaks = ( data.at( 4 ) == "t" ) ? true : false;

        if ( ticks || breaks )
        {
            double radius { 0.0 };
            double tickAngle { 0.0 };
            double tickLength { 0.0 };
            QPointF startX { 0.0, 0.0 };
            QPointF startXY { 0.0, 0.0 };
            QPointF centerCircle { 0.0, 0.0 };

            QString oldLayer = doc->getCurrentLayer();
            int dataSize = data.size();
            if ( data.at( dataSize - 1 ) == "lay" )
                doc->setLayer( data.at( dataSize - 2 ) ); //layer for ticks

            doc->updateView();
            QString entType = data.at( 0 ).simplified().toLower();

            //***********
            if ( entityType == DPI::CIRCLE )
            {
                radius = entData.value( DPI::RADIUS ).toDouble();
                centerCircle = entStart;

                //if data.at(5) is empty, startAngle defaults to zero
                double startAngle = ( data.at( 5 ).toDouble() * M_PI ) / 180.0; //radians
                tickLength = ( 2.0 * radius * ( data.at( 2 ).toDouble() / 100.0 ) ); //%

                if ( data.at( dataSize - 3 ) == "i") //ticks inside/outside circle
                    tickLength *= -1;

                for ( int i = 1; i <= qty; i++ ) {
                    tickAngle = ( ( ( 2 * M_PI ) / qty ) * i ) + startAngle;

                    if ( ticks )
                        drawTick( ( findStartX( radius, tickAngle, centerCircle ) ),
                                  tickLength, tickAngle );

                    if ( breaks )
                        segment( & centerCircle, radius,
                                 ( startAngle * (180.0 / M_PI) )
                                 + ( ( 360.0 / qty ) * ( i - 1 ) ),
                                 ( 360.0 / qty ), entType );
                }
            } // end if CIRCLE

            //***********
            else if ( entityType == DPI::LINE )
            {
                startXY = entStart;
                QPointF endXY { entData.value( DPI::ENDX ).toDouble(),
                                      entData.value( DPI::ENDY ).toDouble() };
                const QPointF inc = endXY - startXY;
                const double length = std::hypot( inc.x(), inc.y() );
                const int size = data.at( 2 ).toInt();

                // line direction in degrees, 0 to 360
                double direction = 0.0;
                if ( length > 0.0 )
                {
                    direction = std::asin( inc.y() / length ) * 180.0 / M_PI;
                    if ( inc.x() < 0 ) direction = 180.0 - direction;
                    if ( direction < 0 ) direction += 360.0;
                }
                tickAngle = ( ( direction + 90.0 ) * M_PI ) / 180; //rads
                tickLength = length * size / 100.0; //%

                //line going right to left
                if ( inc.x() < 0 && tickAngle >= M_PI )
                    tickAngle -= M_PI; //180°

                if ( data.at( dataSize - 3 ) == "i") //ticks above/below linw
                    tickLength *= -1;

                qty += 1;
                for ( int i = 1; i <= qty; i++ ) {
                    startX.setX( ( ((endXY.rx() - startXY.rx()) / qty) * i )
                                 + startXY.rx() );
                    startX.setY( ( ((endXY.ry() - startXY.ry()) / qty) * i )
                                 + startXY.ry() );

                    if ( ticks && ( i < qty ) )
                        drawTick( startX, tickLength, tickAngle );

                    if ( breaks )
                        segmentLine( startX, endXY, startXY, entType, qty, i );
                }
            } //end if LINE

            //***********
            else if ( entityType == DPI::ARC )
            {
                centerCircle = entStart;
                radius = entData.value( DPI::RADIUS ).toDouble();
                const double initial = entData.value( DPI::STARTANGLE ).toDouble(); //rads
                const double final = entData.value( DPI::ENDANGLE ).toDouble(); //rads
                const double sweepStart = ( initial > final ) ? initial - 2.0 * M_PI
                                                              : initial;
                const double arcLength = ( final - sweepStart ) * radius;

                tickLength = ( 2 * radius * ( data.at( 2 ).toDouble() / 100.0 ) ); //%
                if ( data.at( dataSize - 3 ) == "i" ) //ticks inside/outside arc
                    tickLength *= -1;

                qty += 1;

                double angle = ( final > initial ) ? final : initial;
                double diff = ( final > initial ) ? ( initial - final ) / qty
                                                  : ( arcLength / radius ) / qty;

                for ( int i = 1; i <= qty; i++ ) {
                    tickAngle = ( diff * i ) + angle;

                    if ( ticks && ( i < qty ) )
                        drawTick( ( findStartX( radius, tickAngle, centerCircle ) ),
                                  tickLength, tickAngle );

                    if ( breaks )
                        segment( & centerCircle, radius,
                                 ( ( tickAngle * 180.0 ) / M_PI ),
                                 ( ( diff * 180.0 ) / M_PI ) * -1, entType );
                }
            } //end if ARC

            //***********
/*            else if ( entType.startsWith( "po" ) ) //polyline - to do
            {
                qDebug() << "*** pData " << pData;
                qDebug() << "*** data  " << data;
                qDebug() << "*** ret   " << returnedData;

                QVector<double> XX;
                QVector<double> YY;
                //QVector<double> lengths;
                int count = 0;
                for ( int i = 0; i < pData.size(); i++ )
                {
                    if (pData.at( i ).contains("point"))
                    {
                        count += 1;
                        XX.append( (pData.at( i ).split(":").at( 1 ).split("=")
                                    .at( 1 ).split(" ").at( 0 )).toDouble() ) ;
                        YY.append( pData.at( i ).split("=").at( 2 ).toDouble() );
                   }
                }
                double totalLength = 0;
                for ( int i = 0; i < count - 1; i++ )
                {
                    totalLength += findHypLength( XX.at( i ), XX.at( i + 1 ),
                                              YY.at( i ), YY.at( i + 1 ) );

                }
                x = ( dataSize.split(":") );
                QString tickQty = x.at( 1 );
                double tickQ = ( tickQty.remove("\n") ).toDouble();
                double div = totalLength / tickQ;
                qDebug() << "X" << XX << "Y" << YY << count
                         << totalLength << tickQ << div;
            } //POLYLINE              */

            doc->setLayer( oldLayer );
        } //end if ( ticks || breaks )
        doc->updateView(); //updates & removes highlights
        while ( ! obj.isEmpty() )
            delete obj.takeFirst();
    } //end dlg.exec()
//    else //rejected
//    {
//        qDebug() << "reject " << returnedData; //cancel button
//        strData.prepend( strEntity.arg
//                         (tr("MUST be a line, circle or arc")) );
//        while ( ! obj.isEmpty() )
//            delete obj.takeFirst();
    //}
} // end execComm


void divide::segmentLine( QPointF startPoint, QPointF lineEnd, QPointF lineStart,
                          QString type, int qty, int count ) const {
    QString layerName = ( "Break layer - " + type );
    QString cl = d->getCurrentLayer();
    d->setLayer( layerName );

    static QPointF newStart;

    if ( count == 1 ) { //1st line
        d->addLine( &lineStart, &startPoint );
        newStart = startPoint;
    }
    else if ( ( count > 1 ) && ( count < qty ) ) { //mid line
        d->addLine( &newStart, &startPoint );
        newStart = startPoint;
    }
    else if ( count == qty) //end line
        d->addLine( &newStart, &lineEnd );

    d->setLayer( cl ); //restore layer
}

void divide::segment( QPointF * centerCircle, double radius,
                      double angle, double arcAngle, QString type ) const {
    QString layerName = ( "Break layer - " + type );
    QString cl = d->getCurrentLayer();
    d->setLayer( layerName );

    d->addArc( centerCircle, radius, angle, ( angle + arcAngle ) );

    d->setLayer( cl );
}

double divide::findHypLength( double h1, double h2, double v1, double v2 )
{
    return ( std::hypot( h1 - h2, v1 - v2 ) ); //needs (C++14 or later) - hypotenuse
    //see - http://www.cplusplus.com/reference/cmath/hypot/  - C99
}

//line in any quadrant
//angle ± value in radians (range - double)
//startX, startY and length ± value (range - double)
//take care with values near double max and min limits ???
QPointF divide::findLineEndPoint( double startX, double startY,
                                  double length, double angle ) //radians
{
    auto endPoint = ( std::complex<double> ( startX, startY )
                      + std::polar<double> ( length, angle ) );

    return ( QPointF ( endPoint.real(), endPoint.imag() ) );
}

void divide::drawTick( QPointF startX, double tickLength, double tickAngle )
{
    QPointF result = findLineEndPoint( startX.rx(), startX.ry(),
                                       tickLength, tickAngle );
    d->addLine( & startX, & result );
}

QPointF divide::findStartX( double radius, double angle,  QPointF centerCircle )
{
    QPointF startPoint;

    startPoint.setX( ( radius * cos( angle ) ) + centerCircle.rx() );
    startPoint.setY( ( radius * sin( angle ) ) + centerCircle.ry() );

    return startPoint;
}

void divide::gotReturnedDataSlot( QString data )
{
    returnedData = data;
}

QString divide::entityName( int entityType ) const
{
    switch ( entityType )
    {
    case DPI::LINE:
        return tr( "LINE" );
    case DPI::ARC:
        return tr( "ARC" );
    case DPI::CIRCLE:
        return tr( "CIRCLE" );
    case DPI::POLYLINE:
        return tr( "POLYLINE" );
    default:
        return tr( "MUST be a line, circle or arc" );
    }
}

//return center of active window
QPoint divide::findWindowCentre()
{
    QPoint centXY;

    centXY.setX( ( QApplication::activeWindow()->width() / 2 )
                 + QApplication::activeWindow()->x() );
    centXY.setY( ( QApplication::activeWindow()->height() / 2 )
                 + QApplication::activeWindow()->y() );

    return (centXY);
}
