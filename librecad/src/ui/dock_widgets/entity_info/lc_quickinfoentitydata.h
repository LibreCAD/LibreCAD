/****************************************************************************
*
* class that process given entity and creates the list of properties of it

Copyright (C) 2024 LibreCAD.org
Copyright (C) 2024 sand1024

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/
#ifndef LC_QUICKINFOENTITYDATA_H
#define LC_QUICKINFOENTITYDATA_H

#include <QString>
#include <QCoreApplication>
#include "rs_vector.h"
#include "rs_line.h"
#include "rs_circle.h"
#include "rs_arc.h"
#include "rs_polyline.h"
#include "rs_ellipse.h"
#include "rs_mtext.h"
#include "rs_image.h"
#include "rs_spline.h"
#include "rs_hatch.h"
#include "rs_leader.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimradial.h"
#include "rs_dimlinear.h"
#include "rs_dimaligned.h"
#include "rs_text.h"
#include "lc_quickinfobasedata.h"
#include "lc_peninforegistry.h"
#include "lc_splinepoints.h"
#include "lc_parabola.h"
#include "lc_dimarc.h"
#include "lc_quickinfowidgetoptions.h"

class LC_QuickInfoEntityData: public LC_QuickInfoBaseData
{
    Q_DECLARE_TR_FUNCTIONS(LC_QuickInfoEntityData)

public:

    explicit LC_QuickInfoEntityData();
    virtual ~LC_QuickInfoEntityData();

    /**
     * Type of property
     */
    enum PropertyType{
        VECTOR, // holds coordinate value
        LINEAR, // holds linear value
        ANGLE, // holds angle value
        AREA, // holds area value
        OTHER // holds generic purpose value
    };

    /**
     * Base property information
     */
    struct PropertyInfo{
        PropertyInfo(const QString &label, const QString &value, int type);
        QString label; // label/name of property
        QString value; // value
        int type; // type of property
    };

    /**
     * Property that holds of vector value
     */
    struct VectorPropertyInfo :public PropertyInfo{
        VectorPropertyInfo(const QString &label, const QString &value, int type, const RS_Vector& coord);
        RS_Vector data;
    };

    /**
     * Property that holds double value
     */
    struct DoublePropertyInfo: public PropertyInfo{
        DoublePropertyInfo(const QString &label, const QString &value, int type, double d);
        double data;
    };

    QString generateView();
    RS_Vector getVectorForIndex(int index) const override;
    QString getValue(int index);
    unsigned long getEntityId() const{return entityId;};
    bool updateForCoordinateViewMode(int mode) override;
    bool processEntity(RS_Entity *en);
    QString getEntityDescription(RS_Entity *en, RS2::EntityDescriptionLevel shortDescription);
    void clear() override;
    bool hasData() const override;
    void setOptions(LC_QuickInfoOptions *opt);
protected:
    /**
     * ID of entity for which properties are shown, 0 if no entity
     */
    unsigned long entityId = 0;
    unsigned long entityIdForDescription = 0;
    QString cachedEntityDescription = "";

    /**
     * Name of entity
     */
    QString entityName = "";
    /**
     * list of entity's properties
     */
    QVector<PropertyInfo*> properties;

    /**
     * Options
     */
    LC_QuickInfoOptions *options = nullptr;

    /**
     * Registry used for resolving names of visual attributes
     */
    LC_PenInfoRegistry* penRegistry = LC_PenInfoRegistry::instance();

    void addProperty(QString name, const QString &valueStr, PropertyType type);
    void collectLineProperties(RS_Line* line);
    void collectCircleProperties(RS_Circle *circle);
    void collectGenericProperties(RS_Entity *line);
    void collectArcProperties(RS_Arc *arc);
    void collectEllipseProperties(RS_Ellipse *ellipse);
    void collectPointProperties(RS_Point *point);
    void collectPolylineProperties(RS_Polyline *polyline);
    void collectInsertProperties(RS_Insert *insert);
    void collectMTextProperties(RS_MText *pText);
    void collectTextProperties(RS_Text *text);
    void collectImageProperties(RS_Image *image);
    void collectSplineProperties(RS_Spline *spline);
    void collectSplinePointsProperties(LC_SplinePoints *spline);
    void collectParabolaProperties(LC_Parabola *parabola);
    void collectHatchProperties(RS_Hatch *hatch);
    void collectDimLeaderProperties(RS_Leader *leader);
    void collectDimArcProperties(LC_DimArc *dim);
    void collectDimAngularProperties(RS_DimAngular *dim);
    void collectDimDiametricProperties(RS_DimDiametric *dim);
    void collectDimRadialProperties(RS_DimRadial *dim);
    void collectDimLinearProperties(RS_DimLinear *dim);
    void collectDimAlignedProperties(RS_DimAligned *dim);
    QString formatDouble(const double &x) const;
    QString formatInt(const int &x) const;
    static QString getHAlignStr(RS_TextData::HAlign align);
    static QString getVAlignStr(RS_TextData::VAlign align);
    static QString getTextGenerationStr(RS_TextData::TextGeneration generation);
    static QString getHAlignStr(RS_MTextData::HAlign align);
    static QString getVAlignStr(RS_MTextData::VAlign align);
    static QString getDirectionStr(RS_MTextData::MTextDrawingDirection direction);
    static QString getLineSpacingStyleStr(RS_MTextData::MTextLineSpacingStyle style);

    void addAngleProperty(const QString name, double value);
    void addLinearProperty(const QString name, double value, PropertyType type=LINEAR);
    void addAreaProperty(const QString name, double value);
    void addDoubleProperty(const QString name, const QString &valueStr, double value, PropertyType type);
    void addVectorProperty(const QString name,const RS_Vector &value, PropertyType type=VECTOR);
    void addVectorProperty(const QString name, const QString &valueStr, const RS_Vector& coord, PropertyType type=VECTOR);
    void addVectorProperty(const QString name, int count, const RS_Vector &value, PropertyType type=VECTOR);
    void addVectorProperty(QString name, int count, const QString &valueStr, const RS_Vector &coord, PropertyType type=VECTOR);


    QString prepareGenericEntityDescription(RS_Entity* en, const QString &entityName, RS2::EntityDescriptionLevel level);
    QString prepareLineDescription(RS_Line *line, RS2::EntityDescriptionLevel level);
    QString prepareCircleDescription(RS_Circle *line, RS2::EntityDescriptionLevel level);
    QString prepareArcDescription(RS_Arc *line, RS2::EntityDescriptionLevel level);
    QString prepareEllipseDescription(RS_Ellipse *line, RS2::EntityDescriptionLevel level);
    QString preparePointDescription(RS_Point *point, RS2::EntityDescriptionLevel level);
    QString preparePolylineDescription(RS_Polyline *polyline, RS2::EntityDescriptionLevel level);
    QString prepareInsertDescription(RS_Insert *insert, RS2::EntityDescriptionLevel level);
    QString prepareMTextDescription(RS_MText *pText, RS2::EntityDescriptionLevel level);
    QString prepareTextDescription(RS_Text *text, RS2::EntityDescriptionLevel level);
    QString prepareImageDescription(RS_Image *image, RS2::EntityDescriptionLevel level);
    QString prepareSplineDescription(RS_Spline *spline, RS2::EntityDescriptionLevel level);
    QString prepareSplinePointsDescription(LC_SplinePoints *spline, RS2::EntityDescriptionLevel level);
    QString prepareParabolaDescription(LC_Parabola *parabola, RS2::EntityDescriptionLevel level);
    QString prepareHatchDescription(RS_Hatch *hatch, RS2::EntityDescriptionLevel level);
    QString prepareDimLeaderDescription(RS_Leader *leader, RS2::EntityDescriptionLevel level);
    QString prepareDimArcDescription(LC_DimArc *dim, RS2::EntityDescriptionLevel level);
    QString prepareDimAngularDescription(RS_DimAngular *dim, RS2::EntityDescriptionLevel level);
    QString prepareDimDiametricDescription(RS_DimDiametric *dim, RS2::EntityDescriptionLevel level);
    QString prepareDimRadialDescription(RS_DimRadial *dim, RS2::EntityDescriptionLevel level);
    QString prepareDimLinearDescription(RS_DimLinear *dim, RS2::EntityDescriptionLevel level);
    QString prepareDimAlignedDescription(RS_DimAligned *dim, RS2::EntityDescriptionLevel level);


    void appendLinear(QString &result, const QString &label, double value);
    void appendDouble(QString &result, const QString &label, double value);
    void appendAngle(QString &result, const QString &label, double value);
    void appendArea(QString &result, const QString &label, double value);
    void appendAbsolute(QString &result, const QString &label, const RS_Vector& value);
    void appendRelativePolar(QString &result, const QString &label, const RS_Vector& value);
    void appendInt(QString &result, const QString &label, const int& value);
    void appendValue(QString &result, const QString &label, const QString& value);
};

#endif // LC_QUICKINFOENTITYDATA_H
