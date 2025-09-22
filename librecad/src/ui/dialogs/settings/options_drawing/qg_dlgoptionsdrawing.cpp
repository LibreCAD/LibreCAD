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
#include <QMenu>
#include <QStandardItemModel>
#include <cfloat>

#include "lc_dimstyletovariablesmapper.h"
#include "lc_defaults.h"
#include "lc_dimstyleitem.h"
#include "lc_dimstylepreviewgraphicview.h"
#include "lc_dimstylepreviewpanel.h"
#include "lc_dimstylesexporter.h"
#include "lc_dimstyleslistmodel.h"
#include "lc_dimstylestreemodel.h"
#include "lc_dlgdimstylemanager.h"
#include "lc_dlgnewcustomvariable.h"
#include "lc_dlgnewdimstyle.h"
#include "lc_inputtextdialog.h"
#include "qc_applicationwindow.h"
#include "rs_debug.h"
#include "rs_filterdxfrw.h"
#include "rs_font.h"
#include "rs_math.h"
#include "rs_settings.h"
#include "rs_units.h"
#include "rs_vector.h"

#define $ENABLE_LEGACY_DIMENSIONS_TAB // fixme - sand - temporary, remove later

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

    connect(tabWidget, &QTabWidget::currentChanged, this, &QG_DlgOptionsDrawing::onTabCurrentChanged);

    connectPaperTab();
    connectUnitTab();
    connectGridTab();

#ifdef ENABLE_LEGACY_DIMENSIONS_TAB
    _to_remove_ConnectLegacyDimsTab();
#else
    tabWidget->removeTab(3);
#endif

    connectPointsTab();
    connectUserVarsTab();

    tabWidget->setCurrentIndex(0);
    init();
}

void QG_DlgOptionsDrawing::onTabCurrentChanged(int index) {
    if (index == 3) { // dimensions tab
        if (m_previewView != nullptr) {
            m_previewView->zoomAuto();
        }
    }
}

void QG_DlgOptionsDrawing::connectPointsTab() {
    connect(rbRelSize, &QRadioButton::toggled, this, &QG_DlgOptionsDrawing::onRelSizeToggled);
}

void QG_DlgOptionsDrawing::connectUserVarsTab() {
    connect(pbCustomVarAdd, &QPushButton::clicked, this, &QG_DlgOptionsDrawing::onCustomVariableAdd);
    connect(pbCustomVarDelete, &QPushButton::clicked, this, &QG_DlgOptionsDrawing::onCustomVariableDelete);
}

void QG_DlgOptionsDrawing::_to_remove_ConnectLegacyDimsTab() {
    connect(cbDimLUnit, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updateDimLengthPrecision);
    connect(cbDimAUnit, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updateDimAnglePrecision);
    connect(cbDimFxLon, &QCheckBox::toggled, this, &QG_DlgOptionsDrawing::onDimFxLonToggled);
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

void QG_DlgOptionsDrawing::connectPaperTab() {
    connect(cbPaperFormat, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updatePaperSize);
    connect(lePaperWidth, &QLineEdit::textChanged, this, &QG_DlgOptionsDrawing::updatePaperPreview);
    connect(lePaperHeight, &QLineEdit::textChanged, this, &QG_DlgOptionsDrawing::updatePaperPreview);
    connect(rbLandscape, &QRadioButton::toggled, this, &QG_DlgOptionsDrawing::onLandscapeToggled);
    connect(leMarginTop, &QLineEdit::textChanged, this, &QG_DlgOptionsDrawing::updatePaperPreview);
    connect(leMarginBottom, &QLineEdit::textChanged, this, &QG_DlgOptionsDrawing::updatePaperPreview);
    connect(leMarginRight, &QLineEdit::textChanged, this, &QG_DlgOptionsDrawing::updatePaperPreview);
    connect(leMarginLeft, &QLineEdit::textChanged, this, &QG_DlgOptionsDrawing::updatePaperPreview);
}

void QG_DlgOptionsDrawing::connectUnitTab() {
    connect(cbUnit, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updateUnitsPreview);
    connect(cbLengthFormat, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updateLengthPrecision);
    connect(cbLengthFormat, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updateUnitsPreview);
    connect(cbLengthPrecision, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updateUnitsPreview);
    connect(cbAngleFormat, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updateAnglePrecision);
    connect(cbAngleFormat, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updateUnitsPreview);
    connect(cbAnglePrecision, &QComboBox::activated, this, &QG_DlgOptionsDrawing::updateUnitsPreview);
}

void QG_DlgOptionsDrawing::connectGridTab() {
    connect(rbIsoLeft, &QCheckBox::toggled, this, &QG_DlgOptionsDrawing::disableXSpacing);
    connect(rbIsoRight, &QCheckBox::toggled, this, &QG_DlgOptionsDrawing::disableXSpacing);
    connect(rbIsoTop, &QCheckBox::toggled, this, &QG_DlgOptionsDrawing::disableXSpacing);
    connect(rbOrthogonalGrid,  &QCheckBox::toggled, this, &QG_DlgOptionsDrawing::enableXSpacing);
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

    fillLinearUnitsCombobox(cbLengthFormat);
    fillAngleUnitsCombobox(cbAngleFormat);
#ifdef ENABLE_LEGACY_DIMENSIONS_TAB
    fillLinearUnitsCombobox(cbDimLUnit);
    fillAngleUnitsCombobox(cbDimAUnit);
#endif

    // Paper format:
    for (RS2::PaperFormat i = RS2::FirstPaperFormat; RS2::NPageFormat > i; i = static_cast<RS2::PaperFormat>(i + 1)) {
        cbPaperFormat->addItem( RS_Units::paperFormatToString( i));
    }
    // Paper preview:
    gvPaperPreview->setScene(m_paperScene);
    gvPaperPreview->setBackgroundBrush(this->palette().color(QPalette::Window));
#ifdef ENABLE_LEGACY_DIMENSIONS_TAB
    cbDimTxSty->init();
#endif
}

#define TO_MM(v) RS_Units::convert(v, RS2::Millimeter, unit)

void QG_DlgOptionsDrawing::prepareDimStyleItems(QList<LC_DimStyleItem*> &items) {
    QString defaultDimStyleName = m_graphic->getDefaultDimStyleName();
    LC_DimStyle* styleThatIsDefault = m_graphic->getDimStyleByName(defaultDimStyleName);
    QMap<QString, int> usages;
    auto dimStylesList = m_graphic->getDimStyleList();
    auto dimStyles = dimStylesList->getStylesList();
    collectStylesUsage(usages);

    for (const auto dimStyle : *dimStyles) {
        LC_DimStyle* ds = dimStyle->getCopy();
        int usageCount = usages.value(dimStyle->getName(), 0);
        auto item = new LC_DimStyleItem(ds, usageCount, styleThatIsDefault == dimStyle);
        items << item;
    }
}

void QG_DlgOptionsDrawing::setupDimStylesTab() {
    m_previewView = LC_DimStylePreviewGraphicView::init(this, m_graphic, RS2::EntityUnknown);

    auto* layout = new QVBoxLayout(gbDimStylesPreview);
    layout->setContentsMargins(0, 0, 0, 0);

    gbDimStylesPreview->setLayout(layout);
    layout->addWidget(m_previewView);

    auto previewToolbar = new LC_DimStylePreviewPanel(gbDimStylesPreview);
    previewToolbar->setGraphicView(m_previewView);

    layout->addWidget(previewToolbar);
    layout->addWidget(m_previewView, 10);

    QList<LC_DimStyleItem*> items;
    prepareDimStyleItems(items);

    auto* model = new LC_DimStyleTreeModel(this, items, true);
    lvDimStyles->setModel(model);

    updateActiveStyleLabel(model);

    expandStylesTree();

    connect(tbDimNew, &QToolButton::clicked, this, &QG_DlgOptionsDrawing::onDimStyleNew);
    connect(tbDimEdit, &QToolButton::clicked, this, &QG_DlgOptionsDrawing::onDimStyleEdit);
    connect(tbDimRename, &QToolButton::clicked, this, &QG_DlgOptionsDrawing::onDimStyleRename);
    connect(tbDimRemove, &QToolButton::clicked, this, &QG_DlgOptionsDrawing::onDimStyleRemove);
    connect(tbDimExport, &QToolButton::clicked, this, &QG_DlgOptionsDrawing::onDimStyleExport);
    connect(tbDimImport, &QToolButton::clicked, this, &QG_DlgOptionsDrawing::onDimStyleImport);
    connect(tbDimDefault, &QToolButton::clicked, this, &QG_DlgOptionsDrawing::onDimStyleSetDefault);

    bool autoRaiseButtons = LC_GET_ONE_BOOL("Widgets", "DockWidgetsFlatIcons", true);
    tbDimNew->setAutoRaise(autoRaiseButtons);
    tbDimEdit->setAutoRaise(autoRaiseButtons);
    tbDimRename->setAutoRaise(autoRaiseButtons);
    tbDimRemove->setAutoRaise(autoRaiseButtons);
    tbDimExport->setAutoRaise(autoRaiseButtons);
    tbDimImport->setAutoRaise(autoRaiseButtons);
    tbDimDefault->setAutoRaise(autoRaiseButtons);

    lvDimStyles->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(lvDimStyles->selectionModel(), &QItemSelectionModel::currentChanged, this,
            &QG_DlgOptionsDrawing::onDimCurrentChanged);

    QModelIndex indexOfSelected = model->index(0, 0, QModelIndex());
    lvDimStyles->setCurrentIndex(indexOfSelected);

    connect(lvDimStyles, &QListView::customContextMenuRequested, this,
            &QG_DlgOptionsDrawing::onDimStylesListMenuRequested);
    connect(lvDimStyles, &QListView::doubleClicked, this, &QG_DlgOptionsDrawing::onDimStyleDoubleClick);

    // fixme - sand - hide for now, so far all blocks are created by default. Later, only requried blocks may be included
    // yet this is also related to unused elements purging, so rework this later.

    dbDimEmbeddArrowBlocks->setVisible(false);
}

void QG_DlgOptionsDrawing::collectStylesUsage(QMap<QString, int>& map) {
    for (RS_Entity* e : m_graphic->getEntityList()) {
        auto entityType = e->rtti();
        if (!e->isUndone() && RS2::isDimensionalEntity(entityType)) {
            auto* dim = dynamic_cast<RS_Dimension*>(e);
            QString styleName = dim->getStyle();

            auto dimStyleForNameAndType = m_graphic->getDimStyleByName(styleName, entityType);
            if (dimStyleForNameAndType != nullptr) {
                QString resolvedStyleName = dimStyleForNameAndType->getName();
                int value = map.value(resolvedStyleName, 0);
                value++;
                map[resolvedStyleName] = value;
            }
            else {
               // weird case - style is referenced in entity, but is not present in dim styles... looks like DXF error
            }
        }
    }
}

void QG_DlgOptionsDrawing::setupMetaTab() {
    QString title = m_graphic->getVariableString("$TITLE", "");
    QString subject = m_graphic->getVariableString("$SUBJECT", "");
    QString author = m_graphic->getVariableString("$AUTHOR", "");
    QString keywords = m_graphic->getVariableString("$KEYWORDS", "");
    QString comments = m_graphic->getVariableString("$COMMENTS", "");

    leMetaTitle->setText(title);
    leMetaSubject->setText(subject);
    leMetaAuthor->setText(author);
    leMetaKeywords->setText(keywords);
    leMetaComments->setText(comments);
}

void QG_DlgOptionsDrawing::setupUserREditor(QLineEdit* edit, const QString &key) {
    double rval = m_graphic->getVariableDouble(key, 0.0);
    QString val = QString::number(rval, 'g', 12);
    edit->setText(val);
}

void QG_DlgOptionsDrawing::setupUserTab() {
    sbUserI1->setValue(m_graphic->getVariableInt("$USERI1", 0));
    sbUserI2->setValue(m_graphic->getVariableInt("$USERI2", 0));
    sbUserI3->setValue(m_graphic->getVariableInt("$USERI3", 0));
    sbUserI4->setValue(m_graphic->getVariableInt("$USERI4", 0));
    sbUserI5->setValue(m_graphic->getVariableInt("$USERI5", 0));

    setupUserREditor(leUserR1, "$USERR1");
    setupUserREditor(leUserR2, "$USERR2");
    setupUserREditor(leUserR3, "$USERR3");
    setupUserREditor(leUserR4, "$USERR4");
    setupUserREditor(leUserR5, "$USERR5");

    twCustomVars->setColumnCount(2);
    twCustomVars->setHorizontalHeaderLabels({tr("Name"), tr("Value")});

    auto customVars = m_graphic->getCustomProperties();
    int row = 0;
    QHashIterator<QString,RS_Variable> it(customVars);
    while (it.hasNext()){
        it.next();
        QString key = it.key();
        QString value = it.value().getString();
        twCustomVars->insertRow(row);
        auto keyItem = new QTableWidgetItem(key);
        keyItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        twCustomVars->setItem(row,0, keyItem);
        twCustomVars->setItem(row,1, new QTableWidgetItem(value));
        row++;
    }
    if (row == 0) {
        pbCustomVarDelete->setEnabled(false);
    }
    // twCustomVars->resizeColumnsToContents();
    twCustomVars->horizontalHeader()->setStretchLastSection(true);
    twCustomVars->setSelectionBehavior(QAbstractItemView::SelectRows);
    twCustomVars->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    twCustomVars->setSortingEnabled(true);
    twCustomVars->verticalHeader()->hide();
}

void QG_DlgOptionsDrawing::onCustomVariableAdd([[maybe_unused]]bool checked) {
    LC_DlgNewCustomVariable dlg(this);
    QStringList propertyNames;
    int rowCount = twCustomVars->rowCount();
    for (int row = 0; row < rowCount; row++) {
        QString propertyName = twCustomVars->item(row, 0)->text();
        propertyNames.push_back(propertyName);
    }
    dlg.setPropertyNames(&propertyNames);
    if (dlg.exec() == QDialog::Accepted) {
        QString propertyName = dlg.getPropertyName();
        QString propertyValue = dlg.getPropertyValue();
        int rowCount = twCustomVars->rowCount();
        twCustomVars->insertRow(rowCount);
        twCustomVars->setItem(rowCount,0, new QTableWidgetItem(propertyName));
        twCustomVars->setItem(rowCount,1, new QTableWidgetItem(propertyValue));
        pbCustomVarDelete->setEnabled(true);
    }
    // twCustomVars->resizeColumnsToContents();
    // twCustomVars->horizontalHeader()->setStretchLastSection(true);
    // twCustomVars->horizontalHeader()->setSectionR    esizeMode(QHeaderView::Stretch);
    twCustomVars->update();
}

void QG_DlgOptionsDrawing::onCustomVariableDelete([[maybe_unused]]bool checked) {
    int row = twCustomVars->currentRow();
    if (row >= 0) {
        QString propertyName = twCustomVars->item(row, 0)->text();
        int response = QMessageBox::warning(
                   this,
                   tr("Delete Custom Property"),
                   tr("Are you sure you'd like to delete property [%1]?").arg(propertyName),
                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No
                   );

        if (response != QMessageBox::Yes) {
            return;
        }
        twCustomVars->removeRow(row);
    }
    if (twCustomVars->rowCount() == 0) {
        pbCustomVarDelete->setEnabled(false);
    }
}

void QG_DlgOptionsDrawing::onDimStyleDoubleClick() {
    onDimStyleEdit(false);
}

void QG_DlgOptionsDrawing::reject() {
    if (m_hasImportantModificationsToAskOnCancel) {
        int response = QMessageBox::warning(
                   this,
                   tr("Drawing Options"),
                   tr("Settings were changed. Are you sure you'd like to skip saving changes (so they will not be saved)?"),
                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No
                   );

        if (response != QMessageBox::Yes) {
            return;
        }
    }
    LC_Dialog::reject();
}

LC_DimStyleTreeModel* QG_DlgOptionsDrawing::getDimStylesModel() {
    return dynamic_cast<LC_DimStyleTreeModel*>(lvDimStyles->model());
}

void QG_DlgOptionsDrawing::onDimStylesListMenuRequested(const QPoint& pos) {
    auto* contextMenu = new QMenu(this);
    // auto* caption = new QLabel(tr("Dim Styles Menu"), this);
    /*QPalette palette;
    palette.setColor(caption->backgroundRole(), RS_Color(0, 0, 0));
    palette.setColor(caption->foregroundRole(), RS_Color(255, 255, 255));
    caption->setPalette(palette);
    caption->setAlignment(Qt::AlignCenter);*/

    QModelIndex index = lvDimStyles->indexAt(pos);
    using ActionMemberFunc = void (QG_DlgOptionsDrawing::*)(bool);
    const auto addActionFunc = [this, &contextMenu](const QString& iconName, const QString& name, ActionMemberFunc func)
    {
        contextMenu->addAction(QIcon(":/icons/" + iconName + ".lci"), name, this, func);
    };

    if (index.isValid()) {
        auto* model = getDimStylesModel();
        LC_DimStyleItem* item = model->getItemForIndex(index);
        if (!item->isActive() && item->isBaseStyle()) {
            addActionFunc("dim_default", tr("&Set as Active"), &QG_DlgOptionsDrawing::onDimStyleSetDefault);
        }

        addActionFunc("add", tr("&Create Style"), &QG_DlgOptionsDrawing::onDimStyleNew);
        contextMenu->addSeparator();
        addActionFunc("attributes", tr("&Edit Style"), &QG_DlgOptionsDrawing::onDimStyleEdit);

        if (item->isNotUsedInDrawing()) {
            if (item->isBaseStyle()) {
                addActionFunc("rename_active_block", tr("&Rename Style"), &QG_DlgOptionsDrawing::onDimStyleRename);
            }
            addActionFunc("remove", tr("&Delete Style"), &QG_DlgOptionsDrawing::onDimStyleRemove);
        }

        contextMenu->addSeparator();
        addActionFunc("export", tr("E&xport Styles"), &QG_DlgOptionsDrawing::onDimStyleExport);
        addActionFunc("import", tr("&Import Styles"), &QG_DlgOptionsDrawing::onDimStyleImport);
    }
    else {
        addActionFunc("add", tr("&Create Style"), &QG_DlgOptionsDrawing::onDimStyleNew);
        addActionFunc("import", tr("&Import Styles"), &QG_DlgOptionsDrawing::onDimStyleImport);
    }
    contextMenu->exec(QCursor::pos());
    delete contextMenu;
}

QString QG_DlgOptionsDrawing::askForUniqueDimStyleName(const QString &caption, const QString &prompt, const QString &defaultText) {
    bool ok;
    bool styleNameNotUnique = false;
    QString styleName = "";
    QString value = defaultText;
    auto* model = getDimStylesModel();
    int i = 1;
    do {
        styleName = LC_InputTextDialog::getText(this, caption,prompt, {}, true, value, &ok);
        styleName = styleName.trimmed();
        if (ok) {
            auto foundStyle = model->findByName(styleName);
            styleNameNotUnique = foundStyle != nullptr;
            if (styleNameNotUnique) {
                value = styleName+"_"+QString::number(i);
                i++;
            }
        }
        else {
            return "";
        }
    }
    while (styleNameNotUnique);
    return styleName;
}

void QG_DlgOptionsDrawing::onDimStyleNew([[maybe_unused]]bool checked) {
    QModelIndex selectedItemIndex = getSelectedDimStyleIndex();
    auto* model = getDimStylesModel();
    LC_DimStyleItem* defaultItem = nullptr;
    if (selectedItemIndex.isValid()) {
        defaultItem = model->getItemForIndex(selectedItemIndex);
    }
    else {
        defaultItem = model->getActiveStyleItem();
        if (defaultItem == nullptr) {
            defaultItem = model->getStandardItem();
        }
    }

    QString newStyleName  = "";
    LC_DlgNewDimStyle dlgNewDimStyle(this);
    QList<LC_DimStyleItem*> styleItems;
    model->collectAllStyleItems(styleItems);
    dlgNewDimStyle.setup(defaultItem, styleItems);
    if (dlgNewDimStyle.exec() == Accepted) {
        newStyleName = dlgNewDimStyle.getStyleName();
        auto styleItemBaseOn = dlgNewDimStyle.getBaseDimStyle();
        RS2::EntityType newDimType = dlgNewDimStyle.getDimensionType();

        doCreateDimStyle(newStyleName, model, styleItemBaseOn, newDimType);
    }
}

void QG_DlgOptionsDrawing::expandStylesTree() {
    lvDimStyles->expandAll();
    lvDimStyles->setItemsExpandable(false);
}

void QG_DlgOptionsDrawing::doCreateDimStyle(const QString &newStyleName, LC_DimStyleTreeModel* model, LC_DimStyleItem* styleItemBasedOn, RS2::EntityType newDimType) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    auto originalStyle = styleItemBasedOn->dimStyle();

    // prepare copy used during editing
    LC_DimStyle* styleCopyToEdit = originalStyle->getCopy();
    styleCopyToEdit->setName(newStyleName);
    styleCopyToEdit->setFromVars(false);
    styleCopyToEdit->resetFlags(true);

    LC_DlgDimStyleManager dimStyleManager(this, styleCopyToEdit, m_graphic, newDimType);

    // add relevant styles to the dialog
    QList<LC_DimStyleItem*> itemsMatchedStyle;
    model->collectAllItemsForStyle(styleCopyToEdit, &itemsMatchedStyle);
    for (auto dsi: itemsMatchedStyle) {
        auto styleToAdd = dsi->dimStyle();
        if (styleToAdd->getName() != newStyleName) {
            dimStyleManager.addDimStyle(styleToAdd);
        }
    }

    dimStyleManager.setWindowTitle(tr("Style to Create - ") + LC_DimStyleItem::getDisplayDimStyleName(styleCopyToEdit));
    QApplication::restoreOverrideCursor();
    dimStyleManager.refreshPreview();

    if (dimStyleManager.exec() == QDialog::Accepted) {
        auto item = new LC_DimStyleItem(styleCopyToEdit, 0, false);
        model->addItem(item);
        expandStylesTree();
        updateDimStylePreview(originalStyle, model);
        m_hasImportantModificationsToAskOnCancel = true;
    }
    else {
        delete styleCopyToEdit;
    }
}

void QG_DlgOptionsDrawing::onDimStyleEdit([[maybe_unused]]bool checked) {
    QModelIndex selectedItemIndex = getSelectedDimStyleIndex();
    if (selectedItemIndex.isValid()) {
        auto model = getDimStylesModel();
        LC_DimStyleItem* itemToEdit = model->getItemForIndex(selectedItemIndex);

        auto originalStyleToEdit = itemToEdit->dimStyle();

        QString baseName = itemToEdit->baseName();
        RS2::EntityType dimType = itemToEdit->forDimensionType();

        LC_DimStyle* styleCopyToEdit =  originalStyleToEdit->getCopy();

        if (dimType != RS2::EntityUnknown) {
            // this is substyle for specific dim type
            auto baseStyleItem = model->findByName(baseName);
            if (baseStyleItem != nullptr) {
                LC_DimStyle* baseStyle = baseStyleItem->dimStyle();
                if (baseStyle != nullptr) {
                    styleCopyToEdit->mergeWith(baseStyle, LC_DimStyle::ModificationAware::UNSET, LC_DimStyle::ModificationAware::SET);
                }
            }
        }

        QApplication::setOverrideCursor(Qt::WaitCursor);
        LC_DlgDimStyleManager dimStyleManager(this, styleCopyToEdit, m_graphic, dimType);
        QList<LC_DimStyleItem*> itemsMatchedStyle;
        model->collectItemsForBaseStyleName(baseName, &itemsMatchedStyle);
        for (auto dsi: itemsMatchedStyle) {
            auto styleToAdd = dsi->dimStyle();
            if (itemToEdit != dsi){
                dimStyleManager.addDimStyle(styleToAdd);
            }
        }

        dimStyleManager.refreshPreview();
        dimStyleManager.setWindowTitle(tr("Dimension style editing - ") + LC_DimStyleItem::getDisplayDimStyleName(originalStyleToEdit));
        QApplication::restoreOverrideCursor();

        if (dimStyleManager.exec() == QDialog::Accepted) {
            styleCopyToEdit->copyTo(originalStyleToEdit);
            m_hasImportantModificationsToAskOnCancel = true;
            updateDimStylePreview(originalStyleToEdit, model);
        };
        delete styleCopyToEdit;
    }
}

QModelIndex QG_DlgOptionsDrawing::getSelectedDimStyleIndex() {
    return lvDimStyles->selectionModel()->currentIndex();
}

void QG_DlgOptionsDrawing::onDimStyleRename([[maybe_unused]]bool checked) {
    QModelIndex selectedItemIndex = getSelectedDimStyleIndex();
    if (selectedItemIndex.isValid()) {
        auto* model = getDimStylesModel();
        LC_DimStyleItem* item = model->getItemForIndex(selectedItemIndex);
        if (item->isBaseStyle() && item->isNotUsedInDrawing()) {
            auto style = item->dimStyle();
            QString originalStyleName = style->getName();
            QString styleName = askForUniqueDimStyleName(tr("Rename Dimension Style"),
            tr("Enter new unique name of dimension style (was \"%1\"):").arg(originalStyleName), originalStyleName);
            if (!styleName.isEmpty()) {
               item->setNewBaseName(styleName);
            }
            model->emitDataChanged();
            expandStylesTree();
            m_hasImportantModificationsToAskOnCancel = true;
        }
    }
}


void QG_DlgOptionsDrawing::onDimStyleRemove([[maybe_unused]]bool checked) {
    QModelIndex selectedItemIndex = getSelectedDimStyleIndex();
    if (selectedItemIndex.isValid()) {
        auto* model = getDimStylesModel();
        LC_DimStyleItem* item = model->getItemForIndex(selectedItemIndex);
        int itemsCount = model->itemsCount();
        if (itemsCount == 1) {
            QMessageBox msgBox(QMessageBox::Critical,
                              tr("Removing Dimension Style"),
                              tr("Can't delete last dimension style. At least one should be present! "),
                              QMessageBox::Ok);
            return;
        }

        if (item->usageCount() == 0) {
            QString styleName = item->dimStyle()->getName();
            bool allowRemoval = false;

            if (item->childCount() > 0) {
                bool hasUsedChildren = item->hasUsedChildren();
                if (hasUsedChildren) {
                    QMessageBox msgBox(QMessageBox::Critical,
                              tr("Removing Dimension Style"),
                              tr("Can't delete dimension style as it's children is used in drawing. Only unused style may be deleted."),
                              QMessageBox::Ok);
                    allowRemoval = false;
                }
                else {
                    QMessageBox msgBox(QMessageBox::Warning,
                                       tr("Removing Dimension Style"),
                                       tr("Are you sure you want to remove the dimension style \"%1\" together with child styles?").arg(styleName),
                                       QMessageBox::Yes | QMessageBox::No);
                    allowRemoval = msgBox.exec() == QMessageBox::Yes;
                }
            }
            else {
                QMessageBox msgBox(QMessageBox::Warning,
                                   tr("Removing Dimension Style"),
                                   tr("Are you sure you want to remove the dimension style \"%1\"?").arg(styleName),
                                   QMessageBox::Yes | QMessageBox::No);
                allowRemoval = msgBox.exec() == QMessageBox::Yes;
            }

            if (allowRemoval) {
                model->removeItem(item);
                expandStylesTree();
                m_hasImportantModificationsToAskOnCancel = true;
            }
        }
    }
}

void QG_DlgOptionsDrawing::onDimStyleExport([[maybe_unused]]bool checked) {
    LC_DimStylesExporter exporter;
    // here we export styles from the dialog, not from the document!!!
    auto model = getDimStylesModel();
    QList<LC_DimStyleItem*> dimStyleItems;
    model->collectAllStyleItems(dimStyleItems);
    exporter.exportStyles(this, dimStyleItems, m_graphic->getFilename());
}

void QG_DlgOptionsDrawing::onDimStyleImport([[maybe_unused]]bool checked) {
    LC_DimStylesExporter exporter;
    QList<LC_DimStyle*> items;
    if (exporter.importStyles(this, items)) {
        auto model = getDimStylesModel();
        model->mergeWith(items);
        items.clear();
        QModelIndex selectedItemIndex = getSelectedDimStyleIndex();
        LC_DimStyleItem* itemToRefresh{nullptr};
        if (selectedItemIndex.isValid()) {
            itemToRefresh = model->getItemForIndex(selectedItemIndex);
        }
        else{
            itemToRefresh = model->getStandardItem();
        }
        m_hasImportantModificationsToAskOnCancel= true;
        updateDimStylePreview(itemToRefresh->dimStyle(), model);
    }
}

void QG_DlgOptionsDrawing::updateActiveStyleLabel(LC_DimStyleTreeModel* model) {
    auto activeStyleItem = model->getActiveStyleItem();
    QString styleName = "";
    if (activeStyleItem != nullptr){
        styleName = activeStyleItem->baseName();
    }
    lblActiveStyle->setText(styleName);
}

void QG_DlgOptionsDrawing::onDimStyleSetDefault([[maybe_unused]]bool checked) {
    QModelIndex selectedItemIndex = getSelectedDimStyleIndex();
    if (selectedItemIndex.isValid()) {
        auto model = getDimStylesModel();
        model->setActiveStyleItem(selectedItemIndex);
        expandStylesTree();
        updateActiveStyleLabel(model);
    }
}

void QG_DlgOptionsDrawing::updateActionButtons(LC_DimStyleItem* item) {
    auto model = getDimStylesModel();
    int itemsCount = model->itemsCount();
    tbDimDefault->setEnabled(!item->isActive() && item->isBaseStyle());
    bool notUsed = item->isNotUsedInDrawing();
    tbDimRemove->setEnabled(itemsCount > 1 && notUsed);
    // fixme - sand - dims - should we allow renaming if there are usages?
    tbDimRename->setEnabled(item->isBaseStyle() && notUsed);
}

void QG_DlgOptionsDrawing::onDimCurrentChanged(const QModelIndex &current, [[maybe_unused]]const QModelIndex &previous){
    if (current.isValid()) {
        auto model = getDimStylesModel();
        LC_DimStyleItem* item = model->getItemForIndex(current);
        updateActionButtons(item);
        updateDimStylePreview(item->dimStyle(), model);
    }
}

void QG_DlgOptionsDrawing::updateDimStylePreview(LC_DimStyle* dimStyle, LC_DimStyleTreeModel* model) const {
    m_previewView->setDimStyle(dimStyle);
    QList<LC_DimStyleItem*> itemsMatchedStyle;
    model->collectAllItemsForStyle(dimStyle, &itemsMatchedStyle);
    QString dimStyleName = dimStyle->getName();
    for (auto dsi: itemsMatchedStyle) {
        auto styleToAdd = dsi->dimStyle();
        if (styleToAdd->getName() != dimStyleName) {
            m_previewView->addDimStyle(styleToAdd);
        }
    }
    m_previewView->updateDims();
    m_previewView->refresh();
}

void QG_DlgOptionsDrawing::setupPointsTab() {
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
}

void QG_DlgOptionsDrawing::setupSplinesTab() {
    // spline line segments per patch:
    int splinesegs = m_graphic->getVariableInt("$SPLINESEGS", 8);
    //RLZ    cbSplineSegs->setCurrentText(QString("%1").arg(splinesegs));
    cbSplineSegs->setEditText(QString("%1").arg(splinesegs));

    RS_DEBUG->print("QG_DlgOptionsDrawing::setGraphic: splinesegs is: %d",
                    splinesegs);
    int lineCaps = m_graphic->getGraphicVariableInt("$ENDCAPS", 1);
    cbLineCap->setCurrentIndex(lineCaps);

    int joinStyle = m_graphic->getGraphicVariableInt("$JOINSTYLE", 1);
    cbLineJoin ->setCurrentIndex(joinStyle);
}

void QG_DlgOptionsDrawing::setupGridTab() {
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
}

void QG_DlgOptionsDrawing::setupPaperTab() {
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
        auto widthStr = QString("%1").setNum(s.x, 'g', 5);
        auto heightStr = QString("%1").setNum(s.y, 'g', 5);

        lePaperWidth->blockSignals(true);
        lePaperHeight->blockSignals(true);
        lePaperWidth->setText(widthStr);
        lePaperHeight->setText(heightStr);
        lePaperWidth->blockSignals(false);
        lePaperHeight->blockSignals(false);

        lePaperWidth->setEnabled(true);
        lePaperHeight->setEnabled(true);
    } else {
        lePaperWidth->setEnabled(false);
        lePaperHeight->setEnabled(false);
    }

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
}

void QG_DlgOptionsDrawing::_toRemoveSetupLegacyDimsTab(RS2::LinearFormat& linearFormat, int lunits, int luprec, int aunits, int auprec) {
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

    linearFormat = linearFormatFromUI(cbDimLUnit->currentIndex());
    updateLengthPrecisionCombobox(linearFormat, cbDimDec);
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
    updateAnglePrecisionCombobox(angleFormatFromUI(dimaunit), cbDimADec);
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
}

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

    RS2::LinearFormat linearFormat = linearFormatFromUI(cbLengthFormat->currentIndex());
    updateLengthPrecisionCombobox(linearFormat, cbLengthPrecision);
    cbLengthPrecision->setCurrentIndex(luprec);

    // units / angle format:
    int aunits = m_graphic->getVariableInt("$AUNITS", 0);
    cbAngleFormat->setCurrentIndex(aunits);

    // units angle precision:
    int auprec = m_graphic->getVariableInt("$AUPREC", 2);
    updateAnglePrecisionCombobox(angleFormatFromUI(aunits), cbAnglePrecision);
    cbAnglePrecision->setCurrentIndex(auprec);

    // angles system setup
    double baseAngle = m_graphic->getAnglesBase();
    leAngleBaseZero->setText(QString("%1").arg(RS_Math::rad2deg(baseAngle)));

    bool anglesAreCounterClockwise = m_graphic->areAnglesCounterClockWise();
    rbAngleBasePositive->setChecked(anglesAreCounterClockwise);
    rbAngleBaseNegative->setChecked(!anglesAreCounterClockwise);

    setupGridTab();

#ifdef ENABLE_LEGACY_DIMENSIONS_TAB
    _toRemoveSetupLegacyDimsTab(linearFormat, lunits, luprec, aunits, auprec);
#endif

    // encoding:
    /*
    QString encoding = graphic->getVariableString("$DWGCODEPAGE",
                                                  "ANSI_1252");
    encoding=RS_System::getEncoding(encoding);
    cbEncoding->setEditText(encoding);
    */

    setupPaperTab();
    setupPointsTab();
    setupSplinesTab();
    setupDimStylesTab();
    setupMetaTab();
    setupUserTab();
    setupVariablesTab();
}

void QG_DlgOptionsDrawing::setupVariablesTab(){
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
        auto *valueItem = new QTableWidgetItem(str);
        tabVariables->setItem(row, 3, valueItem);
        row++;
        ++it;
    }
    tabVariables->sortByColumn(0, Qt::SortOrder::AscendingOrder);
    tabVariables->setSortingEnabled(true);
}

void QG_DlgOptionsDrawing::_toRemove_validateDimsOld() {
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
    m_graphic->updateDimensions(ok1);
}

bool QG_DlgOptionsDrawing::validateDimensionsTab() {
    LC_DimStyleTreeModel* model = getDimStylesModel();
    QList<LC_DimStyleItem*> items;
    model->collectAllStyleItems(items);

    QList<LC_DimStyle*> newDimStyles;
    QString currentDimStyleName;
    for (LC_DimStyleItem* item: items) {
        auto editedStyle = item->dimStyle();
        newDimStyles.push_back(editedStyle->getCopy());
        if (item->isActive()) {
           currentDimStyleName = editedStyle->getName();
           m_graphic->updateFallbackDimStyle(editedStyle);
        }
    }

    m_graphic->replaceDimStylesList(currentDimStyleName, newDimStyles);

    // dimstyles will be set to graphic, so don't delete them
    model->cleanup(false);
    m_graphic->updateDimensions(false);
    return true;
}

bool QG_DlgOptionsDrawing::validatePointsTab() {
    // Points drawing style:
    // Get currently selected point style from which button is checked
    int pdmode = LC_DEFAULTS_PDMode;

    if (bDot->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_CentreDot;
    }
    else if (bBlank->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_CentreBlank;
    }
    else if (bPlus->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_CentrePlus;
    }
    else if (bCross->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_CentreCross;
    }
    else if (bTick->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_CentreTick;
    }
    else if (bDotCircle->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreDot);
    }
    else if (bBlankCircle->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreBlank);
    }
    else if (bPlusCircle->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentrePlus);
    }
    else if (bCrossCircle->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreCross);
    }
    else if (bTickCircle->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseCircle(DXF_FORMAT_PDMode_CentreTick);
    }
    else if (bDotSquare->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreDot);
    }
    else if (bBlankSquare->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreBlank);
    }
    else if (bPlusSquare->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentrePlus);
    }
    else if (bCrossSquare->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreCross);
    }
    else if (bTickSquare->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseSquare(DXF_FORMAT_PDMode_CentreTick);
    }
    else if (bDotCircleSquare->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreDot);
    }
    else if (bBlankCircleSquare->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreBlank);
    }
    else if (bPlusCircleSquare->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentrePlus);
    }
    else if (bCrossCircleSquare->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreCross);
    }
    else if (bTickCircleSquare->isChecked()) {
        pdmode = DXF_FORMAT_PDMode_EncloseCircleSquare(DXF_FORMAT_PDMode_CentreTick);
    }

    m_graphic->addVariable("$PDMODE", pdmode, DXF_FORMAT_GC_PDMode);

    bool ok{false};
    double pdsize = RS_Math::eval(lePointSize->text(), &ok);
    if (!ok) {
        pdsize = LC_DEFAULTS_PDSize;
    }

    if (pdsize > 0.0 && rbRelSize->isChecked()) {
        pdsize = -pdsize;
    }

    m_graphic->addVariable("$PDSIZE", pdsize, DXF_FORMAT_GC_PDSize);
    return true;
}

void QG_DlgOptionsDrawing::validateSplinesTab() {
    // splines:
    m_graphic->addVariable("$SPLINESEGS",(int) RS_Math::eval(cbSplineSegs->currentText()), 70);

    RS_DEBUG->print("QG_DlgOptionsDrawing::validate: splinesegs is: %s",
                    cbSplineSegs->currentText().toLatin1().data());

    m_graphic->addVariable("$JOINSTYLE", cbLineJoin ->currentIndex(), DXF_FORMAT_GC_JoinStyle);
    m_graphic->addVariable("$ENDCAPS", cbLineCap->currentIndex(), DXF_FORMAT_GC_Endcaps);
    // update all spline entities in the graphic to match the new settings:
    m_graphic->updateSplines();
}

void QG_DlgOptionsDrawing::validateGridTab() {
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
}

void QG_DlgOptionsDrawing::validatePaperTab() {
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
}

void QG_DlgOptionsDrawing::validateMetaTab() {
    m_graphic->addVariable("$TITLE", leMetaTitle->text(), 1);
    m_graphic->addVariable("$SUBJECT", leMetaSubject->text(), 1);
    m_graphic->addVariable("$AUTHOR", leMetaAuthor->text(), 1);
    m_graphic->addVariable("$KEYWORDS", leMetaKeywords->text(), 1);
    m_graphic->addVariable("$COMMENTS", leMetaComments->toPlainText(), 1);
}

void QG_DlgOptionsDrawing::validateUserTab() {
    m_graphic->addVariable("$USERI1", sbUserI1->value(),70);
    m_graphic->addVariable("$USERI2", sbUserI2->value(),70);
    m_graphic->addVariable("$USERI3", sbUserI3->value(),70);
    m_graphic->addVariable("$USERI4", sbUserI4->value(),70);
    m_graphic->addVariable("$USERI5", sbUserI5->value(),70);

    m_graphic->addVariable("$USERR1", RS_Math::eval(leUserR1->text(),0.0),40);
    m_graphic->addVariable("$USERR2", RS_Math::eval(leUserR2->text(),0.0),40);
    m_graphic->addVariable("$USERR3", RS_Math::eval(leUserR3->text(),0.0),40);
    m_graphic->addVariable("$USERR4", RS_Math::eval(leUserR4->text(),0.0),40);
    m_graphic->addVariable("$USERR5", RS_Math::eval(leUserR5->text(),0.0),40);

    int row_count = twCustomVars->rowCount();

    QHash<QString, QString> newCustomVars;
    for (int i = 0; i < row_count; i++) {
        QString name = twCustomVars->item(i, 0)->text();
        QString value = twCustomVars->item(i, 1)->text();
        newCustomVars.insert(name, value);
    }
    m_graphic->replaceCustomVars(newCustomVars);
}

void QG_DlgOptionsDrawing::validateUnitsTab() {
    // units:
    auto unit = static_cast<RS2::Unit>(cbUnit->currentIndex());
    m_graphic->setUnit(unit);

    RS_Units::setCurrentDrawingUnits(unit);

    m_graphic->addVariable("$LUNITS", cbLengthFormat->currentIndex() + 1, 70);
    m_graphic->addVariable("$LUPREC", cbLengthPrecision->currentIndex(), 70);
    m_graphic->addVariable("$AUNITS", cbAngleFormat->currentIndex(), 70);
    m_graphic->addVariable("$AUPREC", cbAnglePrecision->currentIndex(), 70);

    // angles system
    bool ok{false};
    double baseAngle = RS_Math::eval(leAngleBaseZero->text(), &ok);
    if (ok){
        double baseAngleRad = RS_Math::deg2rad(baseAngle);
        m_graphic->setAnglesBase(baseAngleRad);
    }
    bool anglesCounterClockwise = rbAngleBasePositive->isChecked();
    m_graphic->setAnglesCounterClockwise(anglesCounterClockwise);
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
        validatePaperTab();
        validateUnitsTab();
        validateGridTab();
        validateSplinesTab();

        // update all dimension and spline entities in the graphic to match the new settings:
        // update text position when text height or text gap changed
#ifdef ENABLE_LEGACY_DIMENSIONS_TAB
        _toRemove_validateDimsOld();
#endif
        validateDimensionsTab();
        validatePointsTab();
        validateMetaTab();
        validateUserTab();

        // indicate graphic is modified and requires save
        m_graphic->setModified(true);
    }
    accept();
}

/**
 * Updates the length precision combobox
 */
void QG_DlgOptionsDrawing::updateLengthPrecision() {
    RS2::LinearFormat linearFormat = linearFormatFromUI(cbLengthFormat->currentIndex());
    updateLengthPrecisionCombobox(linearFormat, cbLengthPrecision);

    switch (linearFormat) {
        case RS2::Engineering:
        case RS2::Architectural: {
            cbUnit->setCurrentIndex(RS2::Inch);
            break;
        }
        case RS2::ArchitecturalMetric: {
            cbUnit->setCurrentIndex(RS2::Meter);
            break;
        }
        case RS2::Decimal: {
            auto unit = static_cast<RS2::Unit>(cbUnit->currentIndex());
            if (unit == RS2::Foot || unit == RS2::Inch || unit == RS2::Microinch || unit == RS2::Mil || unit == RS2::Mile || unit == RS2::Yard) {
                cbUnit->setCurrentIndex(RS2::Millimeter);
            }
            break;
        }
        default:
            break;
    }
}

/**
 * Updates the Dimension length precision combobox
 */
void QG_DlgOptionsDrawing::updateDimLengthPrecision() {
    RS2::LinearFormat linearFormat = linearFormatFromUI(cbDimLUnit->currentIndex());
    updateLengthPrecisionCombobox(linearFormat, cbDimDec);
}

void QG_DlgOptionsDrawing::updateLengthPrecisionCombobox(RS2::LinearFormat unit, QComboBox* p){

    p->clear();

    switch (unit) {
        // scientific
        case RS2::Scientific: {
            p->addItem("0E+01");
            p->addItem("0.0E+01");
            p->addItem("0.00E+01");
            p->addItem("0.000E+01");
            p->addItem("0.0000E+01");
            p->addItem("0.00000E+01");
            p->addItem("0.000000E+01");
            p->addItem("0.0000000E+01");
            p->addItem("0.00000000E+01");

            // fixme - which precision is default for which unit type? Is it related to drawing precision?
            p->setCurrentIndex(2);
            break;
        }
        case RS2::Decimal: {
            //   (0, 0.1, 0.01, ...)
            // precision list:
            for (int i = 0; i <= 8; i++) {
                p->addItem(QString("%1").arg(0.0, 0, 'f', i));
            }
            p->setCurrentIndex(2); // fixme - which precision is default for which unit type?
            break;
        }
        case RS2::Architectural: {
            p->addItem("0'-0\"");
            p->addItem("0'-0 1/2\"");
            p->addItem("0'-0 1/4\"");
            p->addItem("0'-0 1/8\"");
            p->addItem("0'-0 1/16\"");
            p->addItem("0'-0 1/32\"");
            p->addItem("0'-0 1/64\"");
            p->addItem("0'-0 1/128\"");

            p->setCurrentIndex(2); // fixme - which precision is default for which unit type?
            break;
        }
        case RS2::Engineering: {
            p->addItem("0'-0\"");
            p->addItem("0'-0.0\"");
            p->addItem("0'-0.00\"");
            p->addItem("0'-0.000\"");
            p->addItem("0'-0.0000\"");
            p->addItem("0'-0.00000\"");
            p->addItem("0'-0.000000\"");
            p->addItem("0'-0.0000000\"");
            p->addItem("0'-0.00000000\"");

            p->setCurrentIndex(2); // fixme - which precision is default for which unit type?
            break;
        }
        case RS2::Fractional: {
            p->addItem("0");
            p->addItem("0 1/2");
            p->addItem("0 1/4");
            p->addItem("0 1/8");
            p->addItem("0 1/16");
            p->addItem("0 1/32");
            p->addItem("0 1/64");
            p->addItem("0 1/128");

            p->setCurrentIndex(2); // fixme - which precision is default for which unit type?
            break;
        }
        case RS2::ArchitecturalMetric: {
            for (int i = 0; i <= 8; i++) {
                p->addItem(QString("%1").arg(0.0, 0, 'f', i));
            }

            p->setCurrentIndex(2); // fixme - which precision is default for which unit type?
            break;
        }
        default:
            LC_ERR << "QG_DlgOptionsDrawing::updateLengthPrecisionCombobox: error";
            break;
    }
}

/**
 * Updates the angle precision combobox
 */
void QG_DlgOptionsDrawing::updateAnglePrecision() {
    RS2::AngleFormat angleFormat = angleFormatFromUI(cbAngleFormat->currentIndex());
    updateAnglePrecisionCombobox(angleFormat, cbAnglePrecision);
}

/**
 * Updates the dimension angle precision combobox
 */
void QG_DlgOptionsDrawing::updateDimAnglePrecision() {
    RS2::AngleFormat angleFormat = angleFormatFromUI(cbDimAUnit->currentIndex());
    updateAnglePrecisionCombobox(angleFormat, cbDimADec);
}

/**
 * Updates the preview of unit display.
 */
void QG_DlgOptionsDrawing::updateUnitsPreview() {
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
    updateUnitsPreview();
    updatePaperPreview();
}

/**
 * Updates all unit labels that depend on the global unit.
 */
void QG_DlgOptionsDrawing::updateUnitLabels() {
    auto u = static_cast<RS2::Unit>(cbUnit->currentIndex());
    QString unitSign = RS_Units::unitToSign(u);
    auto labels = {lDimUnit1, lDimUnit2, lDimUnit3, lDimUnit4, lDimUnit5, lDimUnit6, lDimUnit7};
    for(QLabel* unitLabel : labels) {
        unitLabel->setText(unitSign);
    }
    //have to update paper size when unit changes
    updatePaperSize();
}

/**
 * Updates paper preview with specified size and margins.
 */
void QG_DlgOptionsDrawing::updatePaperPreview() {
    auto paperWidthText = lePaperWidth->text();
    auto paperHeightText = lePaperHeight->text();

    double paperW = RS_Math::eval(paperWidthText);
    double paperH = RS_Math::eval(paperHeightText);

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
    if (m_graphic != nullptr) {
        updatePaperPreview();
        m_previewView->zoomAuto();
    }
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

void QG_DlgOptionsDrawing::fillLinearUnitsCombobox(QComboBox* combobox) {
    QStringList unitList;
    unitList << tr("Scientific")
        << tr("Decimal")
        << tr("Engineering")
        << tr("Architectural")
        << tr("Fractional")
        << tr("Architectural (metric)");

    combobox->insertItems(0, unitList);
}

void QG_DlgOptionsDrawing::fillAngleUnitsCombobox(QComboBox* combobox) {
    // init angle units combobox:
    QStringList aunitList;
    aunitList << tr("Decimal Degrees")
        << tr("Deg/min/sec")
        << tr("Gradians")
        << tr("Radians")
        << tr("Surveyor's units");
    combobox->insertItems(0, aunitList);
}

RS2::AngleFormat QG_DlgOptionsDrawing::angleFormatFromUI(int current_index) {
    switch (current_index) {
        case 0:
            return RS2::DegreesDecimal;
        case 1:
            return RS2::DegreesMinutesSeconds;
        case 2:
            return RS2::Gradians;
        case 3:
            return RS2::Radians;
        case 4:
            return RS2::Surveyors;
        default:
            return RS2::DegreesDecimal;
    }
}

RS2::LinearFormat QG_DlgOptionsDrawing::linearFormatFromUI(int currentIndex) {
    switch (currentIndex) {
        case 0:
            return RS2::Scientific;
        case 1:
            return RS2::Decimal;
        case 3:
            return RS2::Architectural;
        case 2:
            return RS2::Engineering;
        case 4:
            return RS2::Fractional;
        case 5:
            return RS2::ArchitecturalMetric;
        default:
            LC_ERR << QString("QG_DlgOptionsDrawing::linearFormatFromUI: error. Value: %1").arg(currentIndex);
            return RS2::Decimal;
    }
}

/**
 * Updates the angle precision combobox
 */
void QG_DlgOptionsDrawing::updateAnglePrecisionCombobox(RS2::AngleFormat format, QComboBox* p)  {
    int index = p->currentIndex();
    p->clear();
    switch (format) {
        case RS2::DegreesDecimal: {
            for (int i = 0; i <= 8; i++) {
                p->addItem(QString("%1").arg(0.0, 0, 'f', i));
            }
            break;
        }
        case RS2::AngleFormat::DegreesMinutesSeconds: {
            p->addItem(QString("0%1").arg(QChar(0xB0)));
            p->addItem(QString("0%100'").arg(QChar(0xB0)));
            p->addItem(QString("0%100'00\"").arg(QChar(0xB0)));
            p->addItem(QString("0%100'00.0\"").arg(QChar(0xB0)));
            p->addItem(QString("0%100'00.00\"").arg(QChar(0xB0)));
            p->addItem(QString("0%100'00.000\"").arg(QChar(0xB0)));
            p->addItem(QString("0%100'00.0000\"").arg(QChar(0xB0)));
            break;
        }
        case RS2::AngleFormat::Gradians: {
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
        }
        case RS2::AngleFormat::Radians: {
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
        }
        case RS2::AngleFormat::Surveyors:{

            p->addItem("N 0d E");
            p->addItem("N 0d00' E");
            p->addItem("N 0d00'00\" E");
            p->addItem("N 0d00'00.0\" E");
            p->addItem("N 0d00'00.00\" E");
            p->addItem("N 0d00'00.000\" E");
            p->addItem("N 0d00'00.0000\" E");
            break;
        }
        default:
            break;
    }
    p->setCurrentIndex(index);
}
