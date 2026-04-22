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

#include "lc_properties_sheet_panel.h"

#include <QLabel>
#include <QMouseEvent>
#include <QSplitter>
#include <QVBoxLayout>

#include "lc_properties_sheet.h"

class LC_SplitterDoubleClickEventsHandler : public QObject {
public:
    explicit LC_SplitterDoubleClickEventsHandler(QObject* parent);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
};

// It's safe to call this function on any platform.
// It will only have an effect on the Mac.
void setSmallerTextOSX(QWidget* w) {
    Q_ASSERT(w != nullptr);

    // By default, none of these size attributes are set.
    // If any has been set explicitly, we'll leave the widget alone.
    if (!w->testAttribute(Qt::WA_MacMiniSize) && !w->testAttribute(Qt::WA_MacSmallSize) && !w->testAttribute(Qt::WA_MacNormalSize)) {
        // make the text the 'normal' size
        w->setAttribute(Qt::WA_MacSmallSize);
    }
}

LC_PropertiesSheetPanel::LC_PropertiesSheetPanel(QWidget* parent)
    : QWidget(parent), m_areas(PROPERTY_WIDGET_AREA_NONE), m_layout(new QVBoxLayout(this)), m_toolbar(nullptr),
      m_propertiesSheet(new LC_PropertiesSheet(this)), m_descriptionSplitter(nullptr), m_propertyInfoLabel(nullptr) {
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    setSmallerTextOSX(this);

    connect(m_propertiesSheet, &LC_PropertiesSheet::activePropertyChanged, this, &LC_PropertiesSheetPanel::setActiveProperty);
    updateParts();
}

LC_PropertiesSheetPanel::~LC_PropertiesSheetPanel() {
    disconnect(m_propertiesSheet, &LC_PropertiesSheet::activePropertyChanged, this, &LC_PropertiesSheetPanel::setActiveProperty);
}

void LC_PropertiesSheetPanel::setParts(const LC_PropertyWidgetAreas newAreas) {
    if (m_areas == newAreas) {
        return;
    }
    m_areas = newAreas;
    updateParts();
}

const LC_PropertyContainer* LC_PropertiesSheetPanel::propertyContainer() const {
    return m_propertiesSheet->getPropertyContainer();
}

LC_PropertyContainer* LC_PropertiesSheetPanel::propertyContainer() {
    return m_propertiesSheet->getPropertyContainer();
}

void LC_PropertiesSheetPanel::setPropertyContainer(LC_PropertyContainer* newPropertiesContainer) {
    if (newPropertiesContainer == propertyContainer()) {
        return;
    }
    m_propertiesSheet->setPropertyContainer(newPropertiesContainer);
    emit propertySetChanged();
}

bool LC_PropertiesSheetPanel::stopInplaceEdit(const bool deleteLater, const bool restoreParentFocus) {
    return m_propertiesSheet->stopInplaceEdit(deleteLater, restoreParentFocus);
}

void LC_PropertiesSheetPanel::createDescriptionLabel(QSplitter* splitter) {
    m_propertyInfoLabel = new QLabel(splitter);
    m_propertyInfoLabel->setTextFormat(Qt::RichText);
    m_propertyInfoLabel->setAlignment(Qt::AlignTop);
    m_propertyInfoLabel->setWordWrap(true);
    m_propertyInfoLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);
    m_propertyInfoLabel->setMinimumSize(0, 5 * QFontMetrics(font()).height() / 2);
    QSizePolicy p = m_propertyInfoLabel->sizePolicy();
    p.setVerticalPolicy(QSizePolicy::Ignored);
    p.setHorizontalPolicy(QSizePolicy::Ignored);
    m_propertyInfoLabel->setSizePolicy(p);
}

void LC_PropertiesSheetPanel::updateParts() {
    while (!m_layout->isEmpty()) {
        m_layout->takeAt(0);
    }
    if (m_areas & PROPERTY_WIDGET_AREA_INFO) {
        if (m_descriptionSplitter == nullptr) {

            Q_ASSERT(!m_propertyInfoLabel);
            auto* splitter = new QSplitter(Qt::Vertical, this);

            splitter->addWidget(m_propertiesSheet);
            createDescriptionLabel(splitter);

            splitter->setStretchFactor(0, 1);
            splitter->setStretchFactor(1, 0);

            splitter->handle(1)->installEventFilter(new LC_SplitterDoubleClickEventsHandler(splitter));
            m_descriptionSplitter = splitter;
        }

        m_layout->addWidget(m_descriptionSplitter);
    }
    else {
        m_layout->addWidget(m_propertiesSheet);

        if (m_descriptionSplitter != nullptr) {
            delete m_descriptionSplitter;
            m_descriptionSplitter = nullptr;
            m_propertyInfoLabel = nullptr;
        }
    }
}

void LC_PropertiesSheetPanel::setActiveProperty(const LC_Property* activeProperty) const {
    if (m_propertyInfoLabel != nullptr) {
        if (activeProperty == nullptr) {
            m_propertyInfoLabel->setText(QString());
        }
        else {
            const auto &displayName = activeProperty->getDisplayName();
            const auto &description = activeProperty->getDescription();
            if (displayName.isEmpty()) {
                m_propertyInfoLabel->setText(description);
            }
            else {
                m_propertyInfoLabel->setText(QStringLiteral("<b>%1</b><br>%2").arg(displayName,description));
            }
        }
    }
}

LC_SplitterDoubleClickEventsHandler::LC_SplitterDoubleClickEventsHandler(QObject* parent)
    : QObject(parent) {
}

bool LC_SplitterDoubleClickEventsHandler::eventFilter(QObject* obj, QEvent* event) {
    // check input
    if (event->type() != QEvent::MouseButtonDblClick) {
        return false;
    }

    const auto mouseEvent = static_cast<QMouseEvent*>(event);
    if (mouseEvent->button() != Qt::LeftButton) {
        return false;
    }

    const auto splitterHandle = qobject_cast<QSplitterHandle*>(obj);
    if (splitterHandle == nullptr) {
        return false;
    }

    QSplitter* splitter = splitterHandle->splitter();
    if (splitter == nullptr) {
        return false;
    }

    if (splitter->count() < 2) {
        return false;
    }
    const QWidget* bottomWidget = splitter->widget(1);
    QList<int> sizes = splitter->sizes();
    if (sizes.size() != 2) {
        return false;
    }

    sizes[0] += sizes[1];
    sizes[1] = bottomWidget->heightForWidth(bottomWidget->size().width());
    sizes[0] -= qMax(sizes[1], 0);

    splitter->setSizes(sizes);

    return true;
}
