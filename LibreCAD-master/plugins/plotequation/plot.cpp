//This plugin allows the user to plot mathematical equations.
//It uses muParser for parsing the mathematical equations.
//
//ToDo: *set max and min value for step size?



#include "document_interface.h"
#include "plot.h"
#include "plotdialog.h"
#include <muParser.h>
#include <QDebug>

plot::plot(QObject *parent) :
    QObject(parent)
{
}

QString plot::name() const
 {
     return (tr("Plot plugin"));
 }

PluginCapabilities plot::getCapabilities() const
{
    PluginCapabilities pluginCapabilities;
    pluginCapabilities.menuEntryPoints
            << PluginMenuLocation("plugins_menu", tr("Plot plugin"));
    return pluginCapabilities;
}

void plot::execComm(Document_Interface *doc, QWidget *parent, QString cmd)
{
    Q_UNUSED(doc);
    Q_UNUSED(cmd);

    QString equation1;
    QString equation2;
    QString startValue;
    QString endValue;
    double stepSize;

    QList<double> xValues;
    QList<double> yValues1;
    QList<double> yValues2;
    plotDialog::EntityType lineType=plotDialog::Polyline;

    plotDialog plotDlg(parent);
    int result =  plotDlg.exec();
    if (result == QDialog::Accepted)
    {
        double equationVariable = 0.0;
        double startVal = 0.0;
        double endVal = 0.0;
        plotDlg.getValues(equation1, equation2, startValue, endValue, stepSize);
        lineType=plotDlg.getEntityType();

        try{
            mu::Parser p;
            p.DefineConst("pi",M_PI);
            p.DefineConst("e",M_E);
            p.DefineVar("x", &equationVariable);
            p.DefineVar("t", &equationVariable);
            p.SetExpr(startValue.toStdString());
            startVal = p.Eval();

            p.SetExpr(endValue.toStdString());
            endVal = p.Eval();

            p.SetExpr(equation1.toStdString());

            for(equationVariable = startVal; equationVariable <= endVal; equationVariable += stepSize)
            {//calculate the values of the first equation
                xValues.append(equationVariable);
                yValues1.append(p.Eval());
            }

            if(!equation2.isEmpty())
            {//calculate the values of the second equation
                p.SetExpr(equation2.toStdString());
                for(int i = 0; i < xValues.size(); ++i)
                {
                    equationVariable = xValues.at(i);
                    yValues2.append(p.Eval());
                }
            }
        }
        catch (mu::Parser::exception_type &e)
        {
            std::cout << e.GetMsg() << std::endl;
        }

        QList<double> const& xpoints=(equation2.isEmpty())?xValues:yValues1;
        QList<double> const& ypoints=(equation2.isEmpty())?yValues1:yValues2;

        if (lineType == plotDialog::LineSegments || lineType == plotDialog::SplinePoints){
            std::vector<QPointF> points;
            for(int i=0; i< xpoints.size(); ++i){
                points.emplace_back(QPointF(xpoints[i], ypoints[i]));
            }
            if (lineType == plotDialog::SplinePoints){
                //TODO add option for splinepoints: closed
                //hardcoded to false now
                doc->addSplinePoints(points, false);
            } else
                doc->addLines(points, false);
        } else { //default plotDialog::Polyline
            std::vector<Plug_VertexData> points;
            for(int i=0; i< xpoints.size(); ++i){
                points.emplace_back(Plug_VertexData(QPointF(xpoints[i], ypoints[i]), 0.0));
            }
            doc->addPolyline(points, false);
        }

    }

}
