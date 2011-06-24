/*****************************************************************************/
/*  list.cpp - List selected entities                                        */
/*                                                                           */
/*  Copyright (C) 2011 Rallaz, rallazz@gmail.com                             */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 2 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/


#include <QTextEdit>
#include <QColor>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <math.h>
#include "document_interface.h"
#include "list.h"

QString LC_List::name() const
 {
     return (tr("List entities"));
 }

QList<PluginMenuLocation> LC_List::menu() const
 {
    return QList<PluginMenuLocation>() << PluginMenuLocation("Info", tr("List entities"));
 }

void LC_List::execComm(Document_Interface *doc,
                             QWidget *parent)
{
    Q_UNUSED(parent);
    QList<Plug_Entity *> obj;
    bool yes  = doc->getSelect(&obj);
    if (!yes || obj.isEmpty()) return;

    QString text;
    for (int i = 0; i < obj.size(); ++i) {
        QString strdata = getStrData(obj.at(i));
        text.append(QString("n %1: \n").arg(i+1));
        text.append(strdata);
    }
    lc_Listdlg dlg(parent);
    dlg.setText(text);
    dlg.exec();

    while (!obj.isEmpty())
        delete obj.takeFirst();
}

QString LC_List::getStrData(Plug_Entity *ent) {
    QHash<int, QVariant> data;
    QString str;
    double numA, numB, numC;
    QPointF ptA, ptB, ptC;
    //common entity data
    if (ent == 0)
        return QString("Empty Entity\n\n");
    ent->getData(&data);
    str = "Layer: " + data.value(DPI::LAYER).toString();
    QColor color = data.value(DPI::COLOR).value<QColor>();
    str.append("\n Color: " + color.name());
    str.append(" Line type: " + data.value(DPI::LTYPE).toString());
    str.append( "\n Line thickness: " + data.value(DPI::LWIDTH).toString());
    str.append( QString("\n ID: %1\n").arg(data.value(DPI::EID).toLongLong()));
    int et = data.value(DPI::ETYPE).toInt();

    //specific entity data
    switch (et) {
    case DPI::POINT:
        str.append( QString("     in point: X=%1 Y=%2\n\n").arg(
                data.value(DPI::STARTX).toDouble()).arg(
                data.value(DPI::STARTY).toDouble() ) );
        return QString("POINT: ").append(str);
        break;
    case DPI::LINE:
        ptA.setX( data.value(DPI::STARTX).toDouble() );
        ptA.setY( data.value(DPI::STARTY).toDouble() );
        ptB.setX( data.value(DPI::ENDX).toDouble() );
        ptB.setY( data.value(DPI::ENDY).toDouble() );
        str.append( QString("     from point: X=%1 Y=%2\n     to point: X=%3 Y=%4\n").arg(
                ptA.x()).arg(ptA.y()).arg(ptB.x()).arg(ptB.y()) );
        ptC = ptB - ptA;
        numA = sqrt( (ptC.x()*ptC.x())+ (ptC.y()*ptC.y()));
        str.append( QString("   length: %1,").arg( numA ));
        numB = asin(ptC.y() / numA);
        numC = numB*180/M_PI;
        if (ptC.x() < 0) numC = 180 - numC;
        if (numC < 0) numC = 360 + numC;
        str.append( QString("  Angle in XY plane: %1\n").arg(numC) );
        str.append( QString("  Inc. X = %1,  Inc. Y = %2\n\n").arg(
                            ptC.x() ).arg(ptC.y()));
         return QString("LINE: ").append(str);
       break;
    case DPI::ARC:
        str.append( QString("   certer point: X=%1 Y=%2\n").arg(
                data.value(DPI::STARTX).toDouble()).arg(
                data.value(DPI::STARTY).toDouble() ) );
        numA = data.value(DPI::RADIUS).toDouble();
        numB = data.value(DPI::STARTANGLE).toDouble();
        numC = data.value(DPI::ENDANGLE).toDouble();
        str.append( QString("   radius: %1\n").arg(numA) );
        str.append( QString("   initial angle: %1\n").arg(numB*180/M_PI) );
        str.append( QString("   final angle: %1\n").arg(numC*180/M_PI) );
        str.append( QString("   length: %1\n").arg( (numC-numB)*numA) );
        return QString("ARC: ").append(str);
        break;
    case DPI::CIRCLE:
        str.append( QString("   certer point: X=%1 Y=%2\n").arg(
                data.value(DPI::STARTX).toDouble()).arg(
                data.value(DPI::STARTY).toDouble() ) );
        numA = data.value(DPI::RADIUS).toDouble();
        str.append( QString("   radius: %1\n").arg(numA) );
        str.append( QString("   circumference: %1\n").arg(
                numA*2*M_PI ) );
        str.append( QString("   area: %1\n\n").arg(
                numA*numA*M_PI ) );
        return QString("CIRCLE: ").append(str);
        break;
    case DPI::ELLIPSE://toy aqui
        str.append( QString("   certer point: X=%1 Y=%2\n").arg(
                data.value(DPI::STARTX).toDouble()).arg(
                data.value(DPI::STARTY).toDouble() ) );
        str.append( QString("   major axis: X=%1 Y=%2\n").arg(
                data.value(DPI::ENDX).toDouble()).arg(
                data.value(DPI::ENDY).toDouble() ) );
/*        str.append( QString("   minor axis: X=%1 Y=%2\n").arg(
                data.value(DPI::ENDX).toDouble()).arg(
                data.value(DPI::ENDY).toDouble() ) );
        str.append( QString("   start point: X=%1 Y=%2\n").arg(
                data.value(DPI::ENDX).toDouble()).arg(
                data.value(DPI::ENDY).toDouble() ) );
        str.append( QString("   end point: X=%1 Y=%2\n").arg(
                data.value(DPI::ENDX).toDouble()).arg(
                data.value(DPI::ENDY).toDouble() ) );
        str.append( QString("   initial angle: %1\n").arg(numB*180/M_PI) );
        str.append( QString("   final angle: %1\n").arg(numC*180/M_PI) );
        str.append( QString("   radius ratio: %1\n").arg(numC*180/M_PI) );*/
        return QString("ELLIPSE: ").append(str);
        break;

    case DPI::CONSTRUCTIONLINE:
        return QString("CONSTRUCTIONLINE: ").append(str);
        break;
    case DPI::OVERLAYBOX:
        return QString("OVERLAYBOX: ").append(str);
        break;
    case DPI::SOLID:
        return QString("SOLID: ").append(str);
        break;
//container entities
    case DPI::TEXT:
        return QString("TEXT: ").append(str);
        break;
    case DPI::INSERT:
        return QString("INSERT: ").append(str);
        break;
    case DPI::POLYLINE:
        return QString("POLYLINE: ").append(str);
        break;
    case DPI::IMAGE:
        return QString("IMAGE: ").append(str);
        break;
    case DPI::SPLINE:
        return QString("SPLINE: ").append(str);
        break;
    case DPI::HATCH:
        return QString("HATCH: ").append(str);
        break;
    case DPI::DIMLEADER:
        return QString("DIMLEADER: ").append(str);
        break;
    case DPI::DIMALIGNED:
        return QString("DIMALIGNED: ").append(str);
        break;
    case DPI::DIMLINEAR:
        return QString("DIMLINEAR: ").append(str);
        break;
    case DPI::DIMRADIAL:
        return QString("DIMRADIAL: ").append(str);
        break;
    case DPI::DIMDIAMETRIC:
        return QString("DIMDIAMETRIC: ").append(str);
        break;
    case DPI::DIMANGULAR:
        return QString("DIMANGULAR: ").append(str);
        break;
    default:
        break;
    }
        return QString("UNKNOWN: ").append(str);
}

/*****************************/
lc_Listdlg::lc_Listdlg(QWidget *parent) :  QDialog(parent)
{
    setWindowTitle(tr("List entities"));
//    QTextEdit *edit= new QTextEdit(this);
    edit.setReadOnly (true);
    edit.setAcceptRichText ( false );
    QDialogButtonBox* bb = new QDialogButtonBox( QDialogButtonBox::Close, Qt::Horizontal, this );
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(&edit);
    mainLayout->addWidget(bb);
    this->setLayout(mainLayout);
    this->resize ( 450, 350 );

    connect(bb, SIGNAL(rejected()), this, SLOT(accept()));
}

void lc_Listdlg::setText(QString text)
{
    edit.setText(text);
}
lc_Listdlg::~lc_Listdlg()
{
}

Q_EXPORT_PLUGIN2(lc_list, LC_List);
