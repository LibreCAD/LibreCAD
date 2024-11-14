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

#include <QWidget>
#include <QList>
#include <QItemSelection>

#include "lc_viewslist.h"
#include "lc_namedviewsmodel.h"
#include "rs_graphicview.h"
#include "qc_mdiwindow.h"

class LC_NamedViewsButton;

namespace Ui {
    class LC_NamedViewsListWidget;
}

class LC_NamedViewsListWidget : public QWidget{
Q_OBJECT

public:
    explicit LC_NamedViewsListWidget(const QString& title, QWidget* parent);
    virtual ~LC_NamedViewsListWidget();
    void setViewsList(LC_ViewList* viewsList);
    void setGraphicView(RS_GraphicView* gv, QMdiSubWindow* window);
    void refresh();
    void restoreView(int index);
    void restoreView(const QString &name);
    void restoreSelectedView();
    void fillViewsList(QList<LC_View *> &list);
    QIcon getViewTypeIcon(LC_View *view);
    QWidget* createSelectionWidget(QAction* saveViewAction, QAction* defaultAction);
signals:
    void viewListChanged(int itemsCount);
public slots:
    void addNewView();
protected slots:
    void invokeOptionsDialog();
    void updateView();
    void invokeView();
    void removeView();
    void removeAllViews();
    void renameView();
    void onCustomContextMenu(const QPoint &point);
    void slotTableClicked(QModelIndex layerIdx);
    void onTableSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void onTableDoubleClicked();
protected:
    void doCreateNewView(QString name);
private:
    Ui::LC_NamedViewsListWidget *ui;
    LC_ViewList* currentViewList{nullptr};
    LC_NamedViewsModel* viewsModel{nullptr};
    LC_NamedViewsListOptions *options {nullptr};
    LC_NamedViewsButton *namedViewsButton {nullptr};
    QAction* saveViewAction {nullptr};
    QMdiSubWindow* window;

    RS2::LinearFormat linearFormat;
    int precision;
    RS2::Unit drawingUnit;

    void initToolbar() const;
    void updateData(bool restoreSelectionIfPossible);
    void loadOptions();
    void createModel();
    void restoreView(LC_View* view);
    void updateExistingView(LC_View *pView);
    LC_View *getSelectedView();
    void removeExistingView(LC_View *view);
    QModelIndex getSelectedItemIndex();
    void renameExistingView(QString newName, LC_View *view);
    RS_GraphicView *graphicView {nullptr};
    void doUpdateView(LC_View *view);
    void renameExistingView(LC_View *selectedView);
    void updateButtonsState() const;
    void doUpdateViewByGraphicView(LC_View *view) const;

    void selectView(LC_View *view);

    void panZoomGraphicView(const RS_Vector &center, const RS_Vector &size);

    int getSingleSelectedRow() const;

    void restoreSingleSelectedRow(bool restoreSelectionIfPossible, int selectedRow);
};

#endif // LC_NAMEDVIEWSLISTWIDGET_H
