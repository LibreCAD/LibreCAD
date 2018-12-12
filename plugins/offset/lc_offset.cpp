#include "lc_offset.h"
using namespace ClipperLib;

QString LC_Offset::name() const
{
    return QString(tr("offset plugin"));
}


PluginCapabilities LC_Offset::getCapabilities() const
{
    PluginCapabilities cap;
    cap.menuEntryPoints.append(PluginMenuLocation("plugins_menu",tr("offset plugin")));

    return cap;
}


void LC_Offset::execComm(Document_Interface *doc, QWidget *parent, QString cmd)
{
    Q_UNUSED(cmd);
    //Q_UNUSED(doc);
    //Q_UNUSED(parent);

    //QHash<int,QVariant*> data;
    QList<Plug_Entity* > e_lt;
    bool yes = doc->getSelect(&e_lt);

    if(e_lt.empty() || !yes) return;


    bool flag = true;
    vector<QPointF> points;

    for(auto it=e_lt.begin();it!=e_lt.end();it++)
    {
        auto entity = *it;
        QHash<int,QVariant> data;
        entity->getData(&data);
        if(data.value(DPI::ETYPE) != DPI::LINE)
        {
            flag = false;
            break;
        }

        double x1 = data.value(DPI::STARTX).toDouble();
        double y1 = data.value(DPI::STARTY).toDouble();

        double x2 = data.value(DPI::ENDX).toDouble();
        double y2 = data.value(DPI::ENDY).toDouble();

        QPointF point1(x1,y1);
        QPointF point2(x2,y2);


        if(points.empty())
        {
            points.emplace_back(point1);
            points.emplace_back(point2);
        }
        else{
            if(points.back() != point1)
                points.emplace_back(point1);

            points.emplace_back(point2);
        }

    }

    if(flag)
    {

        LC_OffsetDlg dlg(parent);
        dlg.exec();

        double delta = dlg.getData();

        vector<QPointF> points1 = getOffset(points,delta);
        doc->addLines(points1,true);
    }
    else{
        QMessageBox::information(parent,"Error","The graphic selected is not a polygon!");
    }

}


vector<QPointF> LC_Offset::getOffset(const vector<QPointF>& vec,double delta)
{
    vector<QPointF> vec1;
    Path subj;
    Paths solution;
    int off_val = delta;

    if(!vec.empty())
    {
        for(QPointF point : vec)
        {
            subj << IntPoint(point.x(),point.y());
        }

        ClipperOffset co;
        co.AddPath(subj,jtRound,etClosedPolygon);
        co.Execute(solution,off_val);
    }

    for(Path path : solution)
        for(IntPoint point : path)
        {
            vec1.emplace_back(QPointF(point.X,point.Y));
        }

    return vec1;

}
