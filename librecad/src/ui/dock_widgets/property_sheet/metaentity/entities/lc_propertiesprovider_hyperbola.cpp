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

#include "lc_propertiesprovider_hyperbola.h"

#include "lc_hyperbola.h"

void LC_PropertiesProviderHyperbola::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(container);

    addVector<LC_Hyperbola>({"center", tr("Center"), tr("Center point of hyperbola")}, [](const LC_Hyperbola* e) -> RS_Vector {
                              return e->getCenter();
                          }, [](const RS_Vector& v, LC_Hyperbola* e) -> void {
                              e->setCenter(v);
                          }, list, contGeometry);

    addVector<LC_Hyperbola>({"focus1", tr("Focus 1"), tr("First focus point of hyperbola")}, [](const LC_Hyperbola* e) -> RS_Vector {
                          return e->getFocus1();
                      }, [](const RS_Vector& v, LC_Hyperbola* e) -> void {
                          e->setFocus1(v);
                      }, list, contGeometry);

    addVector<LC_Hyperbola>({"focus2", tr("Focus 2"), tr("Second focus point of hyperbola")}, [](const LC_Hyperbola* e) -> RS_Vector {
                      return e->getFocus2();
                  }, [](const RS_Vector& v, LC_Hyperbola* e) -> void {
                      e->setFocus2(v);
                  }, list, contGeometry);

    addWCSAngle<LC_Hyperbola>({"angle", tr("Angle"), tr("Angle of the hyperbola major axis")}, [](const LC_Hyperbola* e) -> double {
                                return e->getAngle();
                            }, [](double& v, LC_Hyperbola* l) -> void {
                                // fixme - sand - move this to hyperbola entity?
                                // recalculate the majorP - by rotation old majorP point to delta between old and new angle
                                const double oldAngle = l->getAngle();
                                auto majorP = l->getMajorP();
                                const double angleDelta = oldAngle - v;
                                majorP.rotate(angleDelta);
                                l->setMajorP(majorP);
                            }, list, contGeometry);

    addLinearDistance<LC_Hyperbola>({"radiusMajor", tr("Radius Major"), tr("Major radius of hyperbola")}, [](const LC_Hyperbola* e) -> double {
                                      return e->getMajorRadius();
                                  }, [](double& v, LC_Hyperbola* l) -> void {
                                      // fixme - sand - move this to hyperbola entity?
                                      // recalculate the majorP - by changing length of the vector to new radius, but with staying on the direction
                                      const double angle = l->getAngle();
                                      const auto majorP = l->getCenter().relative(v, angle);
                                      l->setMajorP(majorP);
                                  }, list, contGeometry);

    addLinearDistance<LC_Hyperbola>({"radiusMinor", tr("Radius Minor"), tr("Minor radius of hyperbola")}, [](const LC_Hyperbola* e) -> double {
                                      return e->getMinorRadius();
                                  }, [](double& v, LC_Hyperbola* l) -> void {
                                       l->setMinorRadius(v);
                                  }, list, contGeometry);

    addLinearDistance<LC_Hyperbola>({"radiusRatio", tr("Radius Ratio"), tr("Radius ratio of hyperbola")}, [](const LC_Hyperbola* e) -> double {
                                      return e->getRatio();
                                  }, [](const double& v, LC_Hyperbola* l) -> void {
                                      l->setRatio(v);
                                  }, list, contGeometry);

    addWCSAngle<LC_Hyperbola>({"angle1", tr("Start Angle"), tr("Start angle of hyperbola")}, [](const LC_Hyperbola* e) -> double {
                                return e->getAngle1();
                            }, [](const double& v, LC_Hyperbola* l) -> void {
                                l->setAngle1(v);
                            }, list, contGeometry);

    addWCSAngle<LC_Hyperbola>({"angle2", tr("End Angle"), tr("End angle of hyperbola")}, [](const LC_Hyperbola* e) -> double {
                                return e->getAngle2();
                            }, [](const double& v, LC_Hyperbola* l) -> void {
                                l->setAngle2(v);
                            }, list, contGeometry);

    addBoolean<LC_Hyperbola>({"reversed", tr("Is Reversed"), tr("Determines which focus is within hyperbola")},
                       [](const LC_Hyperbola* e) -> bool {
                           return e->isReversed();
                       }, [](const bool& v, LC_Hyperbola* e) -> void {
                           e->setReversed(v);
                       }, list, contGeometry);
}

void LC_PropertiesProviderHyperbola::doCreateCalculatedProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    addReadOnlyString<LC_Hyperbola>({"eccentricity", tr("Eccentricity"), tr("Ecentricity of hyperbola")}, [this](const LC_Hyperbola* e) -> QString {
        const double eccentricity = e->getEccentricity();
        QString value = formatLinear(eccentricity);
        return value;
    }, list, container);

    addReadOnlyString<LC_Hyperbola>({"circumference", tr("Circumference", "hyperbola"), tr("Circumference of hyperbola")},
                                  [this](const LC_Hyperbola* e) -> QString {
                                      const double len = e->getLength();
                                      QString value = formatLinear(len);
                                      return value;
                                  }, list, container);

    addReadOnlyString<LC_Hyperbola>({"area", tr("Area"), tr("Area of hyperbola")}, [this](const LC_Hyperbola* e) -> QString {
        const double area = e->areaLineIntegral();
        QString value = formatLinear(area);
        return value;
    }, list, container);

    addVector<LC_Hyperbola>({"start", tr("Start"), tr("Start point of hyperbola")}, [](const LC_Hyperbola* e) -> RS_Vector {
        return e->getStartpoint();
    }, nullptr, list, container);

    addVector<LC_Hyperbola>({"end", tr("End"), tr("End point of hyperbola")}, [](const LC_Hyperbola* e) -> RS_Vector {
        return e->getEndpoint();
    }, nullptr, list, container);
}

void LC_PropertiesProviderHyperbola::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto ellipse = static_cast<LC_Hyperbola*>(entity);
    const std::list<CommandLinkInfo> commands = {
        {
            tr("Dividing hyperbola or creation of bounding box"),
            {RS2::ActionModifyCut, tr("Divide"), tr("Divide hyperbola in given point")},
            {RS2::ActionDrawBoundingBox, tr("Bounding box"), tr("Creation of bounding box for hyperbola")}
        },
    };

    createEntityContextCommands<LC_Hyperbola>(commands, cont, ellipse, "hypCommands");
}
