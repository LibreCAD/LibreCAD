//This plugin allows the user to plot mathematical equations.
//It uses muParser for parsing the mathematical equations.
//
//ToDo: *set max and min value for step size?



#include "document_interface.h"
#include "plot.h"
#include "plotdialog.h"
#include <muParser.h>

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
            << PluginMenuLocation(tr("Draw"), tr("Plot plugin"));
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
    QList<QPointF> points;
    QPointF startPoint;
    QPointF endPoint;
    unsigned int pointAmount;

    plotDialog plotDlg(parent);
    int result =  plotDlg.exec();
    if (result == QDialog::Accepted)
    {
        double equationVariable = 0.0;
        double startVal = 0.0;
        double endVal = 0.0;
        plotDlg.getValues(equation1, equation2, startValue, endValue, stepSize);

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

            for(equationVariable = startVal; equationVariable < endVal; equationVariable += stepSize)//end value is not used!
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

        pointAmount = xValues.size();

        if(equation2.isEmpty())
        {//use first equation for drawing (explicit)
            for(size_t i = 0; i < pointAmount -1; ++i)
            {
                startPoint.setX(xValues.at(i));
                startPoint.setY(yValues1.at(i));

                endPoint.setX(xValues.at(i+1));
                endPoint.setY(yValues1.at(i+1));
                doc->addLine(&startPoint, &endPoint);
            }

        }
        else
        {//use first and second equation for drawing (parametric)
            for(size_t i = 0; i < pointAmount -1; ++i)
            {
                startPoint.setX(yValues1.at(i));
                startPoint.setY(yValues2.at(i));

                endPoint.setX(yValues1.at(i+1));
                endPoint.setY(yValues2.at(i+1));
                doc->addLine(&startPoint, &endPoint);
            }
        }
    }

}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(plotequation, plot);
#endif
