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

#include "lc_scale_options_filler.h"

#include "lc_action_modify_scale.h"

void LC_ScaleOptionsFiller::fillToolOptionsContainer(LC_PropertyContainer* container) {
    auto action = static_cast<LC_ActionModifyScale*>(m_action);

    addBoolean({"a_isotropic", tr("Isotropic"), tr("If checked, the same scaling factor for X and Y axis will be applied.")},
               [action]()-> bool {
                   return action->isIsotropicScaling();
               }, [action](bool val)-> void {
                   action->setIsotropicScaling(val);
               }, container);

    addBoolean({
                  "a_free",
                  tr("Free"),
                  tr("If checked, factor will be freely selected by mouse. Otherwise, specified values of factors will be used.")
              }, [action]()-> bool {
                  return action->isExplicitFactor();
              }, [action](bool val)-> void {
                  action->setExplicitFactor(val);
              }, container);

    addLinearDistance({"a_factorX", tr("Factor X"), tr("Scale factor for X axis")}, [action]() {
                          return action->getFactorX();
                      }, [action](double val) {
                          action->setFactorX(val);
                      }, container, [action](LC_PropertyViewDescriptor&) {
                          return action->isExplicitFactor();
                      });

    addLinearDistance({"a_factorY", tr("Factor Y"), tr("Scale factor for Y axis")}, [action]() {
                          return action->getFactorX();
                      }, [action](double val) {
                          action->setFactorX(val);
                      }, container, [action](LC_PropertyViewDescriptor&) {
                          return action->isExplicitFactor() || action->isIsotropicScaling();
                      });

    addBoolean({"a_keepOriginals", tr("Keep originals"), tr("If checked, original entities will survive, otherwise they will be removed")},
               [action]()-> bool {
                   return action->isKeepOriginals();
               }, [action](bool val)-> void {
                   action->setKeepOriginals(val);
               }, container);

    addBoolean({"a_multiCopy", tr("Multiple"), tr("If checked, multiple copies will be created")}, [action]()-> bool {
                   return action->isUseMultipleCopies();
               }, [action](bool val)-> void {
                   action->setUseMultipleCopies(val);
               }, container);

    addIntSpinbox({"a_number", tr("Copies"), tr("Number of copies to create")}, [action]()-> int {
                      return action->getCopiesNumber();
                  }, [action](int val)-> void {
                      action->setCopiesNumber(val);
                  }, container, 1, -1, [action](LC_PropertyViewDescriptor&) {
                      return !action->isUseMultipleCopies();
                  });

    addBoolean({
                   "a_currentAttrs",
                   tr("Current attributes"),
                   tr("If checked, current attributes will be applied to created entities, otherwise original ones will be used")
               }, [action]()-> bool {
                   return action->isUseCurrentAttributes();
               }, [action](bool val)-> void {
                   action->setUseCurrentAttributes(val);
               }, container);

    addBoolean({
                   "a_currentLayer",
                   tr("Current layer"),
                   tr("If checked, created entities will be placed into current layer, otherwise they will be in original layers")
               }, [action]()-> bool {
                   return action->isUseCurrentLayer();
               }, [action](bool val)-> void {
                   action->setUseCurrentLayer(val);
               }, container);
}
