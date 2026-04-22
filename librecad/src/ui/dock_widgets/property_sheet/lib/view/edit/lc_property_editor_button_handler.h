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

#ifndef LC_PROPERTYEDITORBUTTONHANDLER_H
#define LC_PROPERTYEDITORBUTTONHANDLER_H

#include <QMouseEvent>
#include <QToolButton>

#include "lc_property_editor_handler.h"

template <typename PropertyClass, typename PropertyEditorClass>
class LC_PropertyEditorButtonHandler : public LC_PropertyEditorHandler<PropertyClass, PropertyEditorClass> {
protected:
    using Inherited = LC_PropertyEditorHandler<PropertyClass, PropertyEditorClass>;

    LC_PropertyEditorButtonHandler(LC_PropertyViewEditable* view, PropertyEditorClass& editor, QToolButton* button = nullptr)
        : Inherited(view, editor), m_toolButton{button} {
    }

    virtual void doOnToolButtonClick() = 0;

    bool eventFilter(QObject* obj, QEvent* event) override {
        if (this->isEditableByUser()) {
            switch (event->type()) {
                case QEvent::MouseButtonDblClick: {
                    m_mouseDoubleClick = true;
                    return true;
                }
                case QEvent::MouseButtonRelease: {
                    if (m_mouseDoubleClick) {
                        m_mouseDoubleClick = false;
                        doOnToolButtonClick();
                        return true;
                    }
                    break;
                }
                case QEvent::MouseButtonPress: {
                    if (this->m_toolButton != nullptr) {
                        const auto mouseEvent = static_cast<QMouseEvent*>(event);
                        if (mouseEvent->button() == Qt::LeftButton) {
                            auto maybeButton = dynamic_cast<QToolButton*>(obj);
                            if (maybeButton != nullptr) {
                                if (this->m_toolButton == maybeButton) {
                                    doOnToolButtonClick();
                                    return true;
                                }
                            }
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
        return Inherited::eventFilter(obj, event);
    }

private:
    bool m_mouseDoubleClick{false};
    QToolButton* m_toolButton{nullptr};
};

#endif
