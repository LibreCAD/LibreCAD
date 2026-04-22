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
#ifndef QG_DLGOPTIONSDRAWING_H
#define QG_DLGOPTIONSDRAWING_H

#include "lc_dialog.h"
#include "lc_dimstylepreviewgraphicview.h"
#include "ui_qg_dlgoptionsdrawing.h"

class LC_DimStyleTreeModel;
class RS_Graphic;
class RS_Vector;

class QG_DlgOptionsDrawing : public LC_Dialog, public Ui::QG_DlgOptionsDrawing {
    Q_OBJECT
public:
    void connectPointsTab();
    void _to_remove_ConnectLegacyDimsTab();
    explicit QG_DlgOptionsDrawing(QWidget* parent = nullptr);
    ~QG_DlgOptionsDrawing() override;
    void showInitialTab(int tabIndex) const;
    void setGraphic(RS_Graphic* g);
    static void fillLinearUnitsCombobox(QComboBox* combobox);
    static void fillAngleUnitsCombobox(QComboBox* combobox);
    static void updateLengthPrecisionCombobox(RS2::LinearFormat unit, QComboBox* p);
    static void updateAnglePrecisionCombobox(RS2::AngleFormat format, QComboBox* p);
    static RS2::AngleFormat angleFormatFromUI(int currentIndex);
    static RS2::LinearFormat linearFormatFromUI(int currentIndex);
protected
    slots :
    void languageChange();
    void validate();
    void updateLengthPrecision() const;
    void updateAnglePrecision() const;
    void updateUnitsPreview() const;
    void updatePaperSize() const;
    void updateUnitLabels();
    void updateDimLengthPrecision() const;
    void updateDimAnglePrecision() const;
    void updatePaperPreview() const;
    void onTabCurrentChanged(int index) const;
    void on_cbGridOn_toggled(bool checked) const;
    void onLandscapeToggled(bool checked);
    void onDimFxLonToggled(bool checked) const;
    void onRelSizeToggled(bool checked);
    void disableXSpacing(bool checked) const;
    void enableXSpacing(bool checked) const;

    void onDimStyleNew(bool checked);
    void expandStylesTree() const;
    void onDimStyleEdit(bool checked = false);
    void onDimStyleRename(bool checked);
    void onDimStyleRemove(bool checked);
    void onDimStyleExport(bool checked);
    void onDimStyleImport(bool checked);
    void updateActiveStyleLabel(const LC_DimStyleTreeModel* model) const;
    void onDimStyleSetDefault(bool checked);
    void updateActionButtons(const LC_DimStyleItem* item) const;
    void onDimCurrentChanged(const QModelIndex& current, const QModelIndex& previous);

    void onDimStylesListMenuRequested(const QPoint& pos);
    void onDimStyleDoubleClick();

    void onCustomVariableAdd(bool checked);
    void onCustomVariableDelete(bool checked);
    void reject() override;
protected:
    void setupPointsTab() const;
    void setupSplinesTab() const;
    void setupGridTab() const;
    void setupPaperTab();
    void setupMetaTab() const;
    void setupUserREditor(QLineEdit* edit, const QString& key) const;
    void setupUserTab() const;
    void _toRemoveSetupLegacyDimsTab(RS2::LinearFormat& linearFormat, int lunits, int luprec, int aunits, int auprec) const;
    LC_DimStyleTreeModel* getDimStylesModel() const;
    void doCreateDimStyle(const QString& newStyleName, LC_DimStyleTreeModel* model, const LC_DimStyleItem* styleItemBasedOn,
                          RS2::EntityType newDimType);
    void connectPaperTab();
    void connectUnitTab();
    void connectGridTab();
    void connectUserVarsTab();
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void updateLPtSzUnits() const;
    QString askForUniqueDimStyleName(const QString& caption, const QString& prompt, const QString& defaultText);
    void updateDimStylePreview(LC_DimStyle* dimStyle, LC_DimStyleTreeModel* model) const;

private:
    std::unique_ptr<QStringList> m_listPrec1;
    RS_Graphic* m_graphic{nullptr};
    QGraphicsScene* m_paperScene{nullptr};
    std::unique_ptr<RS_Vector> m_spacing;
    LC_DimStylePreviewGraphicView* m_previewView{nullptr};
    bool m_hasImportantModificationsToAskOnCancel = false;

    void init();
    void prepareDimStyleItems(QList<LC_DimStyleItem*>& items) const;
    void collectStylesUsage(QMap<QString, int>& map) const;
    void setupDimStylesTab();
    void setupVariablesTab() const;
    void _toRemove_validateDimsOld() const;
    bool validateDimensionsTab() const;
    bool validatePointsTab() const;
    void validateSplinesTab() const;
    void validateGridTab() const;
    void validatePaperTab() const;
    void validateMetaTab() const;
    void validateUserTab() const;
    void validateUnitsTab() const;
    QModelIndex getSelectedDimStyleIndex() const;
};

#endif
