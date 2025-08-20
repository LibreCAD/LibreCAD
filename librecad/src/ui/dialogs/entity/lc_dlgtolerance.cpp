#include "lc_dlgtolerance.h"

#include "rs_graphic.h"
#include "ui_lc_dlgtolerance.h"


namespace
{

}

LC_DlgTolerance::LC_DlgTolerance(QWidget* parent, LC_GraphicViewport* viewport, LC_Tolerance* tol, bool isNew)
    : LC_EntityPropertiesDlg(parent, QString("DlgTolerance") + (isNew ? "New" : "Edit"), viewport)
      , ui(new Ui::LC_DlgTolerance), m_entity{tol}, m_isNew{isNew}{
    ui->setupUi(this);
    initGeometricCharacterCombobox(ui->cbGeometryTop);
    initGeometricCharacterCombobox(ui->cbGeometryBottom);

    initModifiersComboBox(ui->cbModToleranceTop);
    initModifiersComboBox(ui->cbModToleranceBottom);

    initModifiersComboBox(ui->cbModDatum1Top);
    initModifiersComboBox(ui->cbModDatum2Top);
    initModifiersComboBox(ui->cbModDatum3Top);
    initModifiersComboBox(ui->cbModDatum1Bottom);
    initModifiersComboBox(ui->cbModDatum2Bottom);
    initModifiersComboBox(ui->cbModDatum3Bottom);

    ui->cbLayer->setVisible(false);
    ui->lLayer->setVisible(false);
    ui->wPen->setVisible(false);
    if (!isNew){
        setEntity(tol);
    }
}

LC_DlgTolerance::~LC_DlgTolerance(){
    delete ui;
}

void LC_DlgTolerance::setEntity(LC_Tolerance* e) {
    QString toleranceString = e->getTextCode();
    parseAndSetFields(toleranceString);
    RS_Graphic* graphic = e->getGraphic();
    if (graphic) {
        ui->cbLayer->init(*(graphic->getLayerList()), false, false);
    }
    RS_Layer* lay = e->getLayer(false);
    if (lay) {
        ui->cbLayer->setLayer(*lay);
    }
    ui->wPen->setPen(e, lay, tr("Pen"));
}

void LC_DlgTolerance::initModifiersComboBox(QComboBox* comboBox) const {
    comboBox->addItem("", "");
    comboBox->addItem(QIcon(":/gdt/modifiers/maximummaterialcondition.lci"), "", "m");
    comboBox->addItem(QIcon(":/gdt/modifiers/leastmaterialcondition.lci"), "", "l");
    comboBox->addItem(QIcon(":/gdt/modifiers/unilateral.lci"), "", "u");
    comboBox->addItem(QIcon(":/gdt/modifiers/projectedtolerancezone.lci"), "", "p");

    if (m_showExtendedModifiers) {
        comboBox->addItem(QIcon(":/gdt/modifiers/freestate.lci"), "", "f");
        comboBox->addItem(QIcon(":/gdt/modifiers/regardlessoffeaturesize.lci"), "", "s");
        comboBox->addItem(QIcon(":/gdt/modifiers/tangentplane.lci"), "", "t");

        comboBox->addItem(QIcon(":/gdt/modifiers/continuousfeature.lci"), "", "cf");
        comboBox->addItem(QIcon(":/gdt/modifiers/statisticaltolerance.lci"), "", "cf");
    }
}

void LC_DlgTolerance::initGeometricCharacterCombobox(QComboBox* comboBox) {
    comboBox->addItem("", "");
    comboBox->addItem(QIcon(":/gdt/chars/position.lci"), "", "p");
    comboBox->addItem(QIcon(":/gdt/chars/concentricity.lci"), "", "c");
    comboBox->addItem(QIcon(":/gdt/chars/symmetry.lci"), "", "s");
    comboBox->addItem(QIcon(":/gdt/chars/parallelism.lci"), "", "");
    comboBox->addItem(QIcon(":/gdt/chars/perpendicularity.lci"), "", "");
    comboBox->addItem(QIcon(":/gdt/chars/angularity.lci"), "", "");
    comboBox->addItem(QIcon(":/gdt/chars/cylindricity.lci"), "", "");
    comboBox->addItem(QIcon(":/gdt/chars/flatness.lci"), "", "");
    comboBox->addItem(QIcon(":/gdt/chars/circularity.lci"), "", "");
    comboBox->addItem(QIcon(":/gdt/chars/straightness.lci"), "", "");
    comboBox->addItem(QIcon(":/gdt/chars/profileofasurface.lci"), "", "");
    comboBox->addItem(QIcon(":/gdt/chars/profileofaline.lci"), "", "");
    comboBox->addItem(QIcon(":/gdt/chars/circular_runout.lci"), "", "");
    comboBox->addItem(QIcon(":/gdt/chars/totalrunout.lci"), "", "");
}

void LC_DlgTolerance::updateEntity() {
    QString textCode = generateDataString();
    m_entity->setTextCode(textCode);
    m_entity->setPen(ui->wPen->getPen());
    m_entity->setLayer(ui->cbLayer->getLayer());
    m_entity->update();
}

QString LC_DlgTolerance::generateDataString() {
   return "";
}

void LC_DlgTolerance::accept() {
    LC_EntityPropertiesDlg::accept();
    updateEntity();
}

void LC_DlgTolerance::parseAndSetFields([[maybe_unused]]const QString& string) {
}

void LC_DlgTolerance::languageChange() {
    ui->retranslateUi(this);
}
