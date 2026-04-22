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

#include "lc_propertiesprovider_image.h"

#include <QFileDialog>

#include "lc_property_qstring_file_view.h"
#include "lc_property_rsvector_view.h"
#include "rs_image.h"
#include "rs_units.h"

void LC_PropertiesProviderImage::doCreateEntitySpecificProperties(LC_PropertyContainer* cont, const QList<RS_Entity*>& list) {
    const auto contGeometry = createGeometrySection(cont);

    // fixme - prop -  add errors processing!
    add<RS_Image>({"file", tr("File"), tr("Name of image file")},
                  [this](const LC_Property::Names& names, RS_Image* entity, LC_PropertyContainer* container,
                         QList<LC_PropertyAtomic*>* props) -> void {
                      auto* property = new LC_PropertyQString(container, false);
                      property->setNames(names);
                      props->push_back(property);

                      LC_PropertyViewDescriptor descriptor(LC_PropertyQStringFileView::VIEW_NAME);
                      descriptor.attributes[LC_PropertyQStringFileView::ATTR_FILE_MODE] = QFileDialog::FileMode::ExistingFile;

                      const QStringList filters({"Image files (*.png *.gif *.jpg *.jpeg *.svg *.bmp)"});
                      descriptor.attributes[LC_PropertyQStringFileView::ATTR_FILENAME_FILTERS] = filters;
                      // fixme - prop - image file - add file filters for image
                      property->setViewDescriptor(descriptor);

                      const auto valueStorage = new LC_EntityPropertyValueDelegate<QString, RS_Image>();
                      valueStorage->setup(entity, m_widget, [](const RS_Image* e) -> QString {
                                              QString value = e->getFile();
                                              return value;
                                          }, [](const QString& value, [[maybe_unused]] LC_PropertyChangeReason reason,
                                                RS_Image* e) -> void {
                                              e->setFile(value);
                                          }, [](const QString& v, const RS_Image* e) -> bool {
                                              return v == e->getFile();
                                          });
                      property->setValueStorage(valueStorage, true);
                  }, list, contGeometry);

    addVector<RS_Image>({"insert", tr("Insertion Point"), tr("Point of image insertion")}, [](const RS_Image* e) -> RS_Vector {
                            return e->getInsertionPoint();
                        }, [](const RS_Vector& v, RS_Image* e) -> void {
                            e->setInsertionPoint(v);
                        }, list, contGeometry);

    add<RS_Image>({"scale", tr("Scale"), tr("Scale factor for image")}, [this](const LC_Property::Names& n, RS_Image* entity, LC_PropertyContainer* container,
                                                   QList<LC_PropertyAtomic*>* props) -> void {
        const auto property = createVectorProperty(n, props, container, m_actionContext, m_widget);
        property->setInteractiveInputType(LC_ActionContext::InteractiveInputInfo::NOTNEEDED);
        auto funGet = [](RS_Image* e) ->RS_Vector {
            const double xScale = e->getUVector().magnitude();
            const double yScale = e->getVVector().magnitude();
            return RS_Vector(xScale, yScale);
        };

        auto funSet = [](const RS_Vector& v, RS_Image* e) {
            const double originalScaleX = e->getUVector().magnitude();
            const double originalScaleY = e->getVVector().magnitude();

            const double newScaleX = v.getX();
            const double newScaleY = v.getY();

            const double scaleX  = newScaleX / originalScaleX;
            const double scaleY  = newScaleY / originalScaleY;

            e->scale(e->getInsertionPoint(), RS_Vector(scaleX, scaleY));
        };

        createDelegatedStorage(funGet, funSet, [funGet](RS_Vector& scale, RS_Image* e) -> bool {
                                   const auto original = funGet(e);
                                   return scale == original;
                               }, entity, property);
    }, list, contGeometry);

    auto funGetSize = [](const RS_Image* e)-> RS_Vector {
        const RS_Vector result = e->getData().size;
        return result;
    };

    add<RS_Image>({"sizePx", tr("Size, pixels"), tr("Size of the image in pixels")},
                  [this, funGetSize](const LC_Property::Names& n, RS_Image* e, LC_PropertyContainer* container,
                                     QList<LC_PropertyAtomic*>* props) -> void {
                      const auto property = createVectorProperty(n, props, container, m_actionContext, m_widget);
                      property->setInteractiveInputType(LC_ActionContext::InteractiveInputInfo::NOTNEEDED);
                      const LC_PropertyViewDescriptor descriptor = {
                          {
                              {LC_PropertyRSVectorView::ATTR_X_DISPLAY_NAME, tr("Width")},
                              {LC_PropertyRSVectorView::ATTR_Y_DISPLAY_NAME, tr("Height")},
                              {LC_PropertyRSVectorView::ATTR_X_DESCRIPTION, tr("Width of the image in pixels")},
                              {LC_PropertyRSVectorView::ATTR_Y_DESCRIPTION, tr("Height of the image in pixels")},
                              {LC_PropertyRSVectorView::ATTR_FORMAT_AS_INT, true},
                              {"x", QVariantMap{{LC_PropertyDoubleInteractivePickView::ATTR_FORMAT_AS_INT, true}}},
                              {"y", QVariantMap{{LC_PropertyDoubleInteractivePickView::ATTR_FORMAT_AS_INT, true}}}
                          }
                      };
                      property->setViewDescriptor(descriptor);

                      const auto valueStorage = new LC_EntityPropertyValueDelegate<RS_Vector, RS_Image>();
                      property->setValueStorage(valueStorage, true);
                      const typename LC_EntityPropertyValueDelegate<RS_Vector, RS_Image>::FunValueSetShort funSet = nullptr;
                      valueStorage->setup(e, m_widget, funGetSize, funSet, nullptr);
                      property->setReadOnly();
                  }, list, contGeometry);

    auto funGetImageSize = [](RS_Image* e) -> RS_Vector {
        return e->getImageSize();
    };

    auto funSetImageSize = [](const RS_Vector& size, RS_Image* e) -> void {
        const RS_Vector originalSize = e->getImageSize();
        const double scaleX = size.x / originalSize.x;
        const double scaleY = size.y / originalSize.y;
        e->scale(e->getInsertionPoint(), RS_Vector(scaleX, scaleY));
    };

    add<RS_Image>({"size", tr("Size"), tr("Size in drawing units")},
                [this, funGetImageSize, funSetImageSize](const LC_Property::Names& n, RS_Image* e, LC_PropertyContainer* container,
                                   QList<LC_PropertyAtomic*>* props) -> void {
                    const auto property = createVectorProperty(n, props, container, m_actionContext, m_widget);
                    const LC_PropertyViewDescriptor descriptor = {
                        {
                            {LC_PropertyRSVectorView::ATTR_X_DISPLAY_NAME, tr("Width")},
                            {LC_PropertyRSVectorView::ATTR_Y_DISPLAY_NAME, tr("Height")},
                            {LC_PropertyRSVectorView::ATTR_X_DESCRIPTION, tr("Width of the image in drawing units")},
                            {LC_PropertyRSVectorView::ATTR_Y_DESCRIPTION, tr("Height of the image in drawing units")},
                            {LC_PropertyRSVectorView::ATTR_FORMAT_AS_INT, true}
                        }
                    };
                    property->setViewDescriptor(descriptor);
                    const auto valueStorage = new LC_EntityPropertyValueDelegate<RS_Vector, RS_Image>();
                    property->setValueStorage(valueStorage, true);
                    valueStorage->setup(e, m_widget, funGetImageSize, funSetImageSize, nullptr);
                }, list, contGeometry);

    auto funGetDpi = [this](RS_Image* e) ->RS_Vector {
        const RS_Vector scale = e->getScale();
        const double dpiX = RS_Units::scaleToDpi(scale.x, getFormatter()->getUnit());
        const double dpiY = RS_Units::scaleToDpi(scale.y, getFormatter()->getUnit());
        return RS_Vector(dpiX, dpiY);
    };

    add<RS_Image>({"dpi", tr("DPI"), tr("Dots per inch for the image")},
                [this, funGetDpi](const LC_Property::Names& n, RS_Image* e, LC_PropertyContainer* container,
                                   QList<LC_PropertyAtomic*>* props) -> void {
                    const auto property = createVectorProperty(n, props, container, m_actionContext, m_widget);
                    const LC_PropertyViewDescriptor descriptor = {
                        {
                            {LC_PropertyRSVectorView::ATTR_X_DISPLAY_NAME, tr("By width")},
                            {LC_PropertyRSVectorView::ATTR_Y_DISPLAY_NAME, tr("By height")},
                            {LC_PropertyRSVectorView::ATTR_X_DESCRIPTION, tr("DPI by width direction of image")},
                            {LC_PropertyRSVectorView::ATTR_Y_DESCRIPTION, tr("DPI by height direction of image")}
                        }
                    };
                    property->setViewDescriptor(descriptor);
                    const auto valueStorage = new LC_EntityPropertyValueDelegate<RS_Vector, RS_Image>();
                    property->setValueStorage(valueStorage, true);
                    const typename LC_EntityPropertyValueDelegate<RS_Vector, RS_Image>::FunValueSetShort funSetDPI = nullptr;
                    valueStorage->setup(e, m_widget, funGetDpi, funSetDPI, nullptr);
                    property->setReadOnly(true);
                }, list, contGeometry);


    addWCSAngle<RS_Image>({"angle", tr("Angle"), tr("Image rotation angle")}, [](const RS_Image* e) -> double {
                              return e->getUVector().angle();
                          }, [this](const double& v, RS_Image* e) -> void {
                              const double angle = toWCSAngle(v);
                              const double orgAngle = e->getUVector().angle();
                              e->rotate(e->getInsertionPoint(), angle - orgAngle);
                          }, list, contGeometry);

}

void LC_PropertiesProviderImage::fillComputedProperites([[maybe_unused]] LC_PropertyContainer* container,
                                                        [[maybe_unused]] const QList<RS_Entity*>& entitiesList) {
}

void LC_PropertiesProviderImage::fillSingleEntityCommands([[maybe_unused]] LC_PropertyContainer* container,
                                                          [[maybe_unused]] const QList<RS_Entity*>& entitiesList) {
}
