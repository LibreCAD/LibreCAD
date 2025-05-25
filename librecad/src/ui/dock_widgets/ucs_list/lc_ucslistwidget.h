/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#ifndef LC_UCSLISTWIDGET_H
#define LC_UCSLISTWIDGET_H

#include <QModelIndex>
#include "lc_graphicviewawarewidget.h"
#include "lc_ucslist.h"

class RS_Graphic;
class RS_GraphicView;
class LC_UCSListOptions;
class LC_UCSListModel;

class QItemSelection;
class LC_UCSStateWidget;
class LC_UCSListButton;
class LC_GraphicViewport;

namespace Ui {
    class LC_UCSListWidget;
}

class LC_UCSListWidget : public LC_GraphicViewAwareWidget, LC_UCSListListener{
    Q_OBJECT
public:
    explicit LC_UCSListWidget(const QString& title, QWidget* parent);
    virtual ~LC_UCSListWidget();
    void setUCSList(LC_UCSList* viewsList);
    void setGraphicView(RS_GraphicView* gv) override;
    void reload();
    void fillUCSList(QList<LC_UCS *> &list);
    QIcon getUCSTypeIcon(LC_UCS *view);
    QWidget* createSelectionWidget(QAction* saveViewAction, QAction* defaultAction);
    void ucsListModified([[maybe_unused]]bool changed) override{refresh();}
    QModelIndex getIndexForUCS(LC_UCS *u);
    void applyUCSByIndex(QModelIndex index);
    LC_UCS* getActiveUCS();
signals:
    void ucsListChanged();
public slots:
    void setWCS();
    void onViewUCSChanged(LC_UCS* ucs);
    void setStateWidget(LC_UCSStateWidget *stateWidget);
    void updateWidgetSettings();
protected slots:
    void invokeOptionsDialog();
    void saveCurrentUCS();
    void activateUCS();
    void previewUCS();
    void removeUCS();
    void removeAllUCSs();
    void editUCS();
    void setUCSByDimOrdinate();
    void onCustomContextMenu(const QPoint &point);
    void slotTableClicked(QModelIndex layerIdx);
    void onTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onTableDoubleClicked();
protected:
    LC_UCSList* m_currentUCSList{nullptr};
    LC_UCSListModel* m_ucsListModel{nullptr};
    LC_UCSListOptions *m_options {nullptr};
    LC_UCSListButton *m_ucsListButton {nullptr};
    LC_UCSStateWidget* m_ucsStateWidget {nullptr};
    QAction* m_applyUCSAction {nullptr};
    QAction* m_createUCSAction {nullptr};
    RS_GraphicView *m_graphicView {nullptr};
    LC_GraphicViewport *m_viewport {nullptr};
    RS2::LinearFormat m_linearFormat;
    RS2::AngleFormat m_angleFormat;
    int m_precision;
    int m_anglePrecision;
    RS2::Unit m_drawingUnit;
    Ui::LC_UCSListWidget *ui;

    void initToolbar() const;
    void refresh();
    void updateData(bool restoreSelectionIfPossible);
    void loadOptions();
    void createModel();
    void applyUCS(LC_UCS* ucs);
    void previewExistingUCS(LC_UCS* ucs);
    LC_UCS *getSelectedUCS();
    void removeExistingUCS(LC_UCS *ucs);
    QModelIndex getSelectedItemIndex();
    void renameExistingUCS(QString newName, LC_UCS *ucs);
    void renameExistingUCS(LC_UCS *selectedUCS);
    void updateButtonsState() const;
    void selectUCS(LC_UCS *ucs);
    int getSingleSelectedRow() const;
    void restoreSingleSelectedRow(bool restoreSelectionIfPossible, int selectedRow);
    void loadFormats(RS_Graphic *graphic);
};

#endif // LC_UCSLISTWIDGET_H
