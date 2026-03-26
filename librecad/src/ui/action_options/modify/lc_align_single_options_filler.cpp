/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
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

#include "lc_align_single_options_filler.h"

#include "lc_action_modify_align_single.h"

void LC_AlignSingleOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
        auto action = static_cast<LC_ActionModifyAlignSingle*>(m_action);

    static LC_EnumDescriptor alignToDescriptor = {
        "alignTypeDescriptor",
        {
                {LC_Align::AlignMode::ENTITY, tr("Entity")},
                {LC_Align::AlignMode::DRAWING, tr("Drawing")},
                {LC_Align::AlignMode::POSITION, tr("Position")}
        }
    };

    addEnum({"a_alignMode", tr("Align To"), tr("Defines the way of selected entities alignment")}, &alignToDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getAlignType();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setAlignType(v);
            }, container);



    static LC_EnumDescriptor h_alignDescriptor = {
        "halignDescriptor",
        {
                    {LC_Align::Align::NONE, tr("None")},
                    {LC_Align::Align::LEFT_TOP, tr("Left")},
                    {LC_Align::Align::MIDDLE, tr("Middle")},
                    {LC_Align::Align::RIGHT_BOTTOM, tr("Right")}
        }
    };

    addEnum({"a_halignMode", tr("Horizontal"), tr("Mode of horizontal align")}, &h_alignDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getHAlign();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setHAlign(v);
            }, container);

    static LC_EnumDescriptor v_alignDescriptor = {
        "valignDescriptor",
        {
                        {LC_Align::Align::NONE, tr("None")},
                        {LC_Align::Align::LEFT_TOP, tr("Top")},
                        {LC_Align::Align::MIDDLE, tr("Middle")},
                        {LC_Align::Align::RIGHT_BOTTOM, tr("Bottom")}
        }
    };

    addEnum({"a_valignMode", tr("Vertical"), tr("Mode of vertical align")}, &v_alignDescriptor,
            [action]() -> LC_PropertyEnumValueType {
                return action->getVAlign();
            }, [action](const LC_PropertyEnumValueType& v)-> void {
                action->setVAlign(v);
            }, container);
}
