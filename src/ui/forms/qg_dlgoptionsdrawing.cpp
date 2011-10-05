/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#include "qg_dlgoptionsdrawing.h"

#include <qmessagebox.h>
#include "rs_graphic.h"
#include "rs_filterdxf.h"
//#include "rs_units.h"

/*
 *  Constructs a QG_DlgOptionsDrawing as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgOptionsDrawing::QG_DlgOptionsDrawing(QWidget* parent, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setModal(modal);
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgOptionsDrawing::~QG_DlgOptionsDrawing()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgOptionsDrawing::languageChange()
{
    retranslateUi(this);
}
void QG_DlgOptionsDrawing::init() {
    graphic = NULL;

    // precision list:
    QString s;
    for (int i=0; i<=8; i++) {
        s = QString("%1").arg(0.0,0,'f', i);
        listPrec1 << s;
    }

    // Main drawing unit:
    for (int i=RS2::None; i<RS2::LastUnit; i++) {
        cbUnit->addItem(RS_Units::unitToString((RS2::Unit)i));
    }

    // init units combobox:
    QStringList unitList;
    unitList << tr("Scientific")
    << tr("Decimal")
    << tr("Engineering")
    << tr("Architectural")
    << tr("Fractional");
    cbLengthFormat->insertItems(0, unitList);

    // init angle units combobox:
    QStringList aunitList;
    aunitList << tr("Decimal Degrees")
    << tr("Deg/min/sec")
    << tr("Gradians")
    << tr("Radians")
    << tr("Surveyor's units");
    cbAngleFormat->insertItems(0, aunitList);

    // Paper format:
    for (int i=RS2::Custom; i<=RS2::NPageSize; i++) {
        cbPaperFormat->addItem(RS_Units::paperFormatToString((RS2::PaperFormat)i));
    }

    // Encodings:
    /*
    QStringList encodingList;
    encodingList << "Latin1"
    << "Big5"
    << "Big5-HKSCS"
    << "eucJP"
    << "eucKR"
    << "GB2312"
    << "GBK"
    << "GB18030"
    << "JIS7"
    << "Shift-JIS"
    << "TSCII"
    << "utf88-bit "
    << "utf16"
    << "KOI8-R"
    << "KOI8-U"
    << "ISO8859-1"
    << "ISO8859-2"
    << "ISO8859-3"
    << "ISO8859-4"
    << "ISO8859-5"
    << "ISO8859-6"
    << "ISO8859-7"
    << "ISO8859-8"
    << "ISO8859-8-i"
    << "ISO8859-9"
    << "ISO8859-10 "
    << "ISO8859-13 "
    << "ISO8859-14 "
    << "ISO8859-15"
    << "IBM 850 "
    << "IBM 866 "
    << "CP874 "
    << "CP1250"
    << "CP1251"
    << "CP1252"
    << "CP1253"
    << "CP1254"
    << "CP1255"
    << "CP1256"
    << "CP1257"
    << "CP1258 "
    << "Apple Roman "
    << "TIS-620";
    cbEncoding->insertItems(0, encodingList);
    */
}


/**
 * Sets the graphic and updates the GUI to match the drawing.
 */
void QG_DlgOptionsDrawing::setGraphic(RS_Graphic* g) {
    graphic = g;

    if (graphic==NULL) {
        return;
    }

    // main drawing unit:
    int insunits = graphic->getVariableInt("$INSUNITS", 0);
    cbUnit->setCurrentIndex( cbUnit->findText(
            RS_Units::unitToString(RS_FilterDXF::numberToUnit(insunits))));

    // units / length format:
    int lunits = graphic->getVariableInt("$LUNITS", 2);
    cbLengthFormat->setCurrentIndex(lunits-1);

    // units length precision:
    int luprec = graphic->getVariableInt("$LUPREC", 4);
    updateLengthPrecision();
    cbLengthPrecision->setCurrentIndex(luprec);

    // units / angle format:
    int aunits = graphic->getVariableInt("$AUNITS", 0);
    cbAngleFormat->setCurrentIndex(aunits);

    // units angle precision:
    int auprec = graphic->getVariableInt("$AUPREC", 2);
    updateAnglePrecision();
    cbAnglePrecision->setCurrentIndex(auprec);

    // paper format:
    bool landscape;
    RS2::PaperFormat format = graphic->getPaperFormat(&landscape);
        RS_DEBUG->print("QG_DlgOptionsDrawing::setGraphic: paper format is: %d", (int)format);
    cbPaperFormat->setCurrentIndex((int)format);

    // paper orientation:
    if (landscape) {
        rbLandscape->setChecked(true);
    } else {
        rbPortrait->setChecked(true);
    }

    // Grid:
    cbGridOn->setChecked(graphic->isGridOn());

    RS_Vector spacing = graphic->getVariableVector("$GRIDUNIT",
                        RS_Vector(0.0,0.0));
    cbXSpacing->setEditText( QString("%1").arg(spacing.x));
    cbYSpacing->setEditText( QString("%1").arg(spacing.y));

    if (cbXSpacing->currentText()=="0") {
        cbXSpacing->setEditText(tr("auto"));
    }
    if (cbYSpacing->currentText()=="0") {
        cbYSpacing->setEditText(tr("auto"));
    }

    // dimension text height:
    RS2::Unit unit = (RS2::Unit)cbUnit->currentIndex();

    double dimtxt = graphic->getVariableDouble("$DIMTXT",
                    RS_Units::convert(2.5, RS2::Millimeter, unit));
//RLZ    cbDimTextHeight->setCurrentText(QString("%1").arg(dimtxt));
    cbDimTextHeight->setEditText(QString("%1").arg(dimtxt));

    // dimension extension line extension:
    double dimexe = graphic->getVariableDouble("$DIMEXE",
                    RS_Units::convert(1.25, RS2::Millimeter, unit));
//RLZ    cbDimExe->setCurrentText(QString("%1").arg(dimexe));
    cbDimExe->setEditText(QString("%1").arg(dimexe));

    // dimension extension line offset:
    double dimexo = graphic->getVariableDouble("$DIMEXO",
                    RS_Units::convert(0.625, RS2::Millimeter, unit));
//RLZ    cbDimExo->setCurrentText(QString("%1").arg(dimexo));
    cbDimExo->setEditText(QString("%1").arg(dimexo));

    // dimension line gap:
    double dimgap = graphic->getVariableDouble("$DIMGAP",
                    RS_Units::convert(0.625, RS2::Millimeter, unit));
//RLZ    cbDimGap->setCurrentText(QString("%1").arg(dimgap));
    cbDimGap->setEditText(QString("%1").arg(dimgap));

    // dimension arrow size:
    double dimasz = graphic->getVariableDouble("$DIMASZ",
                    RS_Units::convert(2.5, RS2::Millimeter, unit));
//RLZ    cbDimAsz->setCurrentText(QString("%1").arg(dimasz));
    cbDimAsz->setEditText(QString("%1").arg(dimasz));

    // spline line segments per patch:
    int splinesegs = graphic->getVariableInt("$SPLINESEGS", 8);
//RLZ    cbSplineSegs->setCurrentText(QString("%1").arg(splinesegs));
    cbSplineSegs->setEditText(QString("%1").arg(splinesegs));

    RS_DEBUG->print("QG_DlgOptionsDrawing::setGraphic: splinesegs is: %d",
                    splinesegs);

    // encoding:
    /*
    QString encoding = graphic->getVariableString("$DWGCODEPAGE",
                                                  "ANSI_1252");
    encoding=RS_System::getEncoding(encoding);
    cbEncoding->setEditText(encoding);
    */

    updatePaperSize();
    updatePreview();
    updateUnitLabels();
}


/**
 * Called when OK is clicked.
 */
void QG_DlgOptionsDrawing::validate() {
    RS2::LinearFormat f = (RS2::LinearFormat)cbLengthFormat->currentIndex();
    if (f==RS2::Engineering || f==RS2::Architectural) {
        if (RS_Units::stringToUnit(cbUnit->currentText())!=RS2::Inch) {
            QMessageBox::warning( this, tr("Options"),
                                  tr("For the length formats 'Engineering' and 'Architectural', the "
                                     "unit must be set to Inch."),
                                  QMessageBox::Ok,
                                  Qt::NoButton);
            return;
        }
    }

    if (graphic!=NULL) {
        // units:
        graphic->setUnit((RS2::Unit)cbUnit->currentIndex());

        graphic->addVariable("$LUNITS", cbLengthFormat->currentIndex()+1, 70);
        graphic->addVariable("$DIMLUNIT", cbLengthFormat->currentIndex()+1, 70);
        graphic->addVariable("$LUPREC", cbLengthPrecision->currentIndex(), 70);

        graphic->addVariable("$AUNITS", cbAngleFormat->currentIndex(), 70);
        graphic->addVariable("$DIMAUNIT", cbAngleFormat->currentIndex(), 70);
        graphic->addVariable("$AUPREC", cbAnglePrecision->currentIndex(), 70);
        graphic->addVariable("$DIMADEC", cbAnglePrecision->currentIndex(), 70);

        // paper:
        graphic->setPaperFormat(
            (RS2::PaperFormat)cbPaperFormat->currentIndex(),
            rbLandscape->isChecked());
                // custom paper size:
                if ((RS2::PaperFormat)cbPaperFormat->currentIndex()==RS2::Custom) {
                        graphic->setPaperSize(
                              RS_Units::convert(
                                        RS_Vector(RS_Math::eval(lePaperWidth->text()),
                                          RS_Math::eval(lePaperHeight->text())),
                                        (RS2::Unit) cbUnit->currentIndex(),
                                        RS2::Millimeter));
                }

        // grid:
        //graphic->addVariable("$GRIDMODE", (int)cbGridOn->isChecked() , 70);
        graphic->setGridOn(cbGridOn->isChecked());
#ifdef  RS_VECTOR2D
        RS_Vector spacing(0.0,0.0);
#else
        RS_Vector spacing(0.0,0.0,0.0);
#endif
        if (cbXSpacing->currentText()==tr("auto")) {
            spacing.x = 0.0;
        } else {
            spacing.x = cbXSpacing->currentText().toDouble();
        }
        if (cbYSpacing->currentText()==tr("auto")) {
            spacing.y = 0.0;
        } else {
            spacing.y = cbYSpacing->currentText().toDouble();
        }
        graphic->addVariable("$GRIDUNIT", spacing, 10);

        // dim:
        graphic->addVariable("$DIMTXT",
                             RS_Math::eval(cbDimTextHeight->currentText()), 40);
        graphic->addVariable("$DIMEXE",
                             RS_Math::eval(cbDimExe->currentText()), 40);
        graphic->addVariable("$DIMEXO",
                             RS_Math::eval(cbDimExo->currentText()), 40);
        graphic->addVariable("$DIMGAP",
                             RS_Math::eval(cbDimGap->currentText()), 40);
        graphic->addVariable("$DIMASZ",
                             RS_Math::eval(cbDimAsz->currentText()), 40);

        // splines:
        graphic->addVariable("$SPLINESEGS",
                             (int)RS_Math::eval(cbSplineSegs->currentText()), 70);

        RS_DEBUG->print("QG_DlgOptionsDrawing::validate: splinesegs is: %s",
                        cbSplineSegs->currentText().toLatin1().data());

        // update all dimension and spline entities in the graphic to match the new settings:
        graphic->updateDimensions(false);
        graphic->updateSplines();

        graphic->setModified(true);
    }
    accept();
}


/**
 * Updates the length precision combobox
 */
void QG_DlgOptionsDrawing::updateLengthPrecision() {
    int index = cbLengthPrecision->currentIndex();
    cbLengthPrecision->clear();

    switch (cbLengthFormat->currentIndex()) {
        // scientific
    case 0:
        cbLengthPrecision->addItem("0E+01");
        cbLengthPrecision->addItem("0.0E+01");
        cbLengthPrecision->addItem("0.00E+01");
        cbLengthPrecision->addItem("0.000E+01");
        cbLengthPrecision->addItem("0.0000E+01");
        cbLengthPrecision->addItem("0.00000E+01");
        cbLengthPrecision->addItem("0.000000E+01");
        cbLengthPrecision->addItem("0.0000000E+01");
        cbLengthPrecision->addItem("0.00000000E+01");
        break;

        // decimal
        //   (0, 0.1, 0.01, ...)
    case 1:
        cbLengthPrecision->insertItems(0, listPrec1);
        break;

        // architectural:
    case 3:
        cbLengthPrecision->addItem("0'-0\"");
        cbLengthPrecision->addItem("0'-0 1/2\"");
        cbLengthPrecision->addItem("0'-0 1/4\"");
        cbLengthPrecision->addItem("0'-0 1/8\"");
        cbLengthPrecision->addItem("0'-0 1/16\"");
        cbLengthPrecision->addItem("0'-0 1/32\"");
        cbLengthPrecision->addItem("0'-0 1/64\"");
        cbLengthPrecision->addItem("0'-0 1/128\"");
        break;

        // engineering:
    case 2:
        cbLengthPrecision->addItem("0'-0\"");
        cbLengthPrecision->addItem("0'-0.0\"");
        cbLengthPrecision->addItem("0'-0.00\"");
        cbLengthPrecision->addItem("0'-0.000\"");
        cbLengthPrecision->addItem("0'-0.0000\"");
        cbLengthPrecision->addItem("0'-0.00000\"");
        cbLengthPrecision->addItem("0'-0.000000\"");
        cbLengthPrecision->addItem("0'-0.0000000\"");
        cbLengthPrecision->addItem("0'-0.00000000\"");
        break;

        // fractional
    case 4:
        cbLengthPrecision->addItem("0");
        cbLengthPrecision->addItem("0 1/2");
        cbLengthPrecision->addItem("0 1/4");
        cbLengthPrecision->addItem("0 1/8");
        cbLengthPrecision->addItem("0 1/16");
        cbLengthPrecision->addItem("0 1/32");
        cbLengthPrecision->addItem("0 1/64");
        cbLengthPrecision->addItem("0 1/128");
        break;

    default:
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_DlgOptionsDrawing::updateLengthPrecision: error");
        break;
    }

    cbLengthPrecision->setCurrentIndex(index);
}



/**
 * Updates the angle precision combobox
 */
void QG_DlgOptionsDrawing::updateAnglePrecision() {
    int index = cbAnglePrecision->currentIndex();
    cbAnglePrecision->clear();

    switch (cbAngleFormat->currentIndex()) {
        // decimal degrees:
    case 0:
        cbAnglePrecision->insertItems(0, listPrec1);
        break;

        // deg/min/sec:
    case 1:
        cbAnglePrecision->addItem(QString("0%1").arg(QChar(0xB0)));
        cbAnglePrecision->addItem(QString("0%100'").arg(QChar(0xB0)));
        cbAnglePrecision->addItem(QString("0%100'00\"").arg(QChar(0xB0)));
        cbAnglePrecision->addItem(QString("0%100'00.0\"").arg(QChar(0xB0)));
        cbAnglePrecision->addItem(QString("0%100'00.00\"").arg(QChar(0xB0)));
        cbAnglePrecision->addItem(QString("0%100'00.000\"").arg(QChar(0xB0)));
        cbAnglePrecision->addItem(QString("0%100'00.0000\"").arg(QChar(0xB0)));
        break;

        // gradians:
    case 2:
        cbAnglePrecision->addItem("0g");
        cbAnglePrecision->addItem("0.0g");
        cbAnglePrecision->addItem("0.00g");
        cbAnglePrecision->addItem("0.000g");
        cbAnglePrecision->addItem("0.0000g");
        cbAnglePrecision->addItem("0.00000g");
        cbAnglePrecision->addItem("0.000000g");
        cbAnglePrecision->addItem("0.0000000g");
        cbAnglePrecision->addItem("0.00000000g");
        break;

        // radians:
    case 3:
        cbAnglePrecision->addItem("0r");
        cbAnglePrecision->addItem("0.0r");
        cbAnglePrecision->addItem("0.00r");
        cbAnglePrecision->addItem("0.000r");
        cbAnglePrecision->addItem("0.0000r");
        cbAnglePrecision->addItem("0.00000r");
        cbAnglePrecision->addItem("0.000000r");
        cbAnglePrecision->addItem("0.0000000r");
        cbAnglePrecision->addItem("0.00000000r");
        break;

        // surveyor's units:
    case 4:
        cbAnglePrecision->addItem("N 0d E");
        cbAnglePrecision->addItem("N 0d00' E");
        cbAnglePrecision->addItem("N 0d00'00\" E");
        cbAnglePrecision->addItem("N 0d00'00.0\" E");
        cbAnglePrecision->addItem("N 0d00'00.00\" E");
        cbAnglePrecision->addItem("N 0d00'00.000\" E");
        cbAnglePrecision->addItem("N 0d00'00.0000\" E");
        break;

    default:
        break;
    }

    cbAnglePrecision->setCurrentIndex(index);
}

/**
 * Updates the preview of unit display.
 */
void QG_DlgOptionsDrawing::updatePreview() {
    QString prev;
    prev = RS_Units::formatLinear(14.43112351,
                                  (RS2::Unit)cbUnit->currentIndex(),
                                  (RS2::LinearFormat)(cbLengthFormat->currentIndex()),
                                  cbLengthPrecision->currentIndex());
    lLinear->setText(prev);

    prev = RS_Units::formatAngle(0.5327714,
                                 (RS2::AngleFormat)cbAngleFormat->currentIndex(),
                                 cbAnglePrecision->currentIndex());
    lAngular->setText(prev);
}



/**
 * Updates the paper size. Called for initialisation as well as when the
 * paper format changes.
 */
void  QG_DlgOptionsDrawing::updatePaperSize() {
    RS2::PaperFormat format = (RS2::PaperFormat)cbPaperFormat->currentIndex();

        if (format==RS2::Custom) {
            RS_Vector s = RS_Units::convert(
                 graphic->getPaperSize(),
                        RS2::Millimeter,
                        (RS2::Unit) cbUnit->currentIndex()
                        );
                //RS_Vector plimmin = graphic->getVariableVector("$PLIMMIN", RS_Vector(0,0));
                //RS_Vector plimmax = graphic->getVariableVector("$PLIMMAX", RS_Vector(100,100));
                lePaperWidth->setText(QString("%1").arg(s.x));
                lePaperHeight->setText(QString("%1").arg(s.y));
        }
        else {
            //display paper size according to current units
            RS_Vector s = RS_Units::convert(
                        RS_Units::paperFormatToSize(format),
                        RS2::Millimeter,
                        (RS2::Unit) cbUnit->currentIndex()
                        );
            lePaperWidth->setText(QString("%1").setNum(s.x,'g',5));
            lePaperHeight->setText(QString("%1").setNum(s.y,'g',5));
        }

    if (cbPaperFormat->currentIndex()==0) {
        lePaperWidth->setEnabled(true);
        lePaperHeight->setEnabled(true);
    } else {
        lePaperWidth->setEnabled(false);
        lePaperHeight->setEnabled(false);
    }
}



/**
 * Updates all unit labels that depend on the global unit.
 */
void QG_DlgOptionsDrawing::updateUnitLabels() {
    RS2::Unit u = (RS2::Unit)cbUnit->currentIndex();
    QString sign = RS_Units::unitToSign(u);
    lDimUnit1->setText(sign);
    lDimUnit2->setText(sign);
    lDimUnit3->setText(sign);
    lDimUnit4->setText(sign);
    lDimUnit5->setText(sign);
    //have to update paper size when unit changes
    updatePaperSize();
}

