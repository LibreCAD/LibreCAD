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

#include "ui_qg_dlgoptionsdrawing.h"
#include "lc_dialog.h"
#include "lc_dimstylepreviewgraphicview.h"
#include "lc_dimstyleslistmodel.h"

class LC_DimStyleTreeModel;
class RS_Graphic;
class RS_Vector;

class QG_DlgOptionsDrawing : public LC_Dialog, public Ui::QG_DlgOptionsDrawing{
    Q_OBJECT
public:
    void connectPointsTab();
    void _to_remove_ConnectLegacyDimsTab();
    explicit QG_DlgOptionsDrawing(QWidget* parent = nullptr);
	~QG_DlgOptionsDrawing() override;
    void showInitialTab(int tabIndex);
    void setGraphic( RS_Graphic * g );
    static void fillLinearUnitsCombobox(QComboBox* combobox);
    static void fillAngleUnitsCombobox(QComboBox* combobox);
    static void updateLengthPrecisionCombobox(RS2::LinearFormat unit, QComboBox* p);
    static void updateAnglePrecisionCombobox(RS2::AngleFormat format, QComboBox* p);
    static RS2::AngleFormat angleFormatFromUI(int current_index);
    static RS2::LinearFormat linearFormatFromUI(int currentIndex);
protected slots:
    void languageChange();
    void validate();
    void updateLengthPrecision();
    void updateAnglePrecision();
    void updateUnitsPreview();
    void updatePaperSize();
    void updateUnitLabels();
    void updateDimLengthPrecision();
    void updateDimAnglePrecision();
    void updatePaperPreview();
    void onTabCurrentChanged(int index);
    void on_cbGridOn_toggled(bool checked);
	void onLandscapeToggled(bool checked);
    void onDimFxLonToggled(bool checked);
    void onRelSizeToggled(bool checked);
    void disableXSpacing(bool checked);
    void enableXSpacing(bool checked);

    void onDimStyleNew(bool checked);
    void expandStylesTree();
    void onDimStyleEdit(bool checked=false);
    void onDimStyleRename(bool checked);
    void onDimStyleRemove(bool checked);
    void onDimStyleExport(bool checked);
    void onDimStyleImport(bool checked);
    void updateActiveStyleLabel(LC_DimStyleTreeModel* model);
    void onDimStyleSetDefault(bool checked);
    void updateActionButtons(LC_DimStyleItem* item);
    void onDimCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

    void onDimStylesListMenuRequested(const QPoint &pos);
    void onDimStyleDoubleClick();

    void onCustomVariableAdd(bool checked);
    void onCustomVariableDelete(bool checked);

    void reject() override;
protected:
    void setupPointsTab();
    void setupSplinesTab();
    void setupGridTab();
    void setupPaperTab();
    void setupMetaTab();
    void setupUserREditor(QLineEdit* edit, const QString &key);
    void setupUserTab();
    void _toRemoveSetupLegacyDimsTab(RS2::LinearFormat& linearFormat, int lunits, int luprec, int aunits, int auprec);
    LC_DimStyleTreeModel* getDimStylesModel();
    void doCreateDimStyle(const QString &newStyleName, LC_DimStyleTreeModel* model, LC_DimStyleItem* styleItemBasedOn, RS2::EntityType newDimType);
    void connectPaperTab();
    void connectUnitTab();
    void connectGridTab();
    void connectUserVarsTab();
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void updateLPtSzUnits();
    QString askForUniqueDimStyleName(const QString &caption, const QString& prompt, const QString &defaultText);
    void updateDimStylePreview(LC_DimStyle* dimStyle, LC_DimStyleTreeModel* model) const;
private:
    std::unique_ptr<QStringList> m_listPrec1;
    RS_Graphic* m_graphic {nullptr};
    QGraphicsScene* m_paperScene {nullptr};
    std::unique_ptr<RS_Vector> m_spacing;
    LC_DimStylePreviewGraphicView* m_previewView {nullptr};
    bool m_hasImportantModificationsToAskOnCancel = false;

    void init();
    void prepareDimStyleItems(QList<LC_DimStyleItem*> &items);
    void collectStylesUsage(QMap<QString, int>& map);
    void setupDimStylesTab();
    void setupVariablesTab();
    void _toRemove_validateDimsOld();
    bool validateDimensionsTab();
    bool validatePointsTab();
    void validateSplinesTab();
    void validateGridTab();
    void validatePaperTab();
    void validateMetaTab();
    void validateUserTab();
    void validateUnitsTab();
    QModelIndex getSelectedDimStyleIndex();


};

#endif // QG_DLGOPTIONSDRAWING_H
