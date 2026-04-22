/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#ifndef LC_PROPERTIESSHEET_H
#define LC_PROPERTIESSHEET_H

#include <QAbstractScrollArea>
#include <memory>

#include "lc_inplace_property_editing_controller.h"
#include "lc_inplace_property_editing_stopper.h"
#include "lc_properties_sheet_model.h"
#include "lc_property_view_factory.h"

struct LC_PropertyEventContext;
class QRubberBand;
class QHelpEvent;
class LC_GuardedConnectionsList;

enum LC_PropertiesSheetWidgetStyleFlag {
    PropertiesSheetStyleNone               = 0x0000,
    PropertiesSheetStyleShowRoot           = 0x0001,
    PropertiesSheetStyleLiveSplit          = 0x0002,
    PropertiesSheetStyleDblClickActivation = 0x0004
};

Q_DECLARE_FLAGS(LC_PropertiesSheetWidgetStyle, LC_PropertiesSheetWidgetStyleFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(LC_PropertiesSheetWidgetStyle)

class LC_PropertiesSheet : public QAbstractScrollArea, public LC_InplacePropertyEditingStopper {
    Q_OBJECT Q_DISABLE_COPY(LC_PropertiesSheet)

public:
    explicit LC_PropertiesSheet(QWidget* parent = nullptr, LC_PropertyContainer* propertySet = nullptr);
    ~LC_PropertiesSheet() override;

    LC_PropertiesSheetModel* getModel() {
        return &m_sheetModel;
    }

    LC_PropertyViewFactory* getViewFactory() const {
        return m_sheetModel.getViewFactory();
    }

    const LC_PropertyContainer* getPropertyContainer() const {
        return m_sheetModel.getPropertyContainer();
    }

    LC_PropertyContainer* getPropertyContainer() {
        return m_sheetModel.getPropertyContainer();
    }

    void setPropertyContainer(LC_PropertyContainer* newPropertyContainer);
    LC_Property* getPropertyParent(const LC_Property* property) const;

    LC_Property* activeProperty() const {
        return m_activeProperty;
    }

    const LC_Property* getActiveProperty() const {
        return m_activeProperty;
    }

    bool setActiveProperty(LC_Property* newActiveProperty, bool shouldBeVisible = false);
    bool setActiveProperty(int index, bool ensureVisible = false);
    bool ensureItemViewVisible(const LC_Property* property) const;
    bool ensurePropertyVisible(const LC_Property* property) const;

    LC_PropertiesSheetWidgetStyle getPropertyViewStyle() const {
        return m_style;
    }

    void setPropertyViewStyle(LC_PropertiesSheetWidgetStyle style);
    void addPropertyViewStyle(LC_PropertiesSheetWidgetStyle style);
    void removePropertyViewStyle(LC_PropertiesSheetWidgetStyle style);
    LC_Property* getPropertyAt(const QPoint& position, QRect* itemViewRect = nullptr) const;
    void connectPropertyToEdit(const LC_Property* property, LC_GuardedConnectionsList& outConnections);

    void updateStylingVars();

    int getItemHeight() const {
        return m_itemHeight;
    }

    quint32 getItemHeightSpacing() const {
        return m_itemHeightSpacing;
    }

    bool setItemHeightSpacing(quint32 itemHeightSpacing);
    int getValueLeftMargin() const;

    int getMinSplitMargin() const {
        return m_minSplitMargin;
    }

    void setMinSplitMargin(const int minSplitMargin) {
        m_minSplitMargin = minSplitMargin;
    }

    bool isMouseCaptured() const;
    bool stopInplaceEdit(bool deleteLater = true, bool restoreParentFocus = true) override;
    LC_Property* getVisiblePropertyAtPoint(const QPoint& pos) const;
    LC_Property* findProperty(const QString& nameOrPath) const;
    void connectOnPropertyChange(const LC_Property* property, bool doConnect);

    bool isInTreeRebuild() const {
        return m_inTreeRebuild;
    }

    void setSkipNextMouseReleaseEvent() {
        m_skipNextMouseReleaseEvent = true;
    }

signals:
    void propertiesChanged(LC_PropertyChangeReason reason);
    void activePropertyChanged(LC_Property* activeProperty);
    void mouseReleased(QMouseEvent* e);
    void beforePropertyEdited(LC_Property* property, LC_Property::PropertyValuePtr newValue, int typeId);
    void propertyEdited(LC_Property* property);

public slots:
    void onEditedPropertyWillChange(LC_PropertyChangeReason reason, LC_Property::PropertyValuePtr newValue, int typeId);
    void onEditedPropertyDidChange(LC_PropertyChangeReason reason);
    void onActivePropertyDestroyed();

protected:
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    bool viewportEvent(QEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void tooltipEvent(QHelpEvent* e);
    void keyPressEvent(QKeyEvent* e) override;
    void correctMousePositionForSplitByMinMargin(int& mouseX, QRect rect) const;
    void scrollContentsBy(int dx, int dy) override;
    bool startInplaceEdit(QWidget* editor);
    QRect getPropertyViewPartRect(const LC_Property* property, int partIndex) const;
    [[deprecated]] QRect getItemColumnRect(const LC_Property* property, bool forLeftColumn) const;
    QRect getItemViewRect(int itemViewIndex) const;
protected slots:
    void onModelChanged();
    void onModelDataChanged() const;
    void onPropertyDidChange(LC_PropertyChangeReason reason, LC_PropertiesSheetModel::PropertyItem* item);
    void onPropertyContainerDestroyed();

private:
    struct PropertyItemView;
    bool ensureItemViewVisible(int index) const;
    void invalidateViewParts();
    void deactivateViewParts();
    void updateItemsTree();
    void setActivePropertyInternal(LC_Property* property);
    void invalidateItemViews();
    void validateItemViews() const;
    void fillItemViews(LC_PropertiesSheetModel::PropertyItem* item, int level) const;
    bool isItemPropertyVisible(const LC_PropertiesSheetModel::PropertyItem& item) const;
    void paintItem(QStylePainter& painter, const QRect& rect, const PropertyItemView& itemView) const;
    void changeActivePropertyByIndex(int itemViewIndex);
    int getItemViewIndex(const QPoint& pos) const;
    int getItemViewIndex(const LC_Property* property) const;
    bool handleMouseEvent(int itemViewIndex, QEvent* e, QPoint mousePos);
    bool handleEvent(LC_PropertyEventContext& context, const PropertyItemView& itemView, QPoint mousePos);
    bool grabMouseForPart(LC_PropertyViewPart* part, QPoint mousePos);
    bool releaseMouseForPart(const LC_PropertyViewPart* part, QPoint mousePos);
    void updateVerticalScrollbar() const;
    int splitPosition() const;
    void updateSplitRatio(float splitRatio);
    void connectActiveProperty();
    void disconnectActiveProperty();
    inline int getVerticalScrollBarValue() const;

    LC_Property* m_activeProperty = nullptr;

    mutable QList<PropertyItemView> m_itemViews;
    mutable bool m_itemViewsValid;

    QList<LC_PropertyViewPart*> m_activeViewParts;
    LC_PropertyViewPart* m_mouseEventsReceiverPart;

    LC_InplacePropertyEditingController m_inplaceController;

    LC_PropertiesSheetWidgetStyle m_style;
    int m_itemHeight;
    quint32 m_itemHeightSpacing;
    int m_valueLeftMargin;
    int m_minSplitMargin{20};

    QColor m_linesColor;
    QColor m_propertySetBackgroundColor;

    float m_splitRatio;
    std::unique_ptr<QRubberBand> m_rubberBand;

    bool m_mouseAtSplitter = false;
    bool m_mouseCaptured = false;
    bool m_drawDenseLevels = false;

    bool m_skipNextMouseReleaseEvent = false;

    LC_PropertiesSheetModel m_sheetModel;

    bool m_inTreeRebuild{false};

    friend struct LC_PropertyEventContext;
    friend struct LC_PropertyViewPart;
};

#endif
