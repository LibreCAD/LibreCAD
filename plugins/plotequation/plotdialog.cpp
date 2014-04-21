#include "plotdialog.h"

plotDialog::plotDialog(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(tr("Plot equation"));
    mainLayout = new QGridLayout;
    buttonLayout = new QHBoxLayout;
    description = new QLabel(tr("This plugin allows you to plot mathematical equations.\n"
                                "If you don't want to use the parametric form, just leave out \"Equation2\".\n"
                                "You can use pi when you need the value of pi (i.e. (3*pi)).\n"
                                "Use t or x in your equation as a variable/parameter.\n"));
    lblEquasion1 = new QLabel(tr("Equation 1:"));
    lblEquasion2 = new QLabel(tr("Equation 2:"));
    lnedEquasion1 = new QLineEdit;
    lnedEquasion2 = new QLineEdit;
    lblStartValue = new QLabel(tr("start value:"));
    lblEndValue = new QLabel(tr("end value:"));
    lblStepSize = new QLabel(tr("step size:"));
    lnedStartValue = new QLineEdit;
    lnedEndValue = new QLineEdit;
    lnedStepSize = new QLineEdit;
    btnAccept = new QPushButton(tr("Draw"));
    btnCancel = new QPushButton(tr("Cancel"));
    space = new QSpacerItem(0, 20);


    lnedStartValue->setMaximumWidth(50);
    lnedEndValue->setMaximumWidth(50);
    lnedStepSize->setMaximumWidth(50);

    mainLayout->addWidget(description, 0, 0, 1, -1);

    mainLayout->addItem(space, 1, 0);

    mainLayout->addWidget(lblEquasion1, 2, 0);
    mainLayout->addWidget(lnedEquasion1, 2, 1);

    mainLayout->addWidget(lblEquasion2, 3, 0);
    mainLayout->addWidget(lnedEquasion2, 3, 1);

    mainLayout->addWidget(lblStartValue, 4, 0);
    mainLayout->addWidget(lblEndValue, 5, 0);
    mainLayout->addWidget(lblStepSize, 6, 0);

    mainLayout->addWidget(lnedStartValue, 4, 1);
    mainLayout->addWidget(lnedEndValue, 5, 1);
    mainLayout->addWidget(lnedStepSize, 6, 1);

    buttonLayout->addWidget(btnAccept);
    buttonLayout->addWidget(btnCancel);

    mainLayout->addLayout(buttonLayout, 7, 1);

    setLayout(mainLayout);

    connect(btnAccept, SIGNAL(clicked()), this, SLOT(slotDrawButtonClicked()));
    connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));


}

//get the valuew that the user entered
void plotDialog::getValues(QString& eq1, QString& eq2, QString& start, QString& end, double& step) const
{
    eq1 = equation1;
    eq2 = equation2;
    start = startValue;
    end = endValue;
    step = stepSize;
}

//accept the dialog when input is ok otherwise reject
void plotDialog::slotDrawButtonClicked()
{
    if(readInput())
    {
        accept();
    }
    else
    {
        done(QDialog::Rejected);
    }
}

//read the input from the user (equation1, equation2, starvalue, endvalue, stepsize)
bool plotDialog::readInput()
{
    bool conv;

    //get equations
    equation1 = lnedEquasion1->text();
    equation2 = lnedEquasion2->text();

    if(equation1.isEmpty())
    {
        qDebug("no equation1 given");
        return false;
    }
    else
    {
        equation1 = equation1.replace(" ", "");
    }

    if(equation2.isEmpty())
    {
        qDebug( "no equation2 given");
    }
    else
    {
        equation2 = equation2.replace(" ", "");
    }

    //get start/end value
    startValue = lnedStartValue->text();
    if(startValue.isEmpty())
    {
        qDebug("no start point given");
        return false;
    }

    endValue = lnedEndValue->text();
    if(endValue.isEmpty())
    {
        qDebug("no end point given");
        return false;
    }

    //get step size
    stepSize = lnedStepSize->text().toDouble(&conv);
    if(!conv)
    {
        qDebug("could not convert step size");
        return false;
    }

    return true;
}
