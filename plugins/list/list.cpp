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

PluginCapabilities LC_List::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("Info", tr("List entities"));
    return pluginCapabilities;
}

void LC_List::execComm(Document_Interface *doc,
                             QWidget *parent, QString cmd)
{
    Q_UNUSED(parent);
    Q_UNUSED(cmd);
    d = doc;
    QList<Plug_Entity *> obj;
    bool yes  = doc->getSelect(&obj);
    if (!yes || obj.isEmpty()) return;

    QString text;
    for (int i = 0; i < obj.size(); ++i) {
        QString strdata = getStrData(obj.at(i));
        text.append(QString(tr("n %1: \n")).arg(i+1));
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
        return QString(tr("Empty Entity\n\n"));
    ent->getData(&data);
    str = tr("Layer: ") + data.value(DPI::LAYER).toString();
    QColor color = data.value(DPI::COLOR).value<QColor>();
    str.append(tr("\n Color: ") + color.name());
    str.append(tr(" Line type: ") + data.value(DPI::LTYPE).toString());
    str.append(tr("\n Line thickness: ") + data.value(DPI::LWIDTH).toString());
    str.append( QString(tr("\n ID: %1\n")).arg(data.value(DPI::EID).toLongLong()));
    int et = data.value(DPI::ETYPE).toInt();

    //specific entity data
    switch (et) {
    case DPI::POINT:
        str.append( QString(tr("     in point: X=%1 Y=%2\n\n")).arg(
                d->realToStr(data.value(DPI::STARTX).toDouble()) ).arg(
                d->realToStr(data.value(DPI::STARTY).toDouble()) ) );
        return QString(tr("POINT: ")).append(str);
        break;
    case DPI::LINE:
        ptA.setX( data.value(DPI::STARTX).toDouble() );
        ptA.setY( data.value(DPI::STARTY).toDouble() );
        ptB.setX( data.value(DPI::ENDX).toDouble() );
        ptB.setY( data.value(DPI::ENDY).toDouble() );
        str.append( QString(tr("     from point: X=%1 Y=%2\n     to point: X=%3 Y=%4\n")).arg(
                    d->realToStr(ptA.x()) ).arg( d->realToStr(ptA.y()) ).arg(
                    d->realToStr(ptB.x()) ).arg( d->realToStr(ptB.y()) ) );
        ptC = ptB - ptA;
        numA = sqrt( (ptC.x()*ptC.x())+ (ptC.y()*ptC.y()));
        str.append( QString(tr("   length: %1,")).arg( d->realToStr(numA) ));
        numB = asin(ptC.y() / numA);
        numC = numB*180/M_PI;
        if (ptC.x() < 0) numC = 180 - numC;
        if (numC < 0) numC = 360 + numC;
        str.append( QString(tr("  Angle in XY plane: %1\n")).arg(d->realToStr(numC)) );
        str.append( QString(tr("  Inc. X = %1,  Inc. Y = %2\n\n")).arg(
                            d->realToStr(ptC.x()) ).arg( d->realToStr(ptC.y()) ));
         return QString(tr("LINE: ")).append(str);
       break;
    case DPI::ARC:
        str.append( QString(tr("   certer point: X=%1 Y=%2\n")).arg(
                d->realToStr(data.value(DPI::STARTX).toDouble()) ).arg(
                d->realToStr(data.value(DPI::STARTY).toDouble()) ) );
        numA = data.value(DPI::RADIUS).toDouble();
        numB = data.value(DPI::STARTANGLE).toDouble();
        numC = data.value(DPI::ENDANGLE).toDouble();
        str.append( QString(tr("   radius: %1\n")).arg(d->realToStr(numA)) );
        str.append( QString(tr("   initial angle: %1\n")).arg(d->realToStr(numB*180/M_PI)) );
        str.append( QString(tr("   final angle: %1\n")).arg(d->realToStr(numC*180/M_PI)) );
        str.append( QString(tr("   length: %1\n")).arg( d->realToStr((numC-numB)*numA) ) );
        return QString(tr("ARC: ")).append(str);
        break;
    case DPI::CIRCLE:
        str.append( QString(tr("   certer point: X=%1 Y=%2\n")).arg(
                d->realToStr(data.value(DPI::STARTX).toDouble()) ).arg(
                d->realToStr(data.value(DPI::STARTY).toDouble()) ) );
        numA = data.value(DPI::RADIUS).toDouble();
        str.append( QString(tr("   radius: %1\n")).arg(d->realToStr(numA)) );
        str.append( QString(tr("   circumference: %1\n")).arg(
                d->realToStr(numA*2*M_PI) ) );
        str.append( QString(tr("   area: %1\n\n")).arg(
                d->realToStr(numA*numA*M_PI) ) );
        return QString(tr("CIRCLE: ")).append(str);
        break;
    case DPI::ELLIPSE://toy aqui
        str.append( QString(tr("   certer point: X=%1 Y=%2\n")).arg(
                d->realToStr(data.value(DPI::STARTX).toDouble()) ).arg(
                d->realToStr(data.value(DPI::STARTY).toDouble()) ) );
        str.append( QString(("   major axis: X=%1 Y=%2\n")).arg(
                d->realToStr(data.value(DPI::ENDX).toDouble()) ).arg(
                d->realToStr(data.value(DPI::ENDY).toDouble()) ) );
/*        str.append( QString(tr("   minor axis: X=%1 Y=%2\n")).arg(
                data.value(DPI::ENDX).toDouble()).arg(
                data.value(DPI::ENDY).toDouble() ) );
        str.append( QString(tr("   start point: X=%1 Y=%2\n")).arg(
                data.value(DPI::ENDX).toDouble()).arg(
                data.value(DPI::ENDY).toDouble() ) );
        str.append( QString(tr("   end point: X=%1 Y=%2\n")).arg(
                data.value(DPI::ENDX).toDouble()).arg(
                data.value(DPI::ENDY).toDouble() ) );
        str.append( QString(tr("   initial angle: %1\n")).arg(numB*180/M_PI) );
        str.append( QString(tr("   final angle: %1\n")).arg(numC*180/M_PI) );
        str.append( QString(tr("   radius ratio: %1\n")).arg(numC*180/M_PI) );*/
        return QString(tr("ELLIPSE: ")).append(str);
        break;

    case DPI::CONSTRUCTIONLINE:
        return QString(tr("CONSTRUCTIONLINE: ")).append(str);
        break;
    case DPI::OVERLAYBOX:
        return QString(tr("OVERLAYBOX: ")).append(str);
        break;
    case DPI::SOLID:
        return QString(tr("SOLID: ")).append(str);
        break;
//container entities
    case DPI::MTEXT:
        return QString(tr("MTEXT: ")).append(str);
        break;
    case DPI::TEXT:
        return QString(tr("TEXT: ")).append(str);
        break;
    case DPI::INSERT:
        ptA.setX( data.value(DPI::STARTX).toDouble() );
        ptA.setY( data.value(DPI::STARTY).toDouble() );
        str.append( QString(tr("   Name: %1\n")).arg( data.value(DPI::BLKNAME).toString()) );
        str.append( QString(tr("   Insertion point: X=%1 Y=%2\n")).arg(
                d->realToStr(ptA.x()) ).arg( d->realToStr(ptA.y()) ) );
        return QString(tr("INSERT: ")).append(str);
        break;
    case DPI::POLYLINE: {
        if (data.value(DPI::CLOSEPOLY).toInt() == 0 )
            str.append( QString(tr("     Opened\n")) );
        else
            str.append( QString(tr("     Closed\n")) );
        str.append( QString(tr("     Vertex:\n")));
        QList<Plug_VertexData> vl;
        ent->getPolylineData(&vl);
        for (int i = 0; i < vl.size(); ++i) {
            str.append( QString(tr("     in point: X=%1 Y=%2\n")).arg(
                           d->realToStr(vl.at(i).point.x()) ).arg( d->realToStr(vl.at(i).point.y()) ) );
            if (vl.at(i).bulge != 0)
            str.append( QString(tr("     curvature: %1\n")).arg( d->realToStr(vl.at(i).bulge)) );
        }
        return QString(tr("POLYLINE: ")).append(str);
        break; }
    case DPI::IMAGE:
        return QString(tr("IMAGE: ")).append(str);
        break;
    case DPI::SPLINE:
        return QString(tr("SPLINE: ")).append(str);
        break;
    case DPI::HATCH:
        return QString(tr("HATCH: ")).append(str);
        break;
    case DPI::DIMLEADER:
        return QString(tr("DIMLEADER: ")).append(str);
        break;
    case DPI::DIMALIGNED:
        return QString(tr("DIMALIGNED: ")).append(str);
        break;
    case DPI::DIMLINEAR:
        return QString(tr("DIMLINEAR: ")).append(str);
        break;
    case DPI::DIMRADIAL:
        return QString(tr("DIMRADIAL: ")).append(str);
        break;
    case DPI::DIMDIAMETRIC:
        return QString(tr("DIMDIAMETRIC: ")).append(str);
        break;
    case DPI::DIMANGULAR:
        return QString(tr("DIMANGULAR: ")).append(str);
        break;
    default:
        break;
    }
        return QString(tr("UNKNOWN: ")).append(str);
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

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(lc_list, LC_List);
#endif
