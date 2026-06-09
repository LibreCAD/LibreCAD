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

#include "lc_properties_sheet.h"

#include <QApplication>
#include <QHelpEvent>
#include <QRubberBand>
#include <QScrollBar>
#include <QToolTip>
#include <memory>

#include "lc_guardedconnectionslist.h"
#include "lc_property_event_context.h"
#include "lc_property_paint_context.h"
#include "lc_property_view_part.h"

struct LC_PropertiesSheet::PropertyItemView {
    LC_PropertiesSheetModel::PropertyItem* propertyItem;
    int level;
    bool hasChildren;

    mutable QList<LC_PropertyViewPart> viewParts;
    mutable bool viewPartsValid;

    PropertyItemView();

    bool isSplittable() const {
        const auto view = propertyItem->view.get();
        Q_ASSERT(view); // cannot be null
        return view->isSplittable();
    }

    void paint(LC_PropertyPaintContext paintContext) const {
        const auto view = propertyItem->view.get();
        Q_ASSERT(view); // cannot be null

        // create sub-items if not initialized
        if (!viewPartsValid) {
            Q_ASSERT(viewParts.isEmpty());
            view->buildViewParts(paintContext, viewParts);
            viewPartsValid = true;
        }

        // draw sub-items
        for (const auto& part : std::as_const(viewParts)) {
            part.paint(paintContext);
        }
    }
};

class LC_SheetPainterGuard {
public:
    explicit LC_SheetPainterGuard(QPainter& p);
    ~LC_SheetPainterGuard();

private:
    QPainter& m_p;
};

extern void setSmallerTextOSX(QWidget* w);

LC_PropertiesSheet::LC_PropertiesSheet(QWidget* parent, LC_PropertyContainer* propertySet)
    : QAbstractScrollArea(parent), m_itemViewsValid(false), m_mouseEventsReceiverPart(nullptr), m_style(PropertiesSheetStyleLiveSplit),
      m_itemHeight(0), m_itemHeightSpacing(6), m_valueLeftMargin(0), m_splitRatio(0.5F),
      m_sheetModel{LC_PropertiesSheetModel(propertySet)} {

    const auto viewFactory = LC_PropertyViewFactory::staticInstance();
    m_sheetModel.setViewFactory(viewFactory);

    setSmallerTextOSX(this);

    connect(&m_sheetModel, &LC_PropertiesSheetModel::modelChanged, this, &LC_PropertiesSheet::onModelChanged);
    connect(&m_sheetModel, &LC_PropertiesSheetModel::modelDataChanged, this, &LC_PropertiesSheet::onModelDataChanged);
    connect(&m_sheetModel, &LC_PropertiesSheetModel::propertyDidChange, this, &LC_PropertiesSheet::onPropertyDidChange);

    setFocusPolicy(Qt::StrongFocus);
    viewport()->setMouseTracking(true);
    updateStylingVars();
    updateItemsTree();
}

LC_PropertiesSheet::~LC_PropertiesSheet() {
    // destruct everything
}

void LC_PropertiesSheet::setPropertyContainer(LC_PropertyContainer* newPropertyContainer) {
    const auto old = m_sheetModel.getPropertyContainer();
    if (newPropertyContainer == old) {
        return;
    }
    if (old != nullptr) {
        disconnect(old, &LC_Property::destroyed, this, &LC_PropertiesSheet::onPropertyContainerDestroyed);
    }
    if (newPropertyContainer != nullptr) {
        connect(newPropertyContainer, &LC_Property::destroyed, this, &LC_PropertiesSheet::onPropertyContainerDestroyed);
    }
    m_sheetModel.setPropertyContainer(newPropertyContainer);
    m_inTreeRebuild = true;
    updateItemsTree();
    m_inTreeRebuild = false;
}

LC_Property* LC_PropertiesSheet::getPropertyParent(const LC_Property* property) const {
    return m_sheetModel.getParentProperty(property);
}

bool LC_PropertiesSheet::setActiveProperty(LC_Property* newActiveProperty, const bool shouldBeVisible) {
    if (m_activeProperty == newActiveProperty) {
        return false;
    }
    stopInplaceEdit();
    if (shouldBeVisible) {
        ensureItemViewVisible(newActiveProperty);
    }
    if (newActiveProperty == nullptr) {
        setActivePropertyInternal(nullptr);
        return true;
    }
    const int index = getItemViewIndex(newActiveProperty);
    if (index < 0) {
        return false;
    }
    setActivePropertyInternal(newActiveProperty);
    return true;
}

bool LC_PropertiesSheet::setActiveProperty(int index, const bool ensureVisible) {
    if (index < 0) {
        index = 0;
    }
    const auto container = m_sheetModel.getPropertyContainer();
    if (container == nullptr) {
        return false;
    }
    auto& cp = container->childProperties();
    if (cp.isEmpty()) {
        return false;
    }
    const qsizetype childCount = cp.size();
    if (index >= childCount) {
        index = childCount - 1;
    }
    return setActiveProperty(cp.at(index), ensureVisible);
}

bool LC_PropertiesSheet::ensureItemViewVisible(const LC_Property* property) const {
    if (property == nullptr) {
        return false;
    }
    const int index = getItemViewIndex(property);
    return ensureItemViewVisible(index);
}

bool LC_PropertiesSheet::ensurePropertyVisible(const LC_Property* property) const {
    if (property == nullptr) {
        return false;
    }
    auto currentProperty = property;

    while (true) {
        LC_Property* propertyParent = qobject_cast<LC_PropertyContainer*>(currentProperty->parent());
        if (propertyParent != nullptr) {
            propertyParent = currentProperty->getPrimaryProperty();
        }
        if (propertyParent == nullptr) {
            break;
        }
        propertyParent->removeState(PropertyStateCollapsed);
        currentProperty = propertyParent;
    }
    return ensureItemViewVisible(property);
}

void LC_PropertiesSheet::setPropertyViewStyle(const LC_PropertiesSheetWidgetStyle style) {
    m_style = style;
}

void LC_PropertiesSheet::addPropertyViewStyle(const LC_PropertiesSheetWidgetStyle style) {
    setPropertyViewStyle(getPropertyViewStyle() | style);
}

void LC_PropertiesSheet::removePropertyViewStyle(const LC_PropertiesSheetWidgetStyle style) {
    setPropertyViewStyle(getPropertyViewStyle() & ~style);
}

LC_Property* LC_PropertiesSheet::getPropertyAt(const QPoint& position, QRect* itemViewRect) const {
    const int itemViewIndex = getItemViewIndex(position);
    if (itemViewIndex >= 0) {
        if (nullptr != itemViewRect) {
            *itemViewRect = getItemViewRect(itemViewIndex);
        }
        return m_itemViews[itemViewIndex].propertyItem->property;
    }
    return nullptr;
}

void LC_PropertiesSheet::connectPropertyToEdit(const LC_Property* property, LC_GuardedConnectionsList& outConnections) {
    Q_ASSERT(property != nullptr);

    outConnections.push_back(connect(property, &LC_Property::beforePropertyChange, this, &LC_PropertiesSheet::onEditedPropertyWillChange));
    outConnections.push_back(connect(property, &LC_Property::afterPropertyChange, this, &LC_PropertiesSheet::onEditedPropertyDidChange));
}

bool LC_PropertiesSheet::setItemHeightSpacing(const quint32 itemHeightSpacing) {
    m_itemHeightSpacing = itemHeightSpacing;
    updateStylingVars();
    return true;
}

int LC_PropertiesSheet::getValueLeftMargin() const {
    return m_valueLeftMargin;
}

bool LC_PropertiesSheet::isMouseCaptured() const {
    return m_mouseCaptured || m_rubberBand;
}

bool LC_PropertiesSheet::stopInplaceEdit(const bool deleteLater, const bool restoreParentFocus) {
    return m_inplaceController.stopInplaceEdit(deleteLater, restoreParentFocus);
}

LC_Property* LC_PropertiesSheet::getVisiblePropertyAtPoint(const QPoint& pos) const {
    const int itemViewIndex = getItemViewIndex(pos);
    if (itemViewIndex < 0) {
        return nullptr;
    }
    return m_itemViews[itemViewIndex].propertyItem->property;
}

LC_Property* LC_PropertiesSheet::findProperty(const QString& nameOrPath) const {
    const auto root = getPropertyContainer();
    if (root == nullptr) {
        return nullptr;
    }
    auto properties = root->findChildProperties(nameOrPath);
    if (properties.size() != 1) {
        return nullptr;
    }
    return properties[0];
}

void LC_PropertiesSheet::connectOnPropertyChange(const LC_Property* property, const bool doConnect) {
    if (doConnect) {
        connect(property, &LC_Property::beforePropertyChange, this, &LC_PropertiesSheet::onEditedPropertyWillChange);
        connect(property, &LC_Property::afterPropertyChange, this, &LC_PropertiesSheet::onEditedPropertyDidChange);
    }
    else {
        disconnect(property, &LC_Property::beforePropertyChange, this, &LC_PropertiesSheet::onEditedPropertyWillChange);
        disconnect(property, &LC_Property::afterPropertyChange, this, &LC_PropertiesSheet::onEditedPropertyDidChange);
    }
}

[[deprecated]]
void LC_PropertiesSheet::onEditedPropertyWillChange(const LC_PropertyChangeReason reason, const LC_Property::PropertyValuePtr newValue,
                                                    const int typeId) {
    if (!(reason & PropertyChangeReasonEdit)) {
        return;
    }
    Q_ASSERT(nullptr != qobject_cast<LC_Property*>(sender()));
    const auto property = static_cast<LC_Property*>(sender());
    // fixme - review condition
    if ((reason & PropertyChangeReasonValue) != 0u) {
        emit beforePropertyEdited(property, newValue, typeId);
    }
}

void LC_PropertiesSheet::onEditedPropertyDidChange(const LC_PropertyChangeReason reason) {
    if (!(reason & PropertyChangeReasonEdit)) {
        return;
    }
    Q_ASSERT(nullptr != qobject_cast<LC_Property*>(sender()));
    if ((reason & PropertyChangeReasonValue) != 0u) {
        const auto property = static_cast<LC_Property*>(sender());
        emit propertyEdited(property);
    }
}

void LC_PropertiesSheet::onActivePropertyDestroyed() {
    m_activeProperty = nullptr;
    emit activePropertyChanged(m_activeProperty);
    viewport()->update();
}

void LC_PropertiesSheet::paintEvent([[maybe_unused]] QPaintEvent* e) {
    validateItemViews();

    if (m_itemViews.isEmpty()) {
        return;
    }

    const int verticalScrollBarValue = getVerticalScrollBarValue();
    const int firstVisibleItemIndex = qMin(verticalScrollBarValue / m_itemHeight, (m_itemViews.size() - 1));
    const int lastVisibleItemIndex = qMin(((verticalScrollBarValue + viewport()->height()) / m_itemHeight) + 1, (m_itemViews.size() - 1));

    const auto viewPortRect = viewport()->rect();
    QRect itemRect = viewPortRect;
    itemRect.setTop(firstVisibleItemIndex * m_itemHeight - verticalScrollBarValue);
    itemRect.setBottom(itemRect.top() + m_itemHeight);

    QStylePainter painter(viewport());

    QPen splitterPen;
    splitterPen.setColor(this->palette().color(QPalette::Mid));
    splitterPen.setStyle(Qt::DotLine);
    const int splitPos = splitPosition();

    for (int i = firstVisibleItemIndex; i <= lastVisibleItemIndex; ++i) {
        const PropertyItemView& itemView = m_itemViews[i];

        paintItem(painter, itemRect, itemView);

        if (itemView.isSplittable()) {
            painter.save();
            splitterPen.setDashOffset(itemRect.top());
            painter.setPen(splitterPen);
            painter.drawLine(splitPos, itemRect.top(), splitPos, itemRect.bottom());
            painter.restore();
        }
        itemRect.translate(0, m_itemHeight);
    }
}

void LC_PropertiesSheet::resizeEvent([[maybe_unused]] QResizeEvent* e) {
    stopInplaceEdit();
    invalidateViewParts();
    updateVerticalScrollbar();
}

QRect LC_PropertiesSheet::getPropertyViewPartRect(const LC_Property* property, const int partIndex) const {
    if (property == nullptr) {
        return {};
    }
    const int itemViewIndex = getItemViewIndex(property);
    if (itemViewIndex < 0) {
        return {};
    }
    const auto& item = m_itemViews[itemViewIndex];
    if (!item.viewPartsValid) {
        return {};
    }
    if (partIndex < 0 || partIndex >= item.viewParts.size()) {
        return {};
    }
    return item.viewParts[partIndex].rect;
}

QRect LC_PropertiesSheet::getItemColumnRect(const LC_Property* property, const bool forLeftColumn) const {
    if (property == nullptr) {
        return QRect();
    }
    const int itemViewIndex = getItemViewIndex(property);
    if (itemViewIndex < 0) {
        return QRect();
    }

    QRect rect = getItemViewRect(itemViewIndex);
    if (forLeftColumn) {
        rect.setRight(splitPosition());
    }
    else {
        rect.setLeft(splitPosition() + 1);
    }
    return rect;
}

QRect LC_PropertiesSheet::getItemViewRect(const int itemViewIndex) const {
    Q_ASSERT(itemViewIndex >= 0 && itemViewIndex < m_itemViews.size());
    QRect rect = viewport()->rect();
    rect.setTop(itemViewIndex * m_itemHeight - verticalScrollBar()->value());
    rect.setHeight(m_itemHeight);
    return rect;
}

void LC_PropertiesSheet::paintItem(QStylePainter& painter, const QRect& rect, const PropertyItemView& itemView) const {
    const int level = itemView.level;
    int levelCorrected = level;
    if (m_drawDenseLevels) {
        levelCorrected = level > 0 ? level - 1 : 0;
    }
    const QMargins margins(m_valueLeftMargin + rect.height() * levelCorrected, 0, 0, 0);
    const bool isActive = itemView.propertyItem->property == m_activeProperty;

    const LC_PropertyPaintContext paintContext{&painter, this, rect, margins, splitPosition(), isActive, itemView.hasChildren};
    itemView.paint(paintContext);
}

void LC_PropertiesSheet::changeActivePropertyByIndex(const int itemViewIndex) {
    LC_Property* newActiveProperty = (itemViewIndex < 0) ? nullptr : m_itemViews[itemViewIndex].propertyItem->property;
    setActiveProperty(newActiveProperty);
    ensureItemViewVisible(itemViewIndex);
}

int LC_PropertiesSheet::getItemViewIndex(const QPoint& pos) const {
    const int index = (getVerticalScrollBarValue() + pos.y()) / m_itemHeight;
    if (index >= m_itemViews.size()) {
        return -1;
    }
    return index;
}

int LC_PropertiesSheet::getItemViewIndex(const LC_Property* property) const {
    validateItemViews();
    for (qsizetype i = 0, n = m_itemViews.size(); i < n; ++i) {
        if (m_itemViews[i].propertyItem->property == property) {
            return i;
        }
    }
    return -1;
}

LC_Property* LC_PropertiesSheet::getPropertyWithName(const QString& propertyName) const {
    validateItemViews();
    for (qsizetype i = 0, n = m_itemViews.size(); i < n; ++i) {
        auto property = m_itemViews[i].propertyItem->property;
        if (property->getName() == propertyName) {
            return property;
        }
    }
    return nullptr;
}

bool LC_PropertiesSheet::handleMouseEvent(const int itemViewIndex, QEvent* e, const QPoint mousePos) {
    if (itemViewIndex < 0) {
        deactivateViewParts();
        return false;
    }

    LC_PropertyEventContext context{e, this};
    return handleEvent(context, m_itemViews[itemViewIndex], mousePos);
}

int LC_PropertiesSheet::getVerticalScrollBarValue() const {
    return verticalScrollBar()->value();
}

static constexpr int MOUSE_MAX_DISTANCE_FROM_SPLIT = 3;

void LC_PropertiesSheet::mousePressEvent(QMouseEvent* e) {
    m_mouseCaptured = false;
    const auto mousePos = e->pos();
    switch (e->button()) {
        case Qt::LeftButton: {
            const int itemViewIndex = getItemViewIndex(mousePos);
            bool isSplittableItem = false;
            if (itemViewIndex >= 0) {
                const auto propertyItemView = m_itemViews.at(itemViewIndex);
                isSplittableItem = propertyItemView.isSplittable();
            }

            if (isSplittableItem && qAbs(mousePos.x() - splitPosition()) < MOUSE_MAX_DISTANCE_FROM_SPLIT) {
                m_rubberBand = std::make_unique<QRubberBand>(QRubberBand::Line, this);
                QRect rect = viewport()->rect();
                rect.setLeft(mousePos.x());
                rect.setRight(mousePos.x());
                m_rubberBand->setGeometry(rect);
                m_rubberBand->show();
            }
            else {
                if (itemViewIndex >= 0) {
                    changeActivePropertyByIndex(itemViewIndex);
                    m_mouseCaptured = handleMouseEvent(itemViewIndex, e, mousePos);
                }
            }
            QAbstractScrollArea::mousePressEvent(e);
            break;
        }
        case Qt::RightButton: {
            const auto property = getPropertyAt(mousePos);
            setActiveProperty(property, true);
            QAbstractScrollArea::mousePressEvent(e);
            break;
        }
        default: {
            QAbstractScrollArea::mousePressEvent(e);
        }
    }
}

void LC_PropertiesSheet::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton) {
        QAbstractScrollArea::mouseReleaseEvent(e);
        return;
    }
    if (m_skipNextMouseReleaseEvent) {
        // we may skip if rebuild is initiated by link
        m_skipNextMouseReleaseEvent = false;
        QAbstractScrollArea::mouseReleaseEvent(e);
        m_mouseCaptured = false;
        return;
    }

    if (m_rubberBand != nullptr) {
        m_rubberBand = nullptr;
        // update split ratio
        const QRect rect = viewport()->rect();
        int mouseX = e->pos().x();
        correctMousePositionForSplitByMinMargin(mouseX, rect);
        const float delta = mouseX - rect.left();
        const float splitRatio = delta / rect.width();
        updateSplitRatio(splitRatio);
    }
    else {
        const auto mousePos = e->pos();
        handleMouseEvent(getItemViewIndex(mousePos), e, mousePos);
        emit mouseReleased(e);
    }

    QAbstractScrollArea::mouseReleaseEvent(e);
    m_mouseCaptured = false;
}

void LC_PropertiesSheet::mouseMoveEvent(QMouseEvent* e) {
    const auto mousePos = e->pos();
    int mouseX = mousePos.x();
    if (m_rubberBand != nullptr) {
        if (e->buttons() == Qt::LeftButton) {
            QRect rect = viewport()->rect();
            correctMousePositionForSplitByMinMargin(mouseX, rect);

            rect.setLeft(mouseX);
            rect.setRight(mouseX);
            m_rubberBand->setGeometry(rect);

            if ((m_style & PropertiesSheetStyleLiveSplit) != 0u) {
                // update split ratio
                const QRect viewportRect = viewport()->rect();
                const int delta = mouseX - viewportRect.left();
                updateSplitRatio(static_cast<float>(delta) / viewportRect.width());
            }
        }
    }
    else {
        const int itemViewIndex = getItemViewIndex(mousePos);
        bool isSplittable = false;
        if (itemViewIndex >= 0) {
            const auto propertyItemView = m_itemViews.at(itemViewIndex);
            isSplittable = propertyItemView.isSplittable();
        }
        const bool atSplitterPos = isSplittable && qAbs(mouseX - splitPosition()) < MOUSE_MAX_DISTANCE_FROM_SPLIT;
        if (!handleMouseEvent(itemViewIndex, e, mousePos)) {
            if (atSplitterPos) {
                if (!m_mouseAtSplitter) {
                    m_mouseAtSplitter = true;
                    setCursor(Qt::SplitHCursor);
                }
            }

            if ((e->buttons() & Qt::LeftButton) != 0u) {
                changeActivePropertyByIndex(itemViewIndex);
            }
        }
        if (!atSplitterPos && m_mouseAtSplitter) {
            m_mouseAtSplitter = false;
            unsetCursor();
        }
    }
    QAbstractScrollArea::mouseMoveEvent(e);
}

void LC_PropertiesSheet::correctMousePositionForSplitByMinMargin(int& mouseX, const QRect rect) const {
    const int distance = mouseX - rect.left();
    if (distance < m_minSplitMargin) {
        mouseX = rect.left() + m_minSplitMargin;
    }
    else if ((rect.right() - mouseX) < m_minSplitMargin) {
        mouseX = rect.right() - m_minSplitMargin;
    }
}

void LC_PropertiesSheet::mouseDoubleClickEvent(QMouseEvent* e) {
    if (m_rubberBand == nullptr) {
        const auto mousePos = e->pos();
        handleMouseEvent(getItemViewIndex(mousePos), e, mousePos);
        QAbstractScrollArea::mouseDoubleClickEvent(e);
    }
}

bool LC_PropertiesSheet::viewportEvent(QEvent* e) {
    switch (e->type()) {
        case QEvent::StyleChange: {
            updateStylingVars();
            break;
        }
        case QEvent::ToolTip: {
            auto* helpEvent = static_cast<QHelpEvent*>(e);
            tooltipEvent(helpEvent);
            break;
        }
        case QEvent::Leave: {
            deactivateViewParts();
            break;
        }
        default:
            break;
    }
    return QAbstractScrollArea::viewportEvent(e);
}

void LC_PropertiesSheet::scrollContentsBy(const int dx, const int dy) {
    if (dx != 0 || dy != 0) {
        stopInplaceEdit();
        invalidateViewParts();
    }
    QAbstractScrollArea::scrollContentsBy(dx, dy);
}

void LC_PropertiesSheet::keyPressEvent(QKeyEvent* e) {
    validateItemViews();
    if (m_itemViews.empty()) {
        QAbstractScrollArea::keyPressEvent(e);
        return;
    }
    const auto inplaceEditor = m_inplaceController.getInplaceEdit();
    if (inplaceEditor != nullptr) {
        const int key = e->key();
        if (key == Qt::Key_Escape || key == Qt::Key_Return || key == Qt::Key_Enter) {
            stopInplaceEdit();
            e->accept();
        }
        return;
    }
    const int activePropertyViewIndex = getItemViewIndex(activeProperty());
    switch (e->key()) {
        case Qt::Key_Home: {
            // first item
            changeActivePropertyByIndex(0);
            break;
        }
        case Qt::Key_End: {
            // last item
            changeActivePropertyByIndex(m_itemViews.size() - 1);
            break;
        }
        case Qt::Key_Up: {
            // previous item
            if (activePropertyViewIndex < 0) {
                changeActivePropertyByIndex(0);
            }
            else {
                changeActivePropertyByIndex(qMax(0, activePropertyViewIndex - 1));
            }
            break;
        }
        case Qt::Key_Down: {
            // next item
            if (activePropertyViewIndex < 0) {
                changeActivePropertyByIndex(0);
            }
            else {
                changeActivePropertyByIndex(qMin(m_itemViews.size() - 1, activePropertyViewIndex + 1));
            }
            break;
        }
        case Qt::Key_PageUp: {
            // previous page
            if (activePropertyViewIndex < 0) {
                changeActivePropertyByIndex(0);
            }
            else {
                const int itemsPerPage = qMax(viewport()->rect().height() / m_itemHeight, 1);
                changeActivePropertyByIndex(qMax(0, activePropertyViewIndex - itemsPerPage));
            }
            break;
        }
        case Qt::Key_PageDown: {
            // page
            if (activePropertyViewIndex < 0) {
                changeActivePropertyByIndex(0);
            }
            else {
                const int itemsPerPage = qMax(viewport()->rect().height() / m_itemHeight, 1);
                changeActivePropertyByIndex(qMin(m_itemViews.size() - 1, activePropertyViewIndex + itemsPerPage));
            }
            break;
        }
        case Qt::Key_Left: {
            // parent item or collapse
            if (activePropertyViewIndex < 0) {
                changeActivePropertyByIndex(0);
            }
            else {
                const PropertyItemView& itemView = m_itemViews[activePropertyViewIndex];
                if (itemView.hasChildren && !itemView.propertyItem->collapsed()) {
                    // collapse opened property
                    itemView.propertyItem->property->addState(PropertyStateCollapsed);
                }
                else if (itemView.propertyItem->parentItem != nullptr) {
                    // activate parent property
                    setActiveProperty(itemView.propertyItem->parentItem->property, true);
                }
            }
            break;
        }
        case Qt::Key_Right: {
            // child item or expand
            if (activePropertyViewIndex < 0) {
                changeActivePropertyByIndex(0);
            }
            else {
                const PropertyItemView& itemView = m_itemViews[activePropertyViewIndex];
                if (itemView.hasChildren && itemView.propertyItem->collapsed()) {
                    // expand closed property
                    itemView.propertyItem->property->removeState(PropertyStateCollapsed);
                }
                else if (itemView.hasChildren) {
                    // activate child property
                    setActiveProperty(itemView.propertyItem->children.front()->property, true);
                }
            }
            break;
        }
        default: {
            if (activePropertyViewIndex >= 0) {
                LC_PropertyEventContext context{e, this};
                if (handleEvent(context, m_itemViews[activePropertyViewIndex], QPoint())) {
                    // just accept
                    e->accept();
                    return;
                }
            }
            // process by default
            QAbstractScrollArea::keyPressEvent(e);
        }
    }
}

void LC_PropertiesSheet::wheelEvent(QWheelEvent* e) {
    const QPoint pt(e->position().x(), e->position().y());
    if (!handleMouseEvent(getItemViewIndex(pt), e, pt)) {
        QAbstractScrollArea::wheelEvent(e);
    }
}

void LC_PropertiesSheet::tooltipEvent(QHelpEvent* e) {
    if (!handleMouseEvent(getItemViewIndex(e->pos()), e, e->pos())) {
        QToolTip::hideText();
    }
}

bool LC_PropertiesSheet::startInplaceEdit(QWidget* editor) {
    return m_inplaceController.startInplaceEdit(editor);
}

bool LC_PropertiesSheet::handleEvent(LC_PropertyEventContext& context, const PropertyItemView& itemView, const QPoint mousePos) {
    if (!itemView.viewPartsValid) {
        return false;
    }
    m_sheetModel.stopInvalidate(true);
    bool result = false;
    if (m_mouseEventsReceiverPart != nullptr) {
        result = m_mouseEventsReceiverPart->event(context);
    }
    else {
        result = false;
        QList<LC_PropertyViewPart*> activeParts;
        for (auto& part : itemView.viewParts) {
            if (mousePos.isNull() || part.rect.contains(mousePos)) {
                part.activate(this, mousePos);
                activeParts.append(&part);
            }
        }
        for (const auto part : std::as_const(m_activeViewParts)) {
            part->deactivate(this, mousePos);
        }
        m_activeViewParts.swap(activeParts);

        for (const auto part : std::as_const(m_activeViewParts)) {
            if (part->event(context)) {
                result = true;
                break;
            }
        }
    }
    m_sheetModel.stopInvalidate(false);
    return result;
}

bool LC_PropertiesSheet::grabMouseForPart(LC_PropertyViewPart* part, const QPoint mousePos) {
    Q_ASSERT(!m_mouseEventsReceiverPart);
    if (m_mouseEventsReceiverPart != nullptr) {
        return false;
    }

    viewport()->grabMouse();
    m_mouseEventsReceiverPart = part;
    m_mouseEventsReceiverPart->grabMouse(this, mousePos);
    return true;
}

bool LC_PropertiesSheet::releaseMouseForPart([[maybe_unused]] const LC_PropertyViewPart* part, const QPoint mousePos) {
    Q_ASSERT(m_mouseEventsReceiverPart == part);
    if (m_mouseEventsReceiverPart == nullptr) {
        return false;
    }
    m_mouseEventsReceiverPart->releaseMouse(this, mousePos);
    m_mouseEventsReceiverPart = nullptr;
    viewport()->releaseMouse();
    return true;
}

void LC_PropertiesSheet::updateItemsTree() {
    m_sheetModel.updateTree();
    invalidateItemViews();
}

void LC_PropertiesSheet::setActivePropertyInternal(LC_Property* property) {
    disconnectActiveProperty();

    m_activeProperty = property;
    emit activePropertyChanged(m_activeProperty);
    viewport()->update();

    connectActiveProperty();
}

void LC_PropertiesSheet::invalidateItemViews() {
    deactivateViewParts();
    m_itemViewsValid = false;
    m_itemViews.clear();
    viewport()->update();
}

void LC_PropertiesSheet::validateItemViews() const {
    if (m_itemViewsValid) {
        return;
    }

    fillItemViews(m_sheetModel.getRootItem(), ((m_style & PropertiesSheetStyleShowRoot) != 0u) ? 0 : -1);
    updateVerticalScrollbar();
    m_itemViewsValid = true;
}

void LC_PropertiesSheet::fillItemViews(LC_PropertiesSheetModel::PropertyItem* item, const int level) const {
    if (item == nullptr) {
        return;
    }
    if (level < 0) {
        for (auto& child : item->children) {
            fillItemViews(child.get(), level + 1);
        }
        return;
    }
    if (!isItemPropertyVisible(*item)) {
        return;
    }

    PropertyItemView vItem;
    vItem.propertyItem = item;
    vItem.level = level;

    if (item->collapsed()) {
        for (auto& child : item->children) {
            if (isItemPropertyVisible(*child.get())) {
                vItem.hasChildren = true;
                break;
            }
        }
        m_itemViews.append(vItem);
        return;
    }

    m_itemViews.append(vItem);

    // save just added item index
    const int index = m_itemViews.size() - 1;
    for (auto& child : item->children) {
        fillItemViews(child.get(), level + 1);
    }

    if (index < (m_itemViews.size() - 1)) {
        m_itemViews[index].hasChildren = true;
    }
}

bool LC_PropertiesSheet::isItemPropertyVisible(const LC_PropertiesSheetModel::PropertyItem& item) const {
    return item.property->isVisible();
}

void LC_PropertiesSheet::updateVerticalScrollbar() const {
    const int viewportHeight = viewport()->height();
    const int virtualHeight = m_itemHeight * m_itemViews.size();

    const auto scrollBar = verticalScrollBar();
    scrollBar->setSingleStep(m_itemHeight);
    scrollBar->setPageStep(viewportHeight);
    scrollBar->setRange(0, qMax(0, virtualHeight - viewportHeight + 2));
}

void LC_PropertiesSheet::updateStylingVars() {
    const QFontMetrics fm(font());
    const int fontHeight = fm.height();
    m_itemHeight = fontHeight + m_itemHeightSpacing;
    m_linesColor = palette().color(QPalette::Button);
    m_propertySetBackgroundColor = m_linesColor;
    m_valueLeftMargin = style()->pixelMetric(QStyle::PM_ButtonMargin);
}

bool LC_PropertiesSheet::ensureItemViewVisible(const int index) const {
    if (index < 0) {
        return false;
    }

    const int itemViewTop = index * m_itemHeight;
    const int itemViewBottom = itemViewTop + m_itemHeight;
    const QRect rect = viewport()->rect();
    int scrollPos = getVerticalScrollBarValue();
    if (itemViewTop < scrollPos) {
        scrollPos = itemViewTop;
    }
    else if (itemViewBottom > scrollPos + rect.height()) {
        scrollPos = itemViewBottom - rect.height();
    }
    else {
        return false;
    }

    verticalScrollBar()->setValue(scrollPos);
    return true;
}

void LC_PropertiesSheet::invalidateViewParts() {
    deactivateViewParts();
    for (auto& itemView : m_itemViews) {
        itemView.viewPartsValid = false;
        itemView.viewParts.clear();
    }
}

void LC_PropertiesSheet::deactivateViewParts() {
    if (m_mouseEventsReceiverPart != nullptr) {
        viewport()->releaseMouse();
        m_mouseEventsReceiverPart = nullptr;
    }
    for (const auto part : std::as_const(m_activeViewParts)) {
        part->deactivate(this, QPoint());
    }
    m_activeViewParts.clear();
    QToolTip::hideText();
}

int LC_PropertiesSheet::splitPosition() const {
    return static_cast<int>(viewport()->rect().width() * m_splitRatio);
}

void LC_PropertiesSheet::updateSplitRatio(const float splitRatio) {
    m_splitRatio = qBound(0.F, splitRatio, 1.F);
    invalidateViewParts();
    viewport()->update();
}

void LC_PropertiesSheet::connectActiveProperty() {
    if (m_activeProperty != nullptr) {
        connect(m_activeProperty, &QObject::destroyed, this, &LC_PropertiesSheet::onActivePropertyDestroyed);
    }
}

void LC_PropertiesSheet::disconnectActiveProperty() {
    if (m_activeProperty != nullptr) {
        disconnect(m_activeProperty, &QObject::destroyed, this, &LC_PropertiesSheet::onActivePropertyDestroyed);
    }
}

void LC_PropertiesSheet::onModelChanged() {
    invalidateItemViews();
}

void LC_PropertiesSheet::onModelDataChanged() const {
    viewport()->update();
}

void LC_PropertiesSheet::onPropertyDidChange(const LC_PropertyChangeReason reason,
                                             [[maybe_unused]] LC_PropertiesSheetModel::PropertyItem* item) {
    emit propertiesChanged(reason);
}

LC_PropertiesSheet::PropertyItemView::PropertyItemView()
    : propertyItem(nullptr), level(0), hasChildren(false), viewPartsValid(false) {
}

void LC_PropertiesSheet::onPropertyContainerDestroyed() {
    m_sheetModel.setPropertyContainer(nullptr);
    m_inTreeRebuild = true;
    updateItemsTree();
    m_inTreeRebuild = false;
}

LC_SheetPainterGuard::LC_SheetPainterGuard(QPainter& p)
    : m_p(p) {
    m_p.save();
}

LC_SheetPainterGuard::~LC_SheetPainterGuard() {
    m_p.restore();
}
