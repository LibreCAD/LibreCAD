/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by 
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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

void QG_DlgOptionsDrawing::init() {
    graphic = NULL;

    // precision list:
    QString s;
    QString format;
    for (int i=0; i<=8; i++) {
        format.sprintf("%%.0%df", i);
        s.sprintf(format, 0.0);
        listPrec1 << s;
    }

    // Main drawing unit:
    for (int i=RS2::None; i<RS2::LastUnit; i++) {
        cbUnit->insertItem(RS_Units::unitToString((RS2::Unit)i));
    }

    // init units combobox:
    QStringList unitList;
    unitList << tr("Scientific")
    << tr("Decimal")
    << tr("Engineering")
    << tr("Architectural")
    << tr("Fractional");
    cbLengthFormat->insertStringList(unitList);

    // init angle units combobox:
    QStringList aunitList;
    aunitList << tr("Decimal Degrees")
    << tr("Deg/min/sec")
    << tr("Gradians")
    << tr("Radians")
    << tr("Surveyor's units");
    cbAngleFormat->insertStringList(aunitList);

    // Paper format:
    for (int i=RS2::Custom; i<=RS2::NPageSize; i++) {
        cbPaperFormat->insertItem(RS_Units::paperFormatToString((RS2::PaperFormat)i));
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
    cbEncoding->insertStringList(encodingList);
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
    cbUnit->setCurrentText(
        RS_Units::unitToString(RS_FilterDXF::numberToUnit(insunits)));

    // units / length format:
    int lunits = graphic->getVariableInt("$LUNITS", 2);
    cbLengthFormat->setCurrentItem(lunits-1);

    // units length precision:
    int luprec = graphic->getVariableInt("$LUPREC", 4);
    updateLengthPrecision();
    cbLengthPrecision->setCurrentItem(luprec);

    // units / angle format:
    int aunits = graphic->getVariableInt("$AUNITS", 0);
    cbAngleFormat->setCurrentItem(aunits);

    // units angle precision:
    int auprec = graphic->getVariableInt("$AUPREC", 2);
    updateAnglePrecision();
    cbAnglePrecision->setCurrentItem(auprec);

    // paper format:
    bool landscape;
    RS2::PaperFormat format = graphic->getPaperFormat(&landscape);
	RS_DEBUG->print("QG_DlgOptionsDrawing::setGraphic: paper format is: %d", (int)format);
    cbPaperFormat->setCurrentItem((int)format);

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
    cbXSpacing->setCurrentText(QString("%1").arg(spacing.x));
    cbYSpacing->setCurrentText(QString("%1").arg(spacing.y));

    if (cbXSpacing->currentText()=="0") {
        cbXSpacing->setCurrentText(tr("auto"));
    }
    if (cbYSpacing->currentText()=="0") {
        cbYSpacing->setCurrentText(tr("auto"));
    }

    // dimension text height:
    RS2::Unit unit = (RS2::Unit)cbUnit->currentItem();

    double dimtxt = graphic->getVariableDouble("$DIMTXT",
                    RS_Units::convert(2.5, RS2::Millimeter, unit));
    cbDimTextHeight->setCurrentText(QString("%1").arg(dimtxt));

    // dimension extension line extension:
    double dimexe = graphic->getVariableDouble("$DIMEXE",
                    RS_Units::convert(1.25, RS2::Millimeter, unit));
    cbDimExe->setCurrentText(QString("%1").arg(dimexe));

    // dimension extension line offset:
    double dimexo = graphic->getVariableDouble("$DIMEXO",
                    RS_Units::convert(0.625, RS2::Millimeter, unit));
    cbDimExo->setCurrentText(QString("%1").arg(dimexo));

    // dimension line gap:
    double dimgap = graphic->getVariableDouble("$DIMGAP",
                    RS_Units::convert(0.625, RS2::Millimeter, unit));
    cbDimGap->setCurrentText(QString("%1").arg(dimgap));

    // dimension arrow size:
    double dimasz = graphic->getVariableDouble("$DIMASZ",
                    RS_Units::convert(2.5, RS2::Millimeter, unit));
    cbDimAsz->setCurrentText(QString("%1").arg(dimasz));
    
    // spline line segments per patch:
    int splinesegs = graphic->getVariableInt("$SPLINESEGS", 8);
    cbSplineSegs->setCurrentText(QString("%1").arg(splinesegs));
    
    RS_DEBUG->print("QG_DlgOptionsDrawing::setGraphic: splinesegs is: %d",
                    splinesegs);
    
    // encoding:
    /*
    QString encoding = graphic->getVariableString("$DWGCODEPAGE",
                                                  "ANSI_1252");
    encoding=RS_System::getEncoding(encoding);
    cbEncoding->setCurrentText(encoding);
    */

    updatePaperSize();
    updatePreview();
    updateUnitLabels();
}


/**
 * Called when OK is clicked.
 */
void QG_DlgOptionsDrawing::validate() {
    RS2::LinearFormat f = (RS2::LinearFormat)cbLengthFormat->currentItem();
    if (f==RS2::Engineering || f==RS2::Architectural) {
        if (RS_Units::stringToUnit(cbUnit->currentText())!=RS2::Inch) {
            QMessageBox::warning( this, tr("Options"),
                                  tr("For the length formats 'Engineering' and 'Architectural', the "
                                     "unit must be set to Inch."),
                                  QMessageBox::Ok,
                                  QMessageBox::NoButton);
            return;
        }
    }

    if (graphic!=NULL) {
        // units:
        graphic->setUnit((RS2::Unit)cbUnit->currentItem());

        graphic->addVariable("$LUNITS", cbLengthFormat->currentItem()+1, 70);
        graphic->addVariable("$DIMLUNIT", cbLengthFormat->currentItem()+1, 70);
        graphic->addVariable("$LUPREC", cbLengthPrecision->currentItem(), 70);

        graphic->addVariable("$AUNITS", cbAngleFormat->currentItem(), 70);
        graphic->addVariable("$DIMAUNIT", cbAngleFormat->currentItem(), 70);
        graphic->addVariable("$AUPREC", cbAnglePrecision->currentItem(), 70);
        graphic->addVariable("$DIMADEC", cbAnglePrecision->currentItem(), 70);

        // paper:
        graphic->setPaperFormat(
            (RS2::PaperFormat)cbPaperFormat->currentItem(),
            rbLandscape->isChecked());
		// custom paper size:
		if ((RS2::PaperFormat)cbPaperFormat->currentItem()==RS2::Custom) {
			graphic->setPaperSize(
				RS_Vector(RS_Math::eval(lePaperWidth->text()),
				          RS_Math::eval(lePaperHeight->text())));
		}

        // grid:
        //graphic->addVariable("$GRIDMODE", (int)cbGridOn->isChecked() , 70);
        graphic->setGridOn(cbGridOn->isChecked());
        RS_Vector spacing(0.0,0.0,0.0);
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
                        cbSplineSegs->currentText().latin1());
        
        // update all dimension and spline entities in the graphic to match the new settings:
        graphic->updateDimensions();
        graphic->updateSplines();
        
        graphic->setModified(true);
    }
    accept();
}


/**
 * Updates the length precision combobox
 */
void QG_DlgOptionsDrawing::updateLengthPrecision() {
    int index = cbLengthPrecision->currentItem();
    cbLengthPrecision->clear();

    switch (cbLengthFormat->currentItem()) {
        // scientific
    case 0:
        cbLengthPrecision->insertItem("0E+01");
        cbLengthPrecision->insertItem("0.0E+01");
        cbLengthPrecision->insertItem("0.00E+01");
        cbLengthPrecision->insertItem("0.000E+01");
        cbLengthPrecision->insertItem("0.0000E+01");
        cbLengthPrecision->insertItem("0.00000E+01");
        cbLengthPrecision->insertItem("0.000000E+01");
        cbLengthPrecision->insertItem("0.0000000E+01");
        cbLengthPrecision->insertItem("0.00000000E+01");
        break;

        // decimal
        //   (0, 0.1, 0.01, ...)
    case 1:
        cbLengthPrecision->insertStringList(listPrec1);
        break;

        // architectural:
    case 3:
        cbLengthPrecision->insertItem("0'-0\"");
        cbLengthPrecision->insertItem("0'-0 1/2\"");
        cbLengthPrecision->insertItem("0'-0 1/4\"");
        cbLengthPrecision->insertItem("0'-0 1/8\"");
        cbLengthPrecision->insertItem("0'-0 1/16\"");
        cbLengthPrecision->insertItem("0'-0 1/32\"");
        cbLengthPrecision->insertItem("0'-0 1/64\"");
        cbLengthPrecision->insertItem("0'-0 1/128\"");
        break;

        // engineering:
    case 2:
        cbLengthPrecision->insertItem("0'-0\"");
        cbLengthPrecision->insertItem("0'-0.0\"");
        cbLengthPrecision->insertItem("0'-0.00\"");
        cbLengthPrecision->insertItem("0'-0.000\"");
        cbLengthPrecision->insertItem("0'-0.0000\"");
        cbLengthPrecision->insertItem("0'-0.00000\"");
        cbLengthPrecision->insertItem("0'-0.000000\"");
        cbLengthPrecision->insertItem("0'-0.0000000\"");
        cbLengthPrecision->insertItem("0'-0.00000000\"");
        break;

        // fractional
    case 4:
        cbLengthPrecision->insertItem("0");
        cbLengthPrecision->insertItem("0 1/2");
        cbLengthPrecision->insertItem("0 1/4");
        cbLengthPrecision->insertItem("0 1/8");
        cbLengthPrecision->insertItem("0 1/16");
        cbLengthPrecision->insertItem("0 1/32");
        cbLengthPrecision->insertItem("0 1/64");
        cbLengthPrecision->insertItem("0 1/128");
        break;

    default:
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_DlgOptionsDrawing::updateLengthPrecision: error");
        break;
    }

    cbLengthPrecision->setCurrentItem(index);
}



/**
 * Updates the angle precision combobox
 */
void QG_DlgOptionsDrawing::updateAnglePrecision() {
    int index = cbAnglePrecision->currentItem();
    cbAnglePrecision->clear();

    switch (cbAngleFormat->currentItem()) {
        // decimal degrees:
    case 0:
        cbAnglePrecision->insertStringList(listPrec1);
        break;

        // deg/min/sec:
    case 1:
        cbAnglePrecision->insertItem(QString("0%1").arg(QChar(0xB0)));
        cbAnglePrecision->insertItem(QString("0%100'").arg(QChar(0xB0)));
        cbAnglePrecision->insertItem(QString("0%100'00\"").arg(QChar(0xB0)));
        cbAnglePrecision->insertItem(QString("0%100'00.0\"").arg(QChar(0xB0)));
        cbAnglePrecision->insertItem(QString("0%100'00.00\"").arg(QChar(0xB0)));
        cbAnglePrecision->insertItem(QString("0%100'00.000\"").arg(QChar(0xB0)));
        cbAnglePrecision->insertItem(QString("0%100'00.0000\"").arg(QChar(0xB0)));
        break;

        // gradians:
    case 2:
        cbAnglePrecision->insertItem("0g");
        cbAnglePrecision->insertItem("0.0g");
        cbAnglePrecision->insertItem("0.00g");
        cbAnglePrecision->insertItem("0.000g");
        cbAnglePrecision->insertItem("0.0000g");
        cbAnglePrecision->insertItem("0.00000g");
        cbAnglePrecision->insertItem("0.000000g");
        cbAnglePrecision->insertItem("0.0000000g");
        cbAnglePrecision->insertItem("0.00000000g");
        break;

        // radians:
    case 3:
        cbAnglePrecision->insertItem("0r");
        cbAnglePrecision->insertItem("0.0r");
        cbAnglePrecision->insertItem("0.00r");
        cbAnglePrecision->insertItem("0.000r");
        cbAnglePrecision->insertItem("0.0000r");
        cbAnglePrecision->insertItem("0.00000r");
        cbAnglePrecision->insertItem("0.000000r");
        cbAnglePrecision->insertItem("0.0000000r");
        cbAnglePrecision->insertItem("0.00000000r");
        break;

        // surveyor's units:
    case 4:
        cbAnglePrecision->insertItem("N 0d E");
        cbAnglePrecision->insertItem("N 0d00' E");
        cbAnglePrecision->insertItem("N 0d00'00\" E");
        cbAnglePrecision->insertItem("N 0d00'00.0\" E");
        cbAnglePrecision->insertItem("N 0d00'00.00\" E");
        cbAnglePrecision->insertItem("N 0d00'00.000\" E");
        cbAnglePrecision->insertItem("N 0d00'00.0000\" E");
        break;

    default:
        break;
    }

    cbAnglePrecision->setCurrentItem(index);
}

/**
 * Updates the preview of unit display.
 */
void QG_DlgOptionsDrawing::updatePreview() {
    QString prev;
    prev = RS_Units::formatLinear(14.43112351,
                                  (RS2::Unit)cbUnit->currentItem(),
                                  (RS2::LinearFormat)(cbLengthFormat->currentItem()),
                                  cbLengthPrecision->currentItem());
    lLinear->setText(prev);

    prev = RS_Units::formatAngle(0.5327714,
                                 (RS2::AngleFormat)cbAngleFormat->currentItem(),
                                 cbAnglePrecision->currentItem());
    lAngular->setText(prev);
}



/**
 * Updates the paper size. Called for initialisation as well as when the 
 * paper format changes.
 */
void  QG_DlgOptionsDrawing::updatePaperSize() {
    RS2::PaperFormat format = (RS2::PaperFormat)cbPaperFormat->currentItem();

	if (format==RS2::Custom) {
		RS_Vector s = graphic->getPaperSize();
		//RS_Vector plimmin = graphic->getVariableVector("$PLIMMIN", RS_Vector(0,0));
		//RS_Vector plimmax = graphic->getVariableVector("$PLIMMAX", RS_Vector(100,100));
		lePaperWidth->setText(QString("%1").arg(s.x));
		lePaperHeight->setText(QString("%1").arg(s.y));
	}
	else {
	    RS_Vector s = RS_Units::paperFormatToSize(format);
	    lePaperWidth->setText(QString("%1").arg(s.x));
	    lePaperHeight->setText(QString("%1").arg(s.y));
	}

    if (cbPaperFormat->currentItem()==0) {
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
    RS2::Unit u = (RS2::Unit)cbUnit->currentItem();
    QString sign = RS_Units::unitToSign(u);
    lDimUnit1->setText(sign);
    lDimUnit2->setText(sign);
    lDimUnit3->setText(sign);
    lDimUnit4->setText(sign);
    lDimUnit5->setText(sign);
}
