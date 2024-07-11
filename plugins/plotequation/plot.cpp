//This plugin allows the user to plot mathematical equations.
//It uses muParser for parsing the mathematical equations.
//
//ToDo: *set max and min value for step size?



#include "document_interface.h"
#include "plot.h"
#include "plotdialog.h"
#include <muParser.h>
#include <QDebug>

mu::string_type toMUPString(const QString &str)
{
#if defined(_UNICODE)
  return str.toStdWString();
#else
  return str.toStdString();
#endif
}

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

void plot::execComm([[maybe_unused]] Document_Interface *doc, QWidget *parent, [[maybe_unused]] QString cmd)
{
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
            p.DefineConst(_T("pi"),M_PI);
            p.DefineConst(_T("e"),M_E);
            p.DefineVar(_T("x"), &equationVariable);
            p.DefineVar(_T("t"), &equationVariable);
            p.SetExpr(toMUPString(startValue));
            startVal = p.Eval();

            p.SetExpr(toMUPString(endValue));
            endVal = p.Eval();

            p.SetExpr(toMUPString(equation1));

            for(equationVariable = startVal; equationVariable <= endVal; equationVariable += stepSize)
            {//calculate the values of the first equation
                xValues.append(equationVariable);
                yValues1.append(p.Eval());
            }

            if(!equation2.isEmpty())
            {//calculate the values of the second equation
                p.SetExpr(toMUPString(equation2));
                for(int i = 0; i < xValues.size(); ++i)
                {
                    equationVariable = xValues.at(i);
                    yValues2.append(p.Eval());
                }
            }
        }
        catch (mu::Parser::exception_type &e)
        {
            mu::console() << e.GetMsg() << std::endl;
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
