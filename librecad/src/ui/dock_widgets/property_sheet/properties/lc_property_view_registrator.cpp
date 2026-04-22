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

#include "lc_property_view_registrator.h"

#include "lc_property_action.h"
#include "lc_property_action_link_view.h"
#include "lc_property_bool.h"
#include "lc_property_bool_checkbox_view.h"
#include "lc_property_bool_combobox_view.h"
#include "lc_property_container_view.h"
#include "lc_property_double_interactivepick_view.h"
#include "lc_property_double_line_edit_view.h"
#include "lc_property_double_spinbox_view.h"
#include "lc_property_enum_combobox_view.h"
#include "lc_property_int_list_combobox_view.h"
#include "lc_property_layer_combobox_view.h"
#include "lc_property_linetype_combobox_view.h"
#include "lc_property_linewidth_combobox_view.h"
#include "lc_property_qstring_file_view.h"
#include "lc_property_qstring_font_combobox_view.h"
#include "lc_property_qstring_list_arrows_combobox_view.h"
#include "lc_property_qstring_list_combobox_view.h"
#include "lc_property_rect.h"
#include "lc_property_rect_view.h"
#include "lc_property_rscolor_combobox_view.h"
#include "lc_property_rsvector_view.h"
#include "lc_property_view_multiple.h"

class LC_PropertyQStringListArrowsComboboxView;
class LC_PropertyQStringLineEditView;
class LC_PropertyQString;
class LC_PropertyContainerView;

template <typename PropertyClass, typename PropertyViewClass>
void LC_PropertyViewRegistrator::defaultView(const QByteArray& viewName) {
    m_factory.registerViewDefault(&PropertyClass::staticMetaObject, &createViewForProperty<PropertyViewClass, PropertyClass>, viewName);
}

template <typename PropertyClass, typename PropertyViewClass>
void LC_PropertyViewRegistrator::anotherView(const QByteArray& viewName) {
    m_factory.registerView(&PropertyClass::staticMetaObject, &createViewForProperty<PropertyViewClass, PropertyClass>, viewName);
}

void LC_PropertyViewRegistrator::registerViews() {
    defaultView<LC_PropertyContainer, LC_PropertyContainerView>(LC_PropertyContainerView::VIEW_NAME);
    defaultView<LC_PropertyRSVector, LC_PropertyRSVectorView>(LC_PropertyRSVectorView::VIEW_NAME);
    defaultView<LC_PropertyInt, LC_PropertyIntSpinBoxView>(LC_PropertyIntSpinBoxView::VIEW_NAME);
    defaultView<LC_PropertyDouble, LC_PropertyDoubleSpinBoxView>(LC_PropertyDoubleSpinBoxView::VIEW_NAME);
    anotherView<LC_PropertyDouble, LC_PropertyDoubleInteractivePickView>(LC_PropertyDoubleInteractivePickView::VIEW_NAME);
    anotherView<LC_PropertyDouble, LC_PropertyDoubleLineEditView>(LC_PropertyDoubleLineEditView::VIEW_NAME);
    defaultView<LC_PropertyMulti, LC_PropertyViewMultiple>(LC_PropertyViewMultiple::VIEW_NAME);
    defaultView<LC_PropertyLayer, LC_PropertyLayerComboBoxView>(LC_PropertyLayerComboBoxView::VIEW_NAME);
    defaultView<LC_PropertyQString, LC_PropertyQStringLineEditView>(LC_PropertyQStringLineEditView::VIEW_NAME);
    anotherView<LC_PropertyQString, LC_PropertyQStringListComboBoxView>(LC_PropertyQStringListComboBoxView::VIEW_NAME);
    anotherView<LC_PropertyQString, LC_PropertyQStringFontComboboxView>(LC_PropertyQStringFontComboboxView::VIEW_NAME);
    anotherView<LC_PropertyQString, LC_PropertyQStringFileView>(LC_PropertyQStringFileView::VIEW_NAME);
    anotherView<LC_PropertyQString, LC_PropertyQStringListArrowsComboboxView>(LC_PropertyQStringListArrowsComboboxView::VIEW_NAME);
    defaultView<LC_PropertyRSColor, LC_PropertyRSColorComboBoxView>(LC_PropertyRSColorComboBoxView::VIEW_NAME);
    defaultView<LC_PropertyLineType, LC_PropertyLineTypeComboboxView>(LC_PropertyLineTypeComboboxView::VIEW_NAME);
    defaultView<LC_PropertyLineWidth, LC_PropertyLineWidthComboboxView>(LC_PropertyLineWidthComboboxView::VIEW_NAME);
    defaultView<LC_PropertyBool, LC_PropertyBoolCheckBoxView>(LC_PropertyBoolCheckBoxView::VIEW_NAME);
    anotherView<LC_PropertyBool, LC_PropertyBoolComboBoxView>(LC_PropertyBoolComboBoxView::VIEW_NAME);
    defaultView<LC_PropertyAction, LC_PropertyActionLinkView>(LC_PropertyActionLinkView::VIEW_NAME);
    defaultView<LC_PropertyEnum, LC_PropertyEnumComboBoxView>(LC_PropertyEnumComboBoxView::VIEW_NAME);
    defaultView<LC_PropertyRect, LC_PropertyRectView>(LC_PropertyRectView::VIEW_NAME);
}
