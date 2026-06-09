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

#include "lc_propertiesprovider_ellipse.h"

#include "rs_ellipse.h"

class RS_Ellipse;

void LC_PropertiesProviderEllipse::doCreateEntitySpecificProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(container);

    addVector<RS_Ellipse>({"center", tr("Center"), tr("Center point of ellipse")}, [](const RS_Ellipse* e) -> RS_Vector {
                              return e->getCenter();
                          }, [](const RS_Vector& v, RS_Ellipse* e) -> void {
                              e->setCenter(v);
                          }, list, contGeometry);

    addWCSAngle<RS_Ellipse>({"angle", tr("Angle"), tr("Angle of the ellipse major axis")}, [](const RS_Ellipse* e) -> double {
                                return e->getAngle();
                            }, [](double& v, RS_Ellipse* e) -> void {
                                const double major = e->getMajorRadius();
                                // const double minor = e->getMinorRadius();
                                const double rotation = v;

                                e->setMajorP(RS_Vector::polar(major, rotation));
                                // e->setRatio(minor/major);
                            }, list, contGeometry);

    addLinearDistance<RS_Ellipse>({"radiusMajor", tr("Radius Major"), tr("Major radius of ellipse")}, [](const RS_Ellipse* e) -> double {
                                      return e->getMajorRadius();
                                  }, [](double& v, RS_Ellipse* e) -> void {
                                      double major = v;
                                      if (major < RS_TOLERANCE) {
                                          major = e->getMajorRadius();
                                      }
                                      const double minor = e->getMinorRadius();
                                      const double rotation = e->getMajorP().angle();
                                      e->setMajorP(RS_Vector::polar(major, rotation));
                                      e->setRatio(minor / major);
                                  }, list, contGeometry);

    addLinearDistance<RS_Ellipse>({"radiusMinor", tr("Radius Minor"), tr("Minor radius of ellipse")}, [](const RS_Ellipse* e) -> double {
                                      return e->getMinorRadius();
                                  }, [](double& v, RS_Ellipse* e) -> void {
                                      const double major = e->getMajorRadius();
                                      double minor = v;
                                      if (minor < RS_TOLERANCE) {
                                          minor = e->getMinorRadius();
                                      }
                                      e->setRatio(minor / major);
                                  }, list, contGeometry);

    addLinearDistance<RS_Ellipse>({"radiusRatio", tr("Radius Ratio"), tr("Radius ratio of ellipse")}, [](const RS_Ellipse* e) -> double {
                                      return e->getRatio();
                                  }, [](const double& v, RS_Ellipse* l) -> void {
                                      l->setRatio(v);
                                  }, list, contGeometry);

    addWCSAngle<RS_Ellipse>({"angle1", tr("Start Angle"), tr("Start angle of elliptic arc")}, [](const RS_Ellipse* e) -> double {
                                return e->getAngle1();
                            }, [](const double& v, RS_Ellipse* l) -> void {
                                l->setAngle1(v);
                            }, list, contGeometry);

    addWCSAngle<RS_Ellipse>({"angle2", tr("End Angle"), tr("End angle of elliptic arc")}, [](const RS_Ellipse* e) -> double {
                                return e->getAngle2();
                            }, [](const double& v, RS_Ellipse* l) -> void {
                                l->setAngle2(v);
                            }, list, contGeometry);
}

void LC_PropertiesProviderEllipse::doCreateCalculatedProperties(LC_PropertyContainer* container, const QList<RS_Entity*>& list) {
    addReadOnlyString<RS_Ellipse>({"circumference", tr("Circumference", "ellipse"), tr("Circumference of ellipse")},
                                  [this](const RS_Ellipse* e) -> QString {
                                      const double len = e->getLength();
                                      QString value = formatLinear(len);
                                      return value;
                                  }, list, container);

    addReadOnlyString<RS_Ellipse>({"area", tr("Area"), tr("Area of ellipse")}, [this](const RS_Ellipse* e) -> QString {
        const double area = e->areaLineIntegral();
        QString value = formatLinear(area);
        return value;
    }, list, container);

    addVector<RS_Ellipse>({"start", tr("Start"), tr("Start point of elliptic arc")}, [](const RS_Ellipse* e) -> RS_Vector {
        return e->getStartpoint();
    }, nullptr, list, container);

    addVector<RS_Ellipse>({"end", tr("End"), tr("End point of elliptic arc")}, [](const RS_Ellipse* e) -> RS_Vector {
        return e->getEndpoint();
    }, nullptr, list, container);
}

void LC_PropertiesProviderEllipse::doCreateSingleEntityCommands(LC_PropertyContainer* cont, RS_Entity* entity) {
    const auto ellipse = static_cast<RS_Ellipse*>(entity);
    const std::list<CommandLinkInfo> commands = {
        {
            tr("Creation of line tangental to ellipse"),
            {RS2::ActionDrawLineOrthogonal, tr("Orthogonal"), tr("Create line orthogonal to ellipse")},
            {RS2::ActionDrawLineOrthTan, tr("Tangent Orthogonal"), tr("Create tangental orthogonal line to line")},
        },
        {
            tr("Creation of line tangental to ellipse"),
            {RS2::ActionDrawLineTangent1, tr("Tangent (P,C)"), tr("Create line tangental to ellipse")},
            {RS2::ActionDrawLineTangent2, tr("Tangent (C,C)"), tr("Create line tangental for two entitiers")}
        },
        {
            tr("Creation of line with relative angle"),
            {RS2::ActionDrawLineAngleRel, tr("Relative"), tr("Create line with relative angle to ellipse")},
            {RS2::ActionModifyRound, tr("Fillet"), tr("Create fillet for ellipse")},
        },
        {
            tr("Dividing ellipse or creation of bounding box"),
            {RS2::ActionModifyCut, tr("Divide"), tr("Divide ellipse in given point")},
            {RS2::ActionDrawBoundingBox, tr("Bounding box"), tr("Create bounding box for ellipse")}
        },
    };

    createEntityContextCommands<RS_Ellipse>(commands, cont, ellipse, "ellipseCommands");
}

void LC_PropertiesProviderEllipse::doCreateSelectedSetCommands(LC_PropertyContainer* propertyContainer, const QList<RS_Entity*>& list) {
    LC_EntityTypePropertiesProvider::doCreateSelectedSetCommands(propertyContainer, list);
    const std::list<CommandLinkInfo> commands = {
        {
            tr("Center marks creation"),
            {RS2::ActionDrawCenterMark, tr("Center mark"), tr("Create center mark for selected circles, ellipses and arcs")}
        }
    };
    createEntityContextCommands<RS_Ellipse>(commands, propertyContainer, nullptr, "ellipseMultiCommands", false);
}
