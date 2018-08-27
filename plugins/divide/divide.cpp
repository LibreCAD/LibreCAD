/****************************************************************************
*  divide.cpp - divide lines, circles and arcs                              *
*                                                                           *
*  Copyright (C) 2018 mad-hatter                                            *
*  somme code borrowed from                                                 *
*  list.cpp - Copyrighted by Rallaz, rallazz@gmail.com                      *
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
#include <QMessageBox>
#include <complex>

#include "divide.h"
#include "dividedlg.h"

#include <QDebug>

class QWidget;

QString divide::name() const
{
    return ( tr( "Divide" ) );
}

PluginCapabilities divide::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation( "plugins_menu", tr( "Divide" ) );
    return pluginCapabilities;
}

void divide::execComm( Document_Interface *doc, QWidget *parent,
                              QString cmd )
{
    Q_UNUSED ( parent );
    Q_UNUSED ( cmd );

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
                              //"font-family: Arial;"
                              "font-style: italic; font-size: 11pt; }" );
                              //text colour is blue from QLabel;
        msgBox.setWindowTitle( tr( "Error" ) );

        QString text = ( "Select a line, circle or arc.<br>" );
        if ( obj.size() > 1 )
            text.append( "One entity only!<br>" );
        text.append( "Press \"<b>Enter/Return</b>\"." );
        msgBox.setText( text );
        msgBox.setIcon( QMessageBox::Warning );

        msgBox.show(); //need show to get msgBox size
        QPoint centerXY = findWindowCentre();
        int x = centerXY.rx() - ( msgBox.width() / 2 );
        int y = centerXY.ry() - ( msgBox.height() / 2 );

        QRect screenGeometry = QApplication::desktop()->availableGeometry();
        //in case msgBox is wholly or partially offscreen
        if ( x >= ( screenGeometry.width() - msgBox.width() ) )
            x = QApplication::desktop()->width() - msgBox.width() - 10;
        if ( y >= ( screenGeometry.height() - msgBox.height() ) )
            y = QApplication::desktop()->height() - msgBox.height() - 60;

        msgBox.move( x, y );
        msgBox.exec();

        return;
    }

    QString passedData;
    for ( int i = 0; i < obj.size(); ++i ) {
        passedData.append( QString("%1 %2: ").arg( tr("n") ).arg( i + 1 ) );
        passedData.append( getStrData(obj.at( i )) );
        passedData.append( "\n" );
    }

    dividedlg dlg( d, passedData, parent );
    QObject::connect( &dlg, SIGNAL( returnData( QString ) ), this,
                      SLOT( gotReturnedDataSlot( QString ) ) );
    if ( dlg.exec() == QDialog::Accepted )
    {
        QList<QString> data = ( returnedData.split( ":" ) );

        int qty = data.at( 1 ).toInt(); //defaults to zero
        if ( qty <= 0 ) return;         //if lineedit is empty

        bool ticks = false;
        bool breaks = false;
        if ( data.at( 3 ) == "t" ) ticks = true;
        if ( data.at( 4 ) == "t" ) breaks = true; //breaks - to do

        if ( ticks || breaks )
        {
            double radius { 0.0 };
            double centerX { 0.0 };
            double centerY { 0.0 };
            double tickAngle { 0.0 };
            double tickLength { 0.0 };
            QPointF startX (0.0, 0.0);
            QPointF result (0.0, 0.0);
            QString test = "";

            QString oldLayer = doc->getCurrentLayer();
            int dataSize = data.size();
            if ( data.at( dataSize - 1 ) == "lay" )
                doc->setLayer( data.at( dataSize - 2 ) ); //layer for ticks

            QList<QString> pData = ( passedData.split
                                     ( QRegularExpression ( "[\\n\\t\\r]" ) ) );

            QString entType = data.at( 0 ).toLower();
            if ( entType == "circle" ) //type
            {
                for ( int i = 0; i < pData.size(); i++ )
                {
                    test = pData.at( i ).simplified().toLower();

                    if ( test.startsWith( "ra" ) ) //radius
                        radius = ( test.split( ":" ).at( 1 ) ).toDouble();

                    else if ( test.startsWith( "ce" ) ) //center point
                    {
                        QList<QString> xy = ( ( test.split( ":" ) ).at( 1 ).split( " " ) );
                        centerX = ( (xy.at( 1 ).split( "=" ) ).at( 1 ) ).toDouble();
                        centerY = ( (xy.at( 2 ).split( "=" ) ).at( 1 ) ).toDouble();
                    }
                }

                //if data.at(5) is empty, startAngle defaults to zero
                double startAngle = ( data.at( 5 ).toDouble() * M_PI ) / 180.0; //radians
                tickLength = ( 2.0 * radius ) * ( data.at( 2 ).toDouble() / 100.0 );

                if ( data.at( dataSize - 3 ) == "i") //ticks inside/outside circle
                    tickLength *= -1;

                for ( int i = qty; i >= 1; --i )
                {
                    tickAngle = ( ( ( 2 * M_PI ) / qty ) * i ) + startAngle;

                    startX.setX( ( radius * cos( tickAngle ) ) + centerX );
                    startX.setY( ( radius * sin( tickAngle ) ) + centerY );

                    if ( ticks )
                    {
                        result = findLineEndPoint( startX.rx(), startX.ry(),
                                                   tickLength, tickAngle );
                        doc->addLine( &startX, &result );
                    }

//                    if ( breaks ) //to do
//                    {
//                        //qDebug() << "breaks on";
//                        result.setX(qreal(0));
//                        result.setY(qreal(0));
//                        doc->addLine( &startX, &result);
//                        //startX is point on entity
//                    }
                }
            } // end if CIRCLE

            //***********
            if ( entType == "line" ) //type
            {
                QPointF startXY ( 0.0, 0.0 );
                QPointF endXY ( 0.0, 0.0 );
                QList<QString> part ( { "" } );
                bool aboveLine = false;
                QString test = "";
                int size = data.at( 2 ).toInt();

                for ( int i = 0; i < pData.size(); i++ )
                {
                    test = pData.at( i ).simplified().toLower();
                    part = test.split( ":");

                    if ( test.startsWith( "fr" ) ) //from point
                    {
                        part = part.at( 1 ).split( " " );
                        startXY += QPointF( ((part.at( 1 ).split("=")).at( 1 )).toDouble(),
                                            ((part.at( 2 ).split("=")).at( 1 )).toDouble() );
                    }

                    else if ( test.startsWith( "to" ) ) //to point
                    {
                        part = part.at( 1 ).split( " " );
                        endXY += QPointF( ((part.at( 1 ).split("=")).at( 1 )).toDouble(),
                                          ((part.at( 2 ).split("=")).at( 1 )).toDouble() );
                    }

                    else if ( test.startsWith( "an" ) ) //angle
                        tickAngle = ( (part.at( 1 ).simplified().toDouble() + 90.0 )
                                      * M_PI ) / 180; //rads

                    else if ( test.startsWith( "le" ) ) //length
                        tickLength = ( test.split( ":" ).at( 1 )
                                       .toDouble() ) * size / 100; //%

                    else if ( test.startsWith("in") ) //inc
                    {
                        //if line goes left to right or right to left
                        //aboveLine = false;
                        part = part.at( 1 ).split( "=" );
                        if ( part.at( 1 ).startsWith( "-" ) ) //inc, minus 'x' pos
                            aboveLine = true;
                    }
                }

                if ( data.at( dataSize - 3 ) == "i") //ticks above/below linw
                    tickLength *= -1;

                qty += 1;
                for ( int i = 1; i < qty; i++ )
                {
                    startX.setX( ( ((endXY.rx() - startXY.rx()) / qty) * i )
                                 + startXY.rx() );
                    startX.setY( ( ((endXY.ry() - startXY.ry()) / qty) * i )
                                 + startXY.ry() );

                    if ( ticks )
                    {
                        if ( aboveLine )
                            //M_PI = 180° in rads, put ticks above line
                            if ( tickAngle >= M_PI ) tickAngle = tickAngle - M_PI;

                        result = findLineEndPoint( startX.rx(), startX.ry(),
                                                   tickLength, tickAngle ); //rads
                        doc->addLine( &startX, &result );
                    }

//                    if ( breaks ) //to do
//                    {
//                        qDebug() << "breaks on";
//                        result.setX(qreal(0));
//                        result.setY(qreal(0));
//                        doc->addLine( &startX, &result); //startX is point on entity
//                    }
                }
            } //end if LINE

            //***********
/*            if ( entType == "polyline" ) //type - to do
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

            //***********
            if ( entType == "arc" ) //type
            {
                double initial { 0.0 };
                double final { 0.0 };
                double arcLength { 0.0 };
                double res { 0.0 };
                QString copyTest = "";

                for ( int i = 0; i < pData.size(); i++ )
                {
                    test = pData.at( i ).simplified().toLower();

                    if ( test == "" ) continue; //not valid data for 'res = (...'

                    copyTest = test;
                    res = ( copyTest.split( ":" ).at( 1 ) ).toDouble();

                    if ( test.startsWith( "ce" ) ) //center
                    {
                        QList<QString> xy = ( ( test.split( ":" ) ).at( 1 ).split( " " ) );
                        centerX = ( ( xy.at( 1 ).split( "=" ) ).at( 1 ) ).toDouble();
                        centerY = ( ( xy.at( 2 ).split( "=" ) ).at( 1 ) ).toDouble();
                    }

                    else if ( test.startsWith( "ra" ) ) //radius
                        radius = res;

                    else if ( test.startsWith( "in" ) ) //initial angle
                        initial = res / 180.0 * M_PI; //degrees to rads

                    else if ( test.startsWith( "fi" ) ) //final angle
                        final = res / 180.0 * M_PI; //degrees to rads

                    else if ( test.startsWith( "le" ) ) //length
                        arcLength = res;
                }

                qty += 1;
                double diff { 0.0 };
                bool arcStart = false;

                if ( final > initial )
                    diff = ( initial - final ) / qty;
                else
                {
                    diff = ( arcLength / radius ) / qty;
                    arcStart = true;
                }

                tickLength = ( 2 * radius ) * ( data.at( 2 ).toDouble() / 100.0 ); //%
                if ( data.at( dataSize - 3 ) == "i") //ticks inside/outside arc
                    tickLength *= -1;

                for ( int i = 1; i < qty; i++ )
                {
                    tickAngle = ( diff * i );

                    //if ( arcStart )
                    //    tickAngle += initial;
                    //else
                    //    tickAngle += final;
                    tickAngle += arcStart ? initial : final; //Ternary Operator '?'

                    startX.setX( ( radius * cos( tickAngle ) ) + centerX );
                    startX.setY( ( radius * sin( tickAngle ) ) + centerY );

                    if ( ticks )
                    {
                        result = findLineEndPoint( startX.rx(), startX.ry(),
                                                   tickLength, tickAngle ); //rads
                        doc->addLine( &startX, &result );
                    }

//                    if ( breaks ) //to do
//                    {
//                        qDebug() << "breaks on";
//                        result.setX ( qreal(0) );
//                        result.setY ( qreal(0) );
//                        doc->addLine( &startX, &result ); //startX is point on entity
//                    }
                }
            } //end if ARC
            doc->setLayer( oldLayer );
        } //end if ( ticks || breaks )
        doc->updateView(); //updates & removes highlights
    }
//    else //rejected
//    {
//        qDebug() << "reject " << returnedData; //cancel button
//        strData.prepend( strEntity.arg
//                         (tr("MUST be a line, circle or arc")) );
//    }
}

double divide::findHypLength( double h1, double h2, double v1, double v2 )
{
    return ( hypot( h1 - h2, v1 - v2 ) ); //needs (C++11 or later) - hypotenuse
    //http://www.cplusplus.com/reference/cmath/hypot/ - C99
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

void divide::gotReturnedDataSlot( QString data )
{
    returnedData = data;
}

QString divide::getStrData( Plug_Entity *ent )
{
    if ( nullptr == ent )
        return QString( "%1\n" ).arg( tr( "Empty Entity" ) );

    QHash<int, QVariant> data;
    QString strData(""),
            strEntity( "%1\n" ),
            strCommon( "  %1: %2\n" ),
            strSpecific( "    %1: %2\n" ),
            strSpecificXY( QString("    %1: %2=%3 %4=%5\n")
                           .arg("%1",tr("X"), "%2",tr("Y"),"%3") );
    double numA {0.0};
    double numB {0.0};
    double numC {0.0};
    QPointF ptA, ptB, ptC;

    //common entity data
    ent->getData(&data);
    strData  = strCommon.arg(tr("Layer")).arg(data.value(DPI::LAYER).toString());
    int col = data.value(DPI::COLOR).toInt();
    strData.append( strCommon.arg(tr("Color")).arg( ent->intColor2str(col)) );
    strData.append( strCommon.arg(tr("Line type"))
                    .arg(data.value(DPI::LTYPE).toString()) );
    strData.append( strCommon.arg(tr("Line thickness"))
                    .arg(data.value(DPI::LWIDTH).toString()) );
    strData.append( strCommon.arg(tr("ID")).arg(data.value(DPI::EID).toLongLong()) );

    //specific entity data
    int et = data.value(DPI::ETYPE).toInt();
    switch ( et )
    {
    case DPI::LINE:
        strData.prepend( strEntity.arg(tr("LINE")));
        ptA.setX( data.value(DPI::STARTX).toDouble());
        ptA.setY( data.value(DPI::STARTY).toDouble());
        ptB.setX( data.value(DPI::ENDX).toDouble());
        ptB.setY( data.value(DPI::ENDY).toDouble());
        strData.append( strSpecificXY.arg(tr("from point")).
                        arg(d->realToStr(ptA.x())).
                        arg(d->realToStr(ptA.y())));
        strData.append( strSpecificXY.arg(tr("to point")).
                        arg(d->realToStr(ptB.x())).
                        arg(d->realToStr(ptB.y())));
        ptC = ptB - ptA;
        numA = sqrt( (ptC.x()*ptC.x()) + (ptC.y()*ptC.y()) );
        strData.append( strSpecific.arg(tr("length")).arg( d->realToStr(numA)));
        numB = asin(ptC.y() / numA);
        numC = numB*180/M_PI;
        if (ptC.x() < 0) numC = 180 - numC;
        if (numC < 0) numC = 360 + numC;
        strData.append( strSpecific.arg(tr("Angle in XY plane"))
                        .arg(d->realToStr(numC)));
        strData.append( strSpecificXY.arg(tr("Inc.")).
                        arg(d->realToStr(ptC.x())).
                        arg(d->realToStr(ptC.y())));
        break;
    case DPI::ARC:
        strData.prepend( strEntity.arg(tr("ARC")));
        strData.append( strSpecificXY.arg(tr("center point")).
                        arg(d->realToStr(data.value(DPI::STARTX).toDouble())).
                        arg(d->realToStr(data.value(DPI::STARTY).toDouble())));
        numA = data.value(DPI::RADIUS).toDouble();
        numB = data.value(DPI::STARTANGLE).toDouble();
        numC = data.value(DPI::ENDANGLE).toDouble();
        strData.append( strSpecific.arg(tr("radius")).arg(d->realToStr(numA)));
        strData.append( strSpecific.arg(tr("initial angle"))
                        .arg(d->realToStr(numB*180/M_PI)) );
        strData.append( strSpecific.arg(tr("final angle"))
                        .arg(d->realToStr(numC*180/M_PI)) );
        if( numB > numC) {
            numB -= 2.0 * M_PI;
        }
        strData.append( strSpecific.arg(tr("length"))
                        .arg( d->realToStr((numC-numB)*numA)) );
        break;
    case DPI::CIRCLE:
        strData.prepend( strEntity.arg(tr("CIRCLE")));
        strData.append( strSpecificXY.arg(tr("center point"))
                        .arg(d->realToStr(data.value(DPI::STARTX).toDouble()))
                        .arg(d->realToStr(data.value(DPI::STARTY).toDouble())));
        numA = data.value(DPI::RADIUS).toDouble();
        strData.append( strSpecific.arg(tr("radius")).arg(d->realToStr(numA)));
        strData.append( strSpecific.arg(tr("circumference"))
                        .arg(d->realToStr(numA*2*M_PI)));
        strData.append( strSpecific.arg(tr("area"))
                        .arg(d->realToStr(numA*numA*M_PI)));
        break;
    case DPI::POLYLINE: {
        strData.prepend( strEntity.arg(tr("POLYLINE")));
        strData.append( strSpecific.arg(tr("Closed"))
                        .arg( (0 == data.value(DPI::CLOSEPOLY).toInt())
                              ? tr("No") : tr("Yes")));
        strData.append( strSpecific.arg(tr("Vertices")).arg(""));
        QList<Plug_VertexData> vl;
        ent->getPolylineData(&vl);
        int iVertices = vl.size();
        for (int i = 0; i < iVertices; ++i) {
            strData.append( strSpecificXY.arg(tr("in point")).
                            arg(d->realToStr(vl.at( i ).point.x()) ).
                            arg(d->realToStr(vl.at( i ).point.y())) );
            //***
            if ( 0.0 != vl.at( i ).bulge) { //was 0
            //***
                strData.append( strSpecific.arg( tr("radius") )
                                .arg(d->realToStr
                                     (polylineRadius
                                      (vl.at( i ), vl.at(( i + 1 ) % iVertices)))) );
            }
        }
        break;
    }
    default:
        strData.prepend( strEntity.arg
                         ( tr( "MUST be a line, circle or arc" ) ) );
        break;
    }
    return strData;
}

//return center of active windoow
QPoint divide::findWindowCentre()
{
    QPoint centXY;

    centXY.setX( ( QApplication::activeWindow()->width() / 2 )
                 + QApplication::activeWindow()->x() );
    centXY.setY( ( QApplication::activeWindow()->height() / 2 )
                 + QApplication::activeWindow()->y() );

    return (centXY);
}

double divide::polylineRadius( const Plug_VertexData& ptA,
                                      const Plug_VertexData& ptB )
{
    double dChord = sqrt( pow(ptA.point.x() - ptB.point.x(), 2) +
                          pow(ptA.point.y() - ptB.point.y(), 2) );

    return fabs( 0.5 * dChord / sin( 2.0 * atan( ptA.bulge ) ) );
}
