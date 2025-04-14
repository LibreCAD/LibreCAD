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

#include <QMessageBox>
#include <QListView>
#include <cfloat>

#include "lc_defaults.h"
#include "qc_applicationwindow.h"
#include "rs_debug.h"
#include "rs_filterdxfrw.h"
#include "rs_font.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_units.h"
#include "rs_vector.h"

/*
 *  Constructs a QG_DlgOptionsDrawing as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */

QG_DlgOptionsDrawing::QG_DlgOptionsDrawing(QWidget* parent)
    : LC_Dialog(parent, "OptionsDrawing")
    , m_listPrec1(std::make_unique<QStringList>())
    ,m_paperScene{new QGraphicsScene(parent)}
    ,m_spacing{std::make_unique<RS_Vector>()}{
    setupUi(this);

    connect(rbLandscape, &QRadioButton::toggled, this, &QG_DlgOptionsDrawing::onLandscapeToggled);
    connect(cbDimFxLon, &QCheckBox::toggled, this, &QG_DlgOptionsDrawing::onDimFxLonToggled);
    connect(rbRelSize, &QRadioButton::toggled, this, &QG_DlgOptionsDrawing::onRelSizeToggled);

    connect(lePaperWidth, &QLineEdit::textChanged, this, &QG_DlgOptionsDrawing::updatePaperPreview);
    connect(lePaperHeight, &QLineEdit::textChanged, this, &QG_DlgOptionsDrawing::updatePaperPreview);
    connect(leMarginTop, &QLineEdit::textChanged, this, &QG_DlgOptionsDrawing::updatePaperPreview);
    connect(leMarginBottom, &QLineEdit::textChanged, this, &QG_DlgOptionsDrawing::updatePaperPreview);
    connect(leMarginRight, &QLineEdit::textChanged, this, &QG_DlgOptionsDrawing::updatePaperPreview);
    connect(leMarginLeft, &QLineEdit::textChanged, this, &QG_DlgOptionsDrawing::updatePaperPreview);

    connect(cbUnit, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updatePreview);
    connect(cbPaperFormat, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updatePaperSize);
    connect(cbLengthPrecision, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updatePreview);
    connect(cbLengthFormat, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updateLengthPrecision);
    connect(cbLengthFormat, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updatePreview);


    connect(cbDimLUnit, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updateDimLengthPrecision);
    connect(cbDimAUnit, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updateDimAnglePrecision);
    connect(cbAnglePrecision, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updatePreview);
    connect(cbAngleFormat, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updateAnglePrecision);
    connect(cbAngleFormat, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updatePreview);


    connect(rbIsoLeft, &QCheckBox::toggled, this, &QG_DlgOptionsDrawing::disableXSpacing);
    connect(rbIsoRight, &QCheckBox::toggled, this, &QG_DlgOptionsDrawing::disableXSpacing);
    connect(rbIsoTop, &QCheckBox::toggled, this, &QG_DlgOptionsDrawing::disableXSpacing);
    connect(rbOrthogonalGrid,  &QCheckBox::toggled, this, &QG_DlgOptionsDrawing::enableXSpacing);

    tabWidget->setCurrentIndex(0);
    init();
}
/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgOptionsDrawing::~QG_DlgOptionsDrawing(){
    // no need to delete child widgets, Qt does it all for us
/*
    // fixme - review and check what is affected by GridSpacingX, Y settings!!! it might be that they were used by previous grid drawing algorithm to set minimal grid spacing values!!!
    LC_GROUP_GUARD("Appearance");
    {
        LC_SET("IsometricGrid", rbIsometricGrid->isChecked() ? QString("1") : QString("0"));
        RS2::GridViewType chType(RS2::IsoTop);
        if (rbCrosshairLeft->isChecked()) {
            chType = RS2::IsoLeft;
        } else if (rbCrosshairTop->isChecked()) {
            chType = RS2::IsoTop;
        } else if (rbCrosshairRight->isChecked()) {
            chType = RS2::IsoRight;
        }
        LC_SET("CrosshairType", QString::number(static_cast<int>(chType)));
        if (spacing->valid) {
            LC_SET("GridSpacingX", spacing->x);
            LC_SET("GridSpacingY", spacing->y);
        }
    }
    */
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgOptionsDrawing::languageChange(){
    retranslateUi(this);
}

void QG_DlgOptionsDrawing::init() {
    m_graphic = nullptr;

    // precision list:
    for (int i=0; i<=8; i++)
        *m_listPrec1 << QString("%1").arg(0.0,0,'f', i);

    // Main drawing unit:
    for (int i=RS2::None; i<RS2::LastUnit; i++) {
        cbUnit->addItem(RS_Units::unitToString(static_cast<RS2::Unit>(i)));
    }

    // init units combobox:
    QStringList unitList;
    unitList << tr("Scientific")
             << tr("Decimal")
             << tr("Engineering")
             << tr("Architectural")
             << tr("Fractional")
             << tr("Architectural (metric)");
    cbLengthFormat->insertItems(0, unitList);
    cbDimLUnit->insertItems(0, unitList);

    // init angle units combobox:
    QStringList aunitList;
    aunitList << tr("Decimal Degrees")
              << tr("Deg/min/sec")
              << tr("Gradians")
              << tr("Radians")
              << tr("Surveyor's units");
    cbAngleFormat->insertItems(0, aunitList);
    cbDimAUnit->insertItems(0, aunitList);

    // Paper format:
    for (RS2::PaperFormat i = RS2::FirstPaperFormat; RS2::NPageFormat > i; i = static_cast<RS2::PaperFormat>(i + 1)) {
        cbPaperFormat->addItem( RS_Units::paperFormatToString( i));
    }
    // Paper preview:
    gvPaperPreview->setScene(m_paperScene);
    gvPaperPreview->setBackgroundBrush(this->palette().color(QPalette::Window));

    cbDimTxSty->init();
}

#define TO_MM(v) RS_Units::convert(2.5, RS2::Millimeter, unit)

/**
 * Sets the graphic and updates the GUI to match the drawing.
 */
void QG_DlgOptionsDrawing::setGraphic(RS_Graphic *g) {
    m_graphic = g;

    if (m_graphic == nullptr) {
        RS_DEBUG->print(" QG_DlgOptionsDrawing::setGraphic(nullptr)\n");
        return;
    }

    // main drawing unit:
    int insunits = m_graphic->getVariableInt("$INSUNITS", 0);
    cbUnit->setCurrentIndex(cbUnit->findText(
        RS_Units::unitToString(RS_FilterDXFRW::numberToUnit(insunits))));

    // units / length format:
    int lunits = m_graphic->getVariableInt("$LUNITS", 2);
    cbLengthFormat->setCurrentIndex(lunits - 1);

    // units length precision:
    int luprec = m_graphic->getVariableInt("$LUPREC", 4);
    updateCBLengthPrecision(cbLengthFormat, cbLengthPrecision);
    cbLengthPrecision->setCurrentIndex(luprec);

    // units / angle format:
    int aunits = m_graphic->getVariableInt("$AUNITS", 0);
    cbAngleFormat->setCurrentIndex(aunits);

    // units angle precision:
    int auprec = m_graphic->getVariableInt("$AUPREC", 2);
    updateCBAnglePrecision(cbAngleFormat, cbAnglePrecision);
    cbAnglePrecision->setCurrentIndex(auprec);

    // paper format:
    bool landscape;
    RS2::PaperFormat format = m_graphic->getPaperFormat(&landscape);
    RS_DEBUG->print("QG_DlgOptionsDrawing::setGraphic: paper format is: %d", (int) format);
    cbPaperFormat->setCurrentIndex((int) format);

    // paper orientation:
    rbLandscape->blockSignals(true);
    if (landscape) {
        rbLandscape->setChecked(true);
    } else {
        rbPortrait->setChecked(true);
    }
    rbLandscape->blockSignals(false);
    if (format == RS2::Custom) {
        RS_Vector s = m_graphic->getPaperSize();
        lePaperWidth->setText(QString("%1").setNum(s.x, 'g', 5));
        lePaperHeight->setText(QString("%1").setNum(s.y, 'g', 5));
        lePaperWidth->setEnabled(true);
        lePaperHeight->setEnabled(true);
    } else {
        lePaperWidth->setEnabled(false);
        lePaperHeight->setEnabled(false);
    }

    // Grid:
    cbGridOn->setChecked(m_graphic->isGridOn());
    bool isometricGrid = m_graphic->isIsometricGrid();
    if (isometricGrid){
        rbOrthogonalGrid ->setChecked(false);
        RS2::IsoGridViewType chType = m_graphic->getIsoView();
        switch (chType) {
            case RS2::IsoLeft:
                rbIsoLeft->setChecked(true);
                break;
            case RS2::IsoTop:
                rbIsoTop->setChecked(true);
                break;
            case RS2::IsoRight:
                rbIsoRight->setChecked(true);
                break;
            default:
                break;
        }
    }
    else{
        rbOrthogonalGrid->setChecked(true);
    }

    *m_spacing = m_graphic->getVariableVector("$GRIDUNIT",
                                          {0.0, 0.0});

    cbXSpacing->setEditText(QString("%1").arg(m_spacing->x));
    cbYSpacing->setEditText(QString("%1").arg(m_spacing->y));

    if (cbXSpacing->currentText() == "0") {
        cbXSpacing->setEditText(tr("auto"));
    }
    if (cbYSpacing->currentText() == "0") {
        cbYSpacing->setEditText(tr("auto"));
    }
    LC_GROUP("Appearance");
    {
        bool state = LC_GET_BOOL("ScaleGrid");
        lGridStateScaling->setText(state? tr("ON"): tr("OFF"));

        state = LC_GET_BOOL("UnitlessGrid");
        lGridStateUnitless->setText(state? tr("ON"): tr("OFF"));

        state = LC_GET_BOOL("GridDraw");
        lGridStateDrawGrid->setText(state? tr("ON"): tr("OFF"));

        state = LC_GET_BOOL("metaGridDraw");
        lGridStateDrawMetaGrid->setText(state? tr("ON"): tr("OFF"));
    }

    cbXSpacing->setEnabled(cbGridOn->isChecked() && rbOrthogonalGrid->isChecked());
    cbYSpacing->setEnabled(cbGridOn->isChecked());

    // dimension text height:
    auto unit = static_cast<RS2::Unit>(cbUnit->currentIndex());

    // dimension general factor:
    double dimfactor = m_graphic->getVariableDouble("$DIMLFAC", 1.0);
    cbDimFactor->setEditText(QString("%1").arg(dimfactor));

    // dimension general scale:
    double dimscale = m_graphic->getVariableDouble("$DIMSCALE", 1.0);
    cbDimScale->setEditText(QString("%1").arg(dimscale));

    double dimtxt = m_graphic->getVariableDouble("$DIMTXT", TO_MM(2.5));
    cbDimTextHeight->setEditText(QString("%1").arg(dimtxt));

    // dimension extension line extension:
    double dimexe = m_graphic->getVariableDouble("$DIMEXE", TO_MM(1.25));
    cbDimExe->setEditText(QString("%1").arg(dimexe));

    // dimension extension line offset:
    double dimexo = m_graphic->getVariableDouble("$DIMEXO", TO_MM(0.625));
    cbDimExo->setEditText(QString("%1").arg(dimexo));

    // dimension line gap:
    double dimgap = m_graphic->getVariableDouble("$DIMGAP", TO_MM(0.625));
    cbDimGap->setEditText(QString("%1").arg(dimgap));

    // dimension arrow size:
    double dimasz = m_graphic->getVariableDouble("$DIMASZ", TO_MM(2.5));
    cbDimAsz->setEditText(QString("%1").arg(dimasz));

    // dimension tick size:
    double dimtsz = m_graphic->getVariableDouble("$DIMTSZ", 0.);
    cbDimTsz->setEditText(QString("%1").arg(dimtsz));
    // dimension alignment:
    int dimtih = m_graphic->getVariableInt("$DIMTIH", 0);
    cbDimTih->setCurrentIndex(dimtih);
//RLZ todo add more options for dimensions
    cbDimClrT->init(true, false);
    cbDimClrE->init(true, false);
    cbDimClrD->init(true, false);
    cbDimLwD->init(true, false);
    cbDimLwE->init(true, false);
    // fixed extension length:
    double dimfxl = m_graphic->getVariableDouble("$DIMFXL",TO_MM(1.0));
    cbDimFxL->setValue(dimfxl);
    int dimfxlon = m_graphic->getVariableInt("$DIMFXLON", 0);
    if (dimfxlon > 0) {
        cbDimFxL->setEnabled(true);
        cbDimFxLon->setChecked(true);
    } else {
        cbDimFxL->setEnabled(false);
        cbDimFxLon->setChecked(false);
    }
    int dimlwd = m_graphic->getVariableInt("$DIMLWD", -2); //default ByBlock
//    LC_ERR<<__func__<<"() line "<<__LINE__<<": DIMLWD="<<dimlwd;
    RS2::LineWidth lineWidth = RS2::intToLineWidth(dimlwd);
    cbDimLwD->setWidth(lineWidth);
    int dimlwe = m_graphic->getVariableInt("$DIMLWE", -2); //default ByBlock
    cbDimLwE->setWidth(RS2::intToLineWidth(dimlwe));
//    LC_ERR<<__func__<<"() line "<<__LINE__<<": DIMLwe="<<dimlwe;


    // Dimensions / length format:
    int dimlunit = m_graphic->getVariableInt("$DIMLUNIT", lunits);
    cbDimLUnit->setCurrentIndex(dimlunit - 1);

    // Dimensions length precision:
    int dimdec = m_graphic->getVariableInt("$DIMDEC", luprec);
    updateCBLengthPrecision(cbDimLUnit, cbDimDec);
    cbDimDec->setCurrentIndex(dimdec);
    // Dimensions length zeros:
    int dimzin = m_graphic->getVariableInt("$DIMZIN", 1);
    cbDimZin->setLinear();
    cbDimZin->setData(dimzin);

    // Dimensions / angle format:
    int dimaunit = m_graphic->getVariableInt("$DIMAUNIT", aunits);
    cbDimAUnit->setCurrentIndex(dimaunit);

    // Dimensions angle precision:
    int dimadec = m_graphic->getVariableInt("$DIMADEC", auprec);
    updateCBAnglePrecision(cbDimAUnit, cbDimADec);
    cbDimADec->setCurrentIndex(dimadec);
    // Dimensions angle zeros:
    int dimazin = m_graphic->getVariableInt("$DIMAZIN", 0);
//    cbDimAZin->setCurrentIndex(dimazin);
    cbDimAZin->setData(dimazin);

    int dimclrd = m_graphic->getVariableInt("$DIMCLRD", 0);
    int dimclre = m_graphic->getVariableInt("$DIMCLRE", 0);
    int dimclrt = m_graphic->getVariableInt("$DIMCLRT", 0);
    cbDimClrD->setColor(RS_FilterDXFRW::numberToColor(dimclrd));
    cbDimClrE->setColor(RS_FilterDXFRW::numberToColor(dimclre));
    cbDimClrT->setColor(RS_FilterDXFRW::numberToColor(dimclrt));

    QString dimtxsty = m_graphic->getVariableString("$DIMTXSTY", "standard");
    cbDimTxSty->setFont(dimtxsty);
    int dimdsep = m_graphic->getVariableInt("$DIMDSEP", 0);
    (dimdsep == 44) ? cbDimDSep->setCurrentIndex(1) : cbDimDSep->setCurrentIndex(0);

    // spline line segments per patch:
    int splinesegs = m_graphic->getVariableInt("$SPLINESEGS", 8);
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

    // Paper margins
    bool block = true;
    leMarginLeft->blockSignals(block);
    leMarginRight->blockSignals(block);
    leMarginTop->blockSignals(block);
    leMarginBottom->blockSignals(block);

    leMarginLeft->setText(QString::number(m_graphic->getMarginLeftInUnits()));
    leMarginTop->setText(QString::number(m_graphic->getMarginTopInUnits()));
    leMarginRight->setText(QString::number(m_graphic->getMarginRightInUnits()));
    leMarginBottom->setText(QString::number(m_graphic->getMarginBottomInUnits()));

    block = false;
    leMarginLeft->blockSignals(block);
    leMarginRight->blockSignals(block);
    leMarginTop->blockSignals(block);
    leMarginBottom->blockSignals(block);

    updatePaperSize();
    updateUnitLabels();
    updatePaperPreview();

    // Number of pages
    sbPagesNumH->setValue(m_graphic->getPagesNumHoriz());
    sbPagesNumV->setValue(m_graphic->getPagesNumVert());

// Points drawing style:
    int pdmode = m_graphic->getVariableInt("$PDMODE", LC_DEFAULTS_PDMode);
    double pdsize = m_graphic->getVariableDouble("$PDSIZE", LC_DEFAULTS_PDSize);

// Set button checked for the currently selected point style
    switch (pdmode) {
        case DXF_FORMAT_PDMode_CentreDot:
        default:
            bDot->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_CentreBlank:
            bBlank->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_CentrePlus:
            bPlus->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_CentreCross:
            bCross->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_CentreTick:
            bTick->setChecked(true);
            break;

        case DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreDot):
            bDotCircle->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreBlank):
            bBlankCircle->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentrePlus):
            bPlusCircle->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreCross):
            bCrossCircle->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreTick):
            bTickCircle->setChecked(true);
            break;

        case DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreDot):
            bDotSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreBlank):
            bBlankSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentrePlus):
            bPlusSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreCross):
            bCrossSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreTick):
            bTickSquare->setChecked(true);
            break;

        case DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreDot):
            bDotCircleSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreBlank):
            bBlankCircleSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentrePlus):
            bPlusCircleSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreCross):
            bCrossCircleSquare->setChecked(true);
            break;
        case DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreTick):
            bTickCircleSquare->setChecked(true);
            break;
    }

// Fill points display size value string, and set button checked for screen-size
// relative vs. absolute drawing units radio buttons. Negative pdsize => value
// gives points size as percent of screen size; positive pdsize => value gives
// points size in absolute drawing units; pdsize == 0 implies points size to be
// 5% relative to screen size.
    if (pdsize <= 0.0)
        rbRelSize->setChecked(true);
    else
        rbAbsSize->setChecked(true);
    lePointSize->setText(QString::number(pdsize >= 0.0 ? pdsize : -pdsize));

// Set the appropriate text for the display size value label
    updateLPtSzUnits();

    int lineCaps = m_graphic->getGraphicVariableInt("$ENDCAPS", 1);
    cbLineCap->setCurrentIndex(lineCaps);

    int joinStyle = m_graphic->getGraphicVariableInt("$JOINSTYLE", 1);
    cbLineJoin ->setCurrentIndex(joinStyle);

    // angles system setup
    double baseAngle = m_graphic->getAnglesBase();
    leAngleBaseZero->setText(QString("%1").arg(RS_Math::rad2deg(baseAngle)));

    bool anglesAreCounterClockwise = m_graphic->areAnglesCounterClockWise();
    rbAngleBasePositive->setChecked(anglesAreCounterClockwise);
    rbAngleBaseNegative->setChecked(!anglesAreCounterClockwise);

    initVariables();
}

void QG_DlgOptionsDrawing::initVariables(){
    QHash<QString, RS_Variable>vars = m_graphic->getVariableDict();
    tabVariables->setRowCount(vars.count());
    QHash<QString, RS_Variable>::iterator it = vars.begin();
    int row = 0;
    while (it != vars.end()) {
        QString name = it.key();
        if (name.startsWith("$")){
            name = name.mid(1);
        }
        auto *nameItem = new QTableWidgetItem(name);
        tabVariables->setItem(row, 0, nameItem);

        auto* codeItem = new QTableWidgetItem(QString("%1").arg(it.value().getCode()));
        tabVariables->setItem(row, 1, codeItem);

        QString str = "";
        switch (it.value().getType()) {
            case RS2::VariableVoid:
                tabVariables->setItem(row, 2, new QTableWidgetItem(tr("VOID")));
                break;
            case RS2::VariableInt:
                tabVariables->setItem(row, 2, new QTableWidgetItem(tr("INT")));
                str = QString("%1").arg(it.value().getInt());
                break;
            case RS2::VariableDouble:
                tabVariables->setItem(row, 2, new QTableWidgetItem(tr("DOUBLE")));
                str = QString("%1").arg(it.value().getDouble());
                break;
            case RS2::VariableString:
                tabVariables->setItem(row, 2, new QTableWidgetItem(tr("STRING")));
                str = QString("%1").arg(it.value().getString());
                break;
            case RS2::VariableVector:
                tabVariables->setItem(row, 2, new QTableWidgetItem(tr("VECTOR")));
                str = QString("%1/%2")
                    .arg(it.value().getVector().x)
                    .arg(it.value().getVector().y);
                if (RS_FilterDXFRW::isVariableTwoDimensional(it.key())==false) {
                    str+= QString("/%1").arg(it.value().getVector().z);
                }
                break;
        }
        QTableWidgetItem *valueItem = new QTableWidgetItem(str);
        tabVariables->setItem(row, 3, valueItem);
        row++;
        ++it;
    }
    tabVariables->sortByColumn(0, Qt::SortOrder::AscendingOrder);
    tabVariables->setSortingEnabled(true);
}


/**
 * Called when OK is clicked.
 */
void QG_DlgOptionsDrawing::validate() {
    auto f = (RS2::LinearFormat) cbLengthFormat->currentIndex();
    if (f == RS2::Engineering || f == RS2::Architectural) {
        if (static_cast<RS2::Unit>(cbUnit->currentIndex()) != RS2::Inch) {
            QMessageBox::warning(this, tr("Options"),
                                 tr("For the length formats 'Engineering' and 'Architectural', the "
                                    "unit must be set to Inch."));
            return;
        }
    }
    if (f == RS2::ArchitecturalMetric) {
        if (static_cast<RS2::Unit>(cbUnit->currentIndex()) != RS2::Meter) {
            QMessageBox::warning(this, tr("Options"),
                                 tr("For the length format 'Architectural (metric)', the "
                                    "unit must be set to Meter."));
            return;
        }
    }

    if (m_graphic != nullptr) {
        // units:
        auto unit = static_cast<RS2::Unit>(cbUnit->currentIndex());
        m_graphic->setUnit(unit);

        RS_Units::setCurrentDrawingUnits(unit);

        m_graphic->addVariable("$LUNITS", cbLengthFormat->currentIndex() + 1, 70);
        m_graphic->addVariable("$LUPREC", cbLengthPrecision->currentIndex(), 70);
        m_graphic->addVariable("$AUNITS", cbAngleFormat->currentIndex(), 70);
        m_graphic->addVariable("$AUPREC", cbAnglePrecision->currentIndex(), 70);

        RS2::PaperFormat currentFormat{static_cast<RS2::PaperFormat>(cbPaperFormat->currentIndex())};
        // paper:
        m_graphic->setPaperFormat(currentFormat, rbLandscape->isChecked());
        // custom paper size:
        if (RS2::Custom == currentFormat) {
            m_graphic->setPaperSize(RS_Vector(RS_Math::eval(lePaperWidth->text()),
                                            RS_Math::eval(lePaperHeight->text())));
            bool landscape;
            m_graphic->getPaperFormat(&landscape);
            rbLandscape->setChecked(landscape);
        }

        // Pager margins:
        m_graphic->setMarginsInUnits(RS_Math::eval(leMarginLeft->text()),
                                   RS_Math::eval(leMarginTop->text()),
                                   RS_Math::eval(leMarginRight->text()),
                                   RS_Math::eval(leMarginBottom->text()));
        // Number of pages:
        m_graphic->setPagesNum(sbPagesNumH->value(),
                             sbPagesNumV->value());

        // grid:
        //graphic->addVariable("$GRIDMODE", (int)cbGridOn->isChecked() , 70);

        emit QC_ApplicationWindow::getAppWindow()->gridChanged(cbGridOn->isChecked());

        *m_spacing = RS_Vector{0.0, 0.0, 0.0};
        m_graphic->setGridOn(cbGridOn->isChecked());
        *m_spacing = RS_Vector{0.0, 0.0};
        if (cbXSpacing->currentText() == tr("auto")) {
            m_spacing->x = 0.0;
        } else {
            m_spacing->x = cbXSpacing->currentText().toDouble();
        }
        if (cbYSpacing->currentText() == tr("auto")) {
            m_spacing->y = 0.0;
        } else {
            m_spacing->y = cbYSpacing->currentText().toDouble();
        }
        m_graphic->addVariable("$GRIDUNIT", *m_spacing, 10);

        // dim:
        bool ok1 = true;
        double oldValue = m_graphic->getVariableDouble("$DIMTXT", 1.);
        double newValue = RS_Math::eval(cbDimTextHeight->currentText(), &ok1);
        //only update text height if a valid new position is specified, bug#3470605
        if (ok1 && (std::abs(oldValue - newValue) > RS_TOLERANCE)) {
            m_graphic->addVariable("$DIMTXT", newValue, 40);
        }
        m_graphic->addVariable("$DIMEXE",
                             RS_Math::eval(cbDimExe->currentText()), 40);
        m_graphic->addVariable("$DIMEXO",
                             RS_Math::eval(cbDimExo->currentText()), 40);
        bool ok2 = true;
        oldValue = m_graphic->getVariableDouble("$DIMGAP", 1);
        newValue = RS_Math::eval(cbDimGap->currentText(), &ok2);
        //only update text position if a valid new position is specified, bug#3470605
        ok2 &= (std::abs(oldValue - newValue) > RS_TOLERANCE);
        if (ok2) {
            m_graphic->addVariable("$DIMGAP", newValue, 40);
        }
        ok1 = ok1 || ok2;
        oldValue = m_graphic->getVariableDouble("$DIMLFAC", 1);
        newValue = RS_Math::eval(cbDimFactor->currentText(), &ok2);
        ok2 &= (std::abs(oldValue - newValue) > RS_TOLERANCE);
        ok1 = ok1 || ok2;
        oldValue = m_graphic->getVariableDouble("$DIMSCALE", 1);
        newValue = RS_Math::eval(cbDimScale->currentText(), &ok2);
        ok2 &= (std::abs(oldValue - newValue) > RS_TOLERANCE);
        ok1 = ok1 || ok2;

        m_graphic->addVariable("$DIMASZ",
                             RS_Math::eval(cbDimAsz->currentText()), 40);
        //dimension tick size, 0 for no tick
        m_graphic->addVariable("$DIMTSZ",
                             RS_Math::eval(cbDimTsz->currentText()), 40);
        //DIMTIH, dimension text, horizontal or aligned
        int iOldIndex = m_graphic->getVariableInt("$DIMTIH", 0);
        int iNewIndex = cbDimTih->currentIndex();
        if (iOldIndex != iNewIndex) {
            ok1 = true;
            m_graphic->addVariable("$DIMTIH", iNewIndex, 70);
        }
        //DIMLFAC, general factor for linear dimensions
        double dimFactor = RS_Math::eval(cbDimFactor->currentText());
        if (RS_TOLERANCE > std::abs(dimFactor)) {
            dimFactor = 1.0;
        }
        m_graphic->addVariable("$DIMLFAC", dimFactor, 40);
        //DIMSCALE, general scale for dimensions
        double dimScale = RS_Math::eval(cbDimScale->currentText());
        if (dimScale <= DBL_EPSILON)
            dimScale = 1.0;
        m_graphic->addVariable("$DIMSCALE", dimScale, 40);

        RS2::LineWidth dimLwDLineWidth = cbDimLwD->getWidth();
        int lineWidthDValue = RS2::lineWidthToInt(dimLwDLineWidth);
                m_graphic->addVariable("$DIMLWD", lineWidthDValue, 70);
//    LC_ERR<<__func__<<"() line "<<__LINE__<<": DIMLWD="<<lineWidthDValue;

        RS2::LineWidth dimLwELineWidth = cbDimLwE->getWidth();
        int lineWidthEValue = RS2::lineWidthToInt(dimLwELineWidth);
        m_graphic->addVariable("$DIMLWE", lineWidthEValue, 70);
//    LC_ERR<<__func__<<"() line "<<__LINE__<<": DIMLWE="<<lineWidthEValue;

        m_graphic->addVariable("$DIMFXL", cbDimFxL->value(), 40);
        m_graphic->addVariable("$DIMFXLON", cbDimFxLon->isChecked() ? 1 : 0, 70);
        m_graphic->addVariable("$DIMLUNIT", cbDimLUnit->currentIndex() + 1, 70);
        m_graphic->addVariable("$DIMDEC", cbDimDec->currentIndex(), 70);
        m_graphic->addVariable("$DIMZIN", cbDimZin->getData(), 70);
        m_graphic->addVariable("$DIMAUNIT", cbDimAUnit->currentIndex(), 70);
        m_graphic->addVariable("$DIMADEC", cbDimADec->currentIndex(), 70);
//        graphic->addVariable("$DIMAZIN", cbDimAZin->currentIndex(), 70);
        m_graphic->addVariable("$DIMAZIN", cbDimAZin->getData(), 70);
        int colNum, colRGB;
        colNum = RS_FilterDXFRW::colorToNumber(cbDimClrD->getColor(), &colRGB);
        m_graphic->addVariable("$DIMCLRD", colNum, 70);
        colNum = RS_FilterDXFRW::colorToNumber(cbDimClrE->getColor(), &colRGB);
        m_graphic->addVariable("$DIMCLRE", colNum, 70);
        colNum = RS_FilterDXFRW::colorToNumber(cbDimClrT->getColor(), &colRGB);
        m_graphic->addVariable("$DIMCLRT", colNum, 70);
        if (cbDimTxSty->getFont())
            m_graphic->addVariable("$DIMTXSTY", cbDimTxSty->getFont()->getFileName(), 2);
        m_graphic->addVariable("$DIMDSEP", (cbDimDSep->currentIndex() == 1) ? 44 : 0, 70);

        // splines:
        m_graphic->addVariable("$SPLINESEGS",
                             (int) RS_Math::eval(cbSplineSegs->currentText()), 70);

        RS_DEBUG->print("QG_DlgOptionsDrawing::validate: splinesegs is: %s",
                        cbSplineSegs->currentText().toLatin1().data());

        // update all dimension and spline entities in the graphic to match the new settings:
        // update text position when text height or text gap changed
        m_graphic->updateDimensions(ok1);
        m_graphic->updateSplines();

// Points drawing style:
// Get currently selected point style from which button is checked
        int pdmode = LC_DEFAULTS_PDMode;

        if (bDot->isChecked())
            pdmode = DXF_FORMAT_PDMode_CentreDot;
        else if (bBlank->isChecked())
            pdmode = DXF_FORMAT_PDMode_CentreBlank;
        else if (bPlus->isChecked())
            pdmode = DXF_FORMAT_PDMode_CentrePlus;
        else if (bCross->isChecked())
            pdmode = DXF_FORMAT_PDMode_CentreCross;
        else if (bTick->isChecked())
            pdmode = DXF_FORMAT_PDMode_CentreTick;

        else if (bDotCircle->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreDot);
        else if (bBlankCircle->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreBlank);
        else if (bPlusCircle->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentrePlus);
        else if (bCrossCircle->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreCross);
        else if (bTickCircle->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreTick);

        else if (bDotSquare->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreDot);
        else if (bBlankSquare->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreBlank);
        else if (bPlusSquare->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentrePlus);
        else if (bCrossSquare->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreCross);
        else if (bTickSquare->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreTick);

        else if (bDotCircleSquare->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreDot);
        else if (bBlankCircleSquare->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreBlank);
        else if (bPlusCircleSquare->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentrePlus);
        else if (bCrossCircleSquare->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreCross);
        else if (bTickCircleSquare->isChecked())
            pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreTick);

        m_graphic->addVariable("$PDMODE", pdmode, DXF_FORMAT_GC_PDMode);

// Get points display size from the value string and the relative vs. absolute
// size radio buttons state
        bool ok;
        double pdsize = RS_Math::eval(lePointSize->text(), &ok);
        if (!ok)
            pdsize = LC_DEFAULTS_PDSize;

        if (pdsize > 0.0 && rbRelSize->isChecked())
            pdsize = -pdsize;

        m_graphic->addVariable("$PDSIZE", pdsize, DXF_FORMAT_GC_PDSize);

        bool isometricGrid = !rbOrthogonalGrid->isChecked();

        m_graphic->setIsometricGrid(isometricGrid);

        RS2::IsoGridViewType isoView = RS2::IsoGridViewType::IsoTop;
        if (isometricGrid){
            if (rbIsoLeft->isChecked()){
                isoView = RS2::IsoGridViewType::IsoLeft;
            }
            else if (rbIsoTop->isChecked()){
                isoView = RS2::IsoGridViewType::IsoTop;
            }
            else if (rbIsoRight->isChecked()){
                isoView = RS2::IsoGridViewType::IsoRight;
            }
            m_graphic->setIsoView(isoView);
        }

        m_graphic->addVariable("$JOINSTYLE", cbLineJoin ->currentIndex(), DXF_FORMAT_GC_JoinStyle);
        m_graphic->addVariable("$ENDCAPS", cbLineCap->currentIndex(), DXF_FORMAT_GC_Endcaps);

        // angles system

        double baseAngle = RS_Math::eval(leAngleBaseZero->text(), &ok);
        if (ok){
            double baseAngleRad = RS_Math::deg2rad(baseAngle);
            m_graphic->setAnglesBase(baseAngleRad);
        }
        bool anglesCounterClockwise = rbAngleBasePositive->isChecked();
        m_graphic->setAnglesCounterClockwise(anglesCounterClockwise);

        // indicate graphic is modified and requires save
        m_graphic->setModified(true);
    }
    accept();
}

/**
 * Updates the length precision combobox
 */
void QG_DlgOptionsDrawing::updateLengthPrecision() {
    updateCBLengthPrecision(cbLengthFormat, cbLengthPrecision);
}

/**
 * Updates the Dimension length precision combobox
 */
void QG_DlgOptionsDrawing::updateDimLengthPrecision() {
    updateCBLengthPrecision(cbDimLUnit, cbDimDec);
}
/**
 * Updates the length precision combobox
 */
void QG_DlgOptionsDrawing::updateCBLengthPrecision(QComboBox* f, QComboBox* p) {
    int index = p->currentIndex();
    p->clear();

    switch (f->currentIndex()) {
    // scientific
    case 0:
        p->addItem("0E+01");
        p->addItem("0.0E+01");
        p->addItem("0.00E+01");
        p->addItem("0.000E+01");
        p->addItem("0.0000E+01");
        p->addItem("0.00000E+01");
        p->addItem("0.000000E+01");
        p->addItem("0.0000000E+01");
        p->addItem("0.00000000E+01");
        break;

        // decimal
        //   (0, 0.1, 0.01, ...)
    case 1:
        p->insertItems(0, *m_listPrec1);
        break;

        // architectural:
    case 3:
        p->addItem("0'-0\"");
        p->addItem("0'-0 1/2\"");
        p->addItem("0'-0 1/4\"");
        p->addItem("0'-0 1/8\"");
        p->addItem("0'-0 1/16\"");
        p->addItem("0'-0 1/32\"");
        p->addItem("0'-0 1/64\"");
        p->addItem("0'-0 1/128\"");
        break;

        // engineering:
    case 2:
        p->addItem("0'-0\"");
        p->addItem("0'-0.0\"");
        p->addItem("0'-0.00\"");
        p->addItem("0'-0.000\"");
        p->addItem("0'-0.0000\"");
        p->addItem("0'-0.00000\"");
        p->addItem("0'-0.000000\"");
        p->addItem("0'-0.0000000\"");
        p->addItem("0'-0.00000000\"");
        break;

        // fractional
    case 4:
        p->addItem("0");
        p->addItem("0 1/2");
        p->addItem("0 1/4");
        p->addItem("0 1/8");
        p->addItem("0 1/16");
        p->addItem("0 1/32");
        p->addItem("0 1/64");
        p->addItem("0 1/128");
        break;

        // architectural metric
    case 5:
        p->insertItems(0, *m_listPrec1);
        break;

    default:
        RS_DEBUG->print(RS_Debug::D_ERROR,
                        "QG_DlgOptionsDrawing::updateLengthPrecision: error");
        break;
    }

    p->setCurrentIndex(index);
}

/**
 * Updates the angle precision combobox
 */
void QG_DlgOptionsDrawing::updateAnglePrecision() {
    updateCBAnglePrecision(cbAngleFormat, cbAnglePrecision);
}

/**
 * Updates the dimension angle precision combobox
 */
void QG_DlgOptionsDrawing::updateDimAnglePrecision() {
    updateCBAnglePrecision(cbDimAUnit, cbDimADec);
}

/**
 * Updates the angle precision combobox
 */
void QG_DlgOptionsDrawing::updateCBAnglePrecision(QComboBox* u, QComboBox* p) {
    int index = p->currentIndex();
    p->clear();

    switch (u->currentIndex()) {
    // decimal degrees:
    case 0:
        p->insertItems(0, *m_listPrec1);
        break;

        // deg/min/sec:
    case 1:
        p->addItem(QString("0%1").arg(QChar(0xB0)));
        p->addItem(QString("0%100'").arg(QChar(0xB0)));
        p->addItem(QString("0%100'00\"").arg(QChar(0xB0)));
        p->addItem(QString("0%100'00.0\"").arg(QChar(0xB0)));
        p->addItem(QString("0%100'00.00\"").arg(QChar(0xB0)));
        p->addItem(QString("0%100'00.000\"").arg(QChar(0xB0)));
        p->addItem(QString("0%100'00.0000\"").arg(QChar(0xB0)));
        break;

        // gradians:
    case 2:
        p->addItem("0g");
        p->addItem("0.0g");
        p->addItem("0.00g");
        p->addItem("0.000g");
        p->addItem("0.0000g");
        p->addItem("0.00000g");
        p->addItem("0.000000g");
        p->addItem("0.0000000g");
        p->addItem("0.00000000g");
        break;

        // radians:
    case 3:
        p->addItem("0r");
        p->addItem("0.0r");
        p->addItem("0.00r");
        p->addItem("0.000r");
        p->addItem("0.0000r");
        p->addItem("0.00000r");
        p->addItem("0.000000r");
        p->addItem("0.0000000r");
        p->addItem("0.00000000r");
        break;

        // surveyor's units:
    case 4:
        p->addItem("N 0d E");
        p->addItem("N 0d00' E");
        p->addItem("N 0d00'00\" E");
        p->addItem("N 0d00'00.0\" E");
        p->addItem("N 0d00'00.00\" E");
        p->addItem("N 0d00'00.000\" E");
        p->addItem("N 0d00'00.0000\" E");
        break;

    default:
        break;
    }

    p->setCurrentIndex(index);
}

/**
 * Updates the preview of unit display.
 */
void QG_DlgOptionsDrawing::updatePreview() {
    QString prev = RS_Units::formatLinear(14.43112351,
								  static_cast<RS2::Unit>(cbUnit->currentIndex()),
								  static_cast<RS2::LinearFormat>(cbLengthFormat->currentIndex()),
                                  cbLengthPrecision->currentIndex());
    lLinear->setText(prev);

    prev = RS_Units::formatAngle(0.5327714,
								 static_cast<RS2::AngleFormat>(cbAngleFormat->currentIndex()),
                                 cbAnglePrecision->currentIndex());
    lAngular->setText(prev);
}

/**
 * Updates the paper size. Called for initialisation as well as when the
 * paper format changes.
 */
void  QG_DlgOptionsDrawing::updatePaperSize() {
    auto format = (RS2::PaperFormat)cbPaperFormat->currentIndex();

    RS_Vector s; //paper size: width, height
    if (format==RS2::Custom) {
        s.x = RS_Math::eval(lePaperWidth->text());
        s.y = RS_Math::eval(lePaperHeight->text());
    }
    else {
        //display paper size according to current units
        s = RS_Units::convert(
            RS_Units::paperFormatToSize(format),
            RS2::Millimeter,
            static_cast<RS2::Unit>(cbUnit->currentIndex())
        );
    }

    if (rbLandscape->isChecked() != (s.x > s.y)) {
        std::swap(s.x, s.y);
    }
    m_graphic->setPaperSize(s);

    lePaperWidth->blockSignals(true);
    lePaperWidth->setText(QString("%1").setNum(s.x,'g',5));
    lePaperWidth->blockSignals(false);

    lePaperHeight->blockSignals(true);
    lePaperHeight->setText(QString("%1").setNum(s.y,'g',5));
    lePaperHeight->blockSignals(false);

    if (RS2::Custom == cbPaperFormat->currentIndex()) {
        lePaperWidth->setEnabled(true);
        lePaperHeight->setEnabled(true);
    } else {
        lePaperWidth->setEnabled(false);
        lePaperHeight->setEnabled(false);
    }
    updatePreview();
    updatePaperPreview();
}

/**
 * Updates all unit labels that depend on the global unit.
 */
void QG_DlgOptionsDrawing::updateUnitLabels() {
    auto u = static_cast<RS2::Unit>(cbUnit->currentIndex());
    QString unitSign = RS_Units::unitToSign(u);
    auto labels = {lDimUnit1, lDimUnit2, lDimUnit3, lDimUnit4, lDimUnit5, lDimUnit6, lDimUnit7};
    for(QLabel* unitLabel : labels)
        unitLabel->setText(unitSign);
    //have to update paper size when unit changes
    updatePaperSize();
}

/**
 * Updates paper preview with specified size and margins.
 */
void QG_DlgOptionsDrawing::updatePaperPreview() {
    double paperW = RS_Math::eval(lePaperWidth->text());
    double paperH = RS_Math::eval(lePaperHeight->text());
    rbLandscape->blockSignals(true);
    if (paperW > paperH) {
        rbLandscape->setChecked(true);
    } else {
        rbPortrait->setChecked(true);
    }
    rbLandscape->blockSignals(false);
    /* Margins of preview are 5 px */
    int previewW = gvPaperPreview->width() - 10;
    int previewH = gvPaperPreview->height() - 10;
    double scale = qMin(previewW / paperW, previewH / paperH);
    int lMargin = qRound(RS_Math::eval(leMarginLeft->text(),-1) * scale);
    if (lMargin < 0.0)
        lMargin = m_graphic->getMarginLeftInUnits();
    int tMargin = qRound(RS_Math::eval(leMarginTop->text(),-1) * scale);
    if (tMargin < 0.0)
        tMargin = m_graphic->getMarginTopInUnits();
    int rMargin = qRound(RS_Math::eval(leMarginRight->text(),-1) * scale);
    if (rMargin < 0.0)
        rMargin = m_graphic->getMarginRightInUnits();
    int bMargin = qRound(RS_Math::eval(leMarginBottom->text(),-1) * scale);
    if (bMargin < 0.0)
        bMargin = m_graphic->getMarginBottomInUnits();
    int printAreaW = qRound(paperW*scale) - lMargin - rMargin;
    int printAreaH = qRound(paperH*scale) - tMargin - bMargin;
    m_paperScene->clear();
    m_paperScene->setSceneRect(0, 0, qRound(paperW*scale), qRound(paperH*scale));
    m_paperScene->addRect(0, 0, qRound(paperW*scale), qRound(paperH*scale),
                        QPen(Qt::black), QBrush(Qt::lightGray));
    m_paperScene->addRect(lMargin+1, tMargin+1, printAreaW-1, printAreaH-1,
                        QPen(Qt::NoPen), QBrush(Qt::white));
}

void QG_DlgOptionsDrawing::resizeEvent(QResizeEvent* event) {
    updatePaperPreview();
    LC_Dialog::resizeEvent(event);
}

void QG_DlgOptionsDrawing::showEvent(QShowEvent* event) {
    updatePaperPreview();
    LC_Dialog::showEvent(event);
}

// fixme - sand - review and probably remove after investigating GridSpacingX setting
void QG_DlgOptionsDrawing::on_cbGridOn_toggled(bool checked){
    rbIsoTop->setEnabled(checked);
    rbOrthogonalGrid->setEnabled(checked);
    rbIsoLeft->setEnabled(checked);
    rbIsoRight->setEnabled(checked);
    cbXSpacing->setEnabled(checked && rbOrthogonalGrid->isChecked());
    cbYSpacing->setEnabled(checked);
}

void QG_DlgOptionsDrawing::onLandscapeToggled(bool /*checked*/) {
    updatePaperSize();
}

void QG_DlgOptionsDrawing::disableXSpacing(bool checked) {
    if (checked){
        cbXSpacing->setEnabled(false);
    }
}

void QG_DlgOptionsDrawing::enableXSpacing(bool checked) {
    if (checked){
        cbXSpacing ->setEnabled(true);
    }
}

void QG_DlgOptionsDrawing::onDimFxLonToggled(bool checked) {
    cbDimFxL->setEnabled(checked);
}

void QG_DlgOptionsDrawing::onRelSizeToggled([[maybe_unused]] bool checked) {
//	RS_DEBUG->print(RS_Debug::D_ERROR,"QG_DlgOptionsDrawing::on_rbRelSize_toggled, checked = %d",checked);
    updateLPtSzUnits();
}

/*	Updates the text string for the point size units label  */
void QG_DlgOptionsDrawing::updateLPtSzUnits() {
//	RS_DEBUG->print(RS_Debug::D_ERROR,"QG_DlgOptionsDrawing::updateLPtSzUnits, rbRelSize->isChecked() = %d",rbRelSize->isChecked());
    if (rbRelSize->isChecked())
        lPtSzUnits->setText(QApplication::translate("QG_DlgOptionsDrawing", "Screen %", nullptr));
    else
        lPtSzUnits->setText(QApplication::translate("QG_DlgOptionsDrawing", "Dwg Units", nullptr));
}

void QG_DlgOptionsDrawing::showInitialTab(int tabIndex) {
    if (tabIndex > 0){
        tabWidget->setCurrentIndex(tabIndex);
    }
}
