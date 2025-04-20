/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#ifndef LC_NAMEDVIEWSLISTWIDGET_H
#define LC_NAMEDVIEWSLISTWIDGET_H

#include <QItemSelection>

#include "rs.h"
#include "lc_graphicviewaware.h"
#include "lc_graphicviewawarewidget.h"

class LC_View;
class LC_ViewList;
class LC_NamedViewsButton;
class LC_NamedViewsModel;
class LC_NamedViewsListOptions;
class LC_GraphicViewport;
class RS_Graphic;

namespace Ui {
    class LC_NamedViewsListWidget;
}

class LC_NamedViewsListWidget : public LC_GraphicViewAwareWidget{
 Q_OBJECT
public:
    explicit LC_NamedViewsListWidget(const QString& title, QWidget* parent);
    ~LC_NamedViewsListWidget() override;
    void setViewsList(LC_ViewList* viewsList);
    void setGraphicView(RS_GraphicView* gv) override;
    void reload();
    void restoreView(int index);
    void restoreView(const QString &name);
    void restoreSelectedView();
    void fillViewsList(QList<LC_View *> &list) const;
    QIcon getViewTypeIcon(LC_View *view) const;
    QWidget* createSelectionWidget(QAction* saveViewAction, QAction* defaultAction);
signals:
    void viewListChanged(int itemsCount);
public slots:
    void addNewView();
    void onUcsListChanged() const;
    void updateWidgetSettings() const;
protected slots:
    void invokeOptionsDialog();
    void updateView();
    void invokeView();
    void removeView();
    void removeAllViews();
    void renameView();
    void onCustomContextMenu(const QPoint &point);
    void slotTableClicked(const QModelIndex& layerIdx);
    void onTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) const;
    void onTableDoubleClicked();
protected:
    void doCreateNewView(const QString& name);
    void updateViewsUCSNames() const;
    void refresh();
private:
    Ui::LC_NamedViewsListWidget *ui;
    LC_ViewList* m_currentViewList{nullptr};
    LC_NamedViewsModel* m_viewsModel{nullptr};
    LC_NamedViewsListOptions *m_options {nullptr};
    LC_NamedViewsButton *m_namedViewsButton {nullptr};
    QAction* m_saveViewAction {nullptr};
    RS2::LinearFormat m_linearFormat;
    RS2::AngleFormat m_angleFormat;
    int m_precision;
    int m_anglePrecision;
    RS2::Unit drawingUnit;
    RS_GraphicView *m_graphicView {nullptr};
    LC_GraphicViewport *m_viewport {nullptr};

    void initToolbar() const;
    void updateData(bool restoreSelectionIfPossible);
    void loadOptions();
    void createModel();
    void restoreView(LC_View* view);
    void updateExistingView(LC_View *pView);
    LC_View *getSelectedView() const;
    void removeExistingView(LC_View *view) const;
    QModelIndex getSelectedItemIndex() const;
    void renameExistingView(const QString &newName, LC_View *view);

    void doUpdateView(LC_View *view);
    void renameExistingView(LC_View *selectedView);
    void updateButtonsState() const;
    void selectView(const LC_View *view) const;
    int getSingleSelectedRow() const;
    void restoreSingleSelectedRow(bool restoreSelectionIfPossible, int selectedRow) const;
    void loadFormats(const RS_Graphic *graphic);
};

#endif // LC_NAMEDVIEWSLISTWIDGET_H
