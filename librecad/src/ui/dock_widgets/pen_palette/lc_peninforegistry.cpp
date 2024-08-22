/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 sand1024
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#include <QObject>
#include "lc_peninforegistry.h"

/**
 * internal value used for colors storing in pen palette storage to denote "unchanged" color value
 */
static const int UNCHANGED_COLOR_INTERNAL_VALUE = -3;

LC_PenInfoRegistry::LC_PenInfoRegistry()= default;

LC_PenInfoRegistry::~LC_PenInfoRegistry(){
    lineTypesIconMap.clear();
    lineTypesNamesMap.clear();
    lineWidthIconMap.clear();
    lineWidthNamesMap.clear();
    colorNamesMap.clear();
    standardLCColorNamesMap.clear();
}

/**
 * Returns display name for linetype
 * @param lineType linetype
 * @return string for UI
 */
QIcon LC_PenInfoRegistry::getLineTypeIcon(RS2::LineType lineType) const{
    QIcon result = lineTypesIconMap[lineType];
    return result;
}

/**
 * Returns display name for line width
 * @param lineType line width
 * @return string for UI
 */
QIcon LC_PenInfoRegistry::getLineWidthIcon(RS2::LineWidth lineWidth) const{
    QIcon result = lineWidthIconMap[lineWidth];
    return result;
}

/**
 * Returns name of linetype as it is shown in UI
 * @param lineType  line type
 * @return name to display
 */
QString LC_PenInfoRegistry::getLineTypeText(RS2::LineType lineType) const{
    QString result = lineTypesNamesMap[lineType];
    return result;
}

/**
 * Returns line width name for UI
 * @param lineWidth line width
 * @return  name to display
 */
QString LC_PenInfoRegistry::getLineWidthText(RS2::LineWidth lineWidth) const{
    QString result = lineWidthNamesMap[lineWidth];
    return result;
}

/**
 * Returns singletone instance of the registry
 * @return instance
 */
LC_PenInfoRegistry* LC_PenInfoRegistry::instance(){
    static LC_PenInfoRegistry* uniqueInstance;
    if (uniqueInstance == nullptr){
        uniqueInstance = new LC_PenInfoRegistry{};
        uniqueInstance->registerLineTypes();
        uniqueInstance->registerLineWidths();
        uniqueInstance->registerColorNames();
    }
    return uniqueInstance;
}

/**
 * Returns icon that should be shown for provided color. For predefined colors - returns cached items,
 * for other colors - creates icon with specified dimensions filled by provided color
 *
 * @param color color
 * @param iconSizeW icon width
 * @param iconSizeH icon height
 * @return icon for color
 */
QIcon LC_PenInfoRegistry::getColorIcon(const RS_Color &color, int iconSizeW, int iconSizeH) const{
    if (color.isByLayer()){
        return iconByLayer;
    }
    else if (color.isByBlock()){
        return iconByBlock;
    }
    else if (isUnchangedColor(color)){
        return iconUnchanged;
    }
    if (color.isValid()){
        QPixmap pixmap(iconSizeW, iconSizeH);
        pixmap.fill(color);
        return pixmap;
    }
    return {};
}

/**
 * Utility method that checks whether value of color is unchanged.
 * Unchanged color has flag RS2::FlagFrozen set.
 *
 * Todo - actually, this is good candidate for RS_Color itself,
 * however, not sure whether adding support of such flag on global layer
 * outside of pen palette widget will not break something...
 *
 * @param color color to test
 * @return  true if this is "unchanged" color
 */
bool LC_PenInfoRegistry::isUnchangedColor(const RS_Color &color){
    return color.getFlag(RS2::FlagFrozen);
}

/**
 * Creates color that used to mark "unchanged" color value
 * Internally, relies on RS2::FlagFrozen flag set
 * @return color
 */
RS_Color LC_PenInfoRegistry::createUnchangedColor(){
    return RS_Color(RS2::FlagFrozen);
}

/**
 * Re-creates color from internal string representation used for storing colors.
 * String represents just int value, with special meaning for "unchanged" color.
 * Internally, relies on "fromInColor" method of RS_Color
 * @param str string to parse
 * @return created color (will be invalid if not parsed properly)
 */
RS_Color LC_PenInfoRegistry::getColorFromInternalString(QString &str){
    bool conversionOk = false;
    int intColor = str.toInt(&conversionOk);
    RS_Color result;
    if (conversionOk){
        // were able to parse int value
        if (intColor == UNCHANGED_COLOR_INTERNAL_VALUE){
            // special handling of "unchanged"
            result = createUnchangedColor();
            return result;
        } else {
            // just rely on method from color
            result.fromIntColor(intColor);
            return result;
        }
    }    
    return result;
}

/**
 * Returns string representation of given color for further storing in persistence.
 * String contains int value of the color
 * @param color provided color
 * @return string representation
 */
QString LC_PenInfoRegistry::getInternalColorString(const RS_Color &color){
    int colorInt;
    if (isUnchangedColor(color)){
        colorInt = UNCHANGED_COLOR_INTERNAL_VALUE;
    }
    else{
        colorInt = color.toIntColor();
    }
    QString result =  QString::number(colorInt);
    return result;
}

/**
 * Returns name of the color that should be shown in UI.
 * Based on provided mode, name may represent:
 * HEX value, RGB value of color or mix (natural QT color name, if matched, or HEX)
 * For special color values (like "By Layer") - appropriate string is returned.
 * @param color given color
 * @param colorNameMode specifies how name of the color should be presented
 * @return name of color as it will be used in UI
 */
QString LC_PenInfoRegistry::getColorName(const RS_Color &color, int colorNameMode) const{
    if (color.isByLayer()){
        return QObject::tr("By Layer");
    }
    else if (color.isByBlock()){
        return QObject::tr("By Block");
    }
    else if (isUnchangedColor(color)){
        return QObject::tr("- Unchanged -");
    }
    switch (colorNameMode) {
        case LC_PenInfoRegistry::HEX:
            return color.name();
        case LC_PenInfoRegistry::RGB: {
            QString result = QString("rgb(%1,%2,%3)").arg(color.red(), 1, 10).arg(color.green(), 1, 10).arg(color.blue(), 1, 10);
            return result;
        }
        case LC_PenInfoRegistry::NATURAL: {
            // check where this is standard color first

            const QRgba64 &key = color.rgba64();
            if (standardLCColorNamesMap.contains(key)){
               return standardLCColorNamesMap.value(key, "");
            }
            else{
                // try to find qt name, if no such name - give up and return hex color name
                return colorNamesMap.value(key, color.name());
            }
        }
        default:
            break;
    }
    return QString();
}

/**
 * Method checks whether provided int correspond to one of valid registered line types
 * @param typeCandidate potential line type
 * @return true if this is valid line type value
 */
bool LC_PenInfoRegistry::hasLineType(int typeCandidate){
    RS2::LineType lineType = static_cast<RS2::LineType>(typeCandidate);
    return lineTypesNamesMap.contains(lineType);
}

/**
 * Method checks whether provided int correspond to one of valid registered line width values
 * @param typeCandidate potential line width
 * @return true if this is valid line width value
 */
bool LC_PenInfoRegistry::hasLineWidth(int widthCandidate){
    RS2::LineWidth lineWidth = static_cast<RS2::LineWidth>(widthCandidate);
    return lineWidthNamesMap.contains(lineWidth);
}

/**
 * Registration of all line widths
 */
void LC_PenInfoRegistry::registerLineWidths(){
    doRegisterLineWidth(":/icons/point_cross.svg", "- Unchanged -", RS2::LineWidth::WidthUnchanged);
//    registerLineWidth(":ui/width00.png", "- Unchanged -", RS2::LineWidth::WidthUnchanged);
    doRegisterLineWidth(":/icons/point_blank_square.svg", "By Layer", RS2::LineWidth::WidthByLayer);
//    registerLineWidth(":ui/width00.png", "By Layer", RS2::LineWidth::WidthByLayer);
    doRegisterLineWidth(":/icons/point_plus_square.svg", "By Block", RS2::LineWidth::WidthByBlock);
//    registerLineWidth(":ui/width00.png", "By Block", RS2::LineWidth::WidthByBlock);

    doRegisterLineWidth(":ui/width01.png", "Default", RS2::WidthDefault);
    doRegisterLineWidth(":ui/width01.png", "0.00mm", RS2::Width00);
    doRegisterLineWidth(":ui/width01.png", "0.05mm", RS2::Width01);
    doRegisterLineWidth(":ui/width01.png", "0.09mm", RS2::Width02);
    doRegisterLineWidth(":ui/width01.png", "0.13mm (ISO)", RS2::Width03);
    doRegisterLineWidth(":ui/width01.png", "0.15mm", RS2::Width04);
    doRegisterLineWidth(":ui/width01.png", "0.18mm (ISO)", RS2::Width05);
    doRegisterLineWidth(":ui/width01.png", "0.20mm", RS2::Width06);
    doRegisterLineWidth(":ui/width01.png", "0.25mm (ISO)", RS2::Width07);
    doRegisterLineWidth(":ui/width01.png", "0.30mm", RS2::Width08);
    doRegisterLineWidth(":ui/width03.png", "0.35mm (ISO)", RS2::Width09);
    doRegisterLineWidth(":ui/width03.png", "0.40mm", RS2::Width10);
    doRegisterLineWidth(":ui/width04.png", "0.50mm (ISO)", RS2::Width11);
    doRegisterLineWidth(":ui/width05.png", "0.53mm", RS2::Width12);
    doRegisterLineWidth(":ui/width05.png", "0.60mm", RS2::Width13);
    doRegisterLineWidth(":ui/width06.png", "0.70mm (ISO)", RS2::Width14);
    doRegisterLineWidth(":ui/width07.png", "0.80mm", RS2::Width15);
    doRegisterLineWidth(":ui/width08.png", "0.90mm", RS2::Width16);
    doRegisterLineWidth(":ui/width09.png", "1.00mm (ISO)", RS2::Width17);
    doRegisterLineWidth(":ui/width10.png", "1.06mm", RS2::Width18);
    doRegisterLineWidth(":ui/width10.png", "1.20mm", RS2::Width19);
    doRegisterLineWidth(":ui/width12.png", "1.40mm (ISO)", RS2::Width20);
    doRegisterLineWidth(":ui/width12.png", "1.58mm", RS2::Width21);
    doRegisterLineWidth(":ui/width12.png", "2.00mm (ISO)", RS2::Width22);
    doRegisterLineWidth(":ui/width12.png", "2.11mm", RS2::Width23);
}

/**
 * Registration of all line types
 */
void LC_PenInfoRegistry::registerLineTypes(){
    doRegisterLineType(":/icons/point_cross.svg", "- Unchanged -", RS2::LineTypeUnchanged);
//    registerLineType(":ui/linetype00.png", "- Unchanged -", RS2::LineTypeUnchanged);
    doRegisterLineType(":/icons/point_blank_square.svg", "By Layer", RS2::LineByLayer);
//    registerLineType(":ui/linetype00.png", "By Layer", RS2::LineByLayer);
    doRegisterLineType(":/icons/point_plus_square.svg", "By Block", RS2::LineByBlock);
//    registerLineType(":ui/linetype00.png", "By Block", RS2::LineByBlock);
    doRegisterLineType(":ui/linetype00.png", "No Pen", RS2::NoPen);
    doRegisterLineType(":ui/linetype01.png", "Continuous", RS2::SolidLine);
    doRegisterLineType(":ui/linetype02.png", "Dot", RS2::DotLine);
    doRegisterLineType(":ui/linetype02.png", "Dot (tiny)", RS2::DotLineTiny);
    doRegisterLineType(":ui/linetype02.png", "Dot (small)", RS2::DotLine2);
    doRegisterLineType(":ui/linetype02.png", "Dot (large)", RS2::DotLineX2);
    doRegisterLineType(":ui/linetype03.png", "Dash", RS2::DashLine);
    doRegisterLineType(":ui/linetype03.png", "Dash (tiny)", RS2::DashLineTiny);
    doRegisterLineType(":ui/linetype03.png", "Dash (small)", RS2::DashLine2);
    doRegisterLineType(":ui/linetype03.png", "Dash (large)", RS2::DashLineX2);
    doRegisterLineType(":ui/linetype04.png", "Dash Dot", RS2::DashDotLine);
    doRegisterLineType(":ui/linetype04.png", "Dash Dot (tiny)", RS2::DashDotLineTiny);
    doRegisterLineType(":ui/linetype04.png", "Dash Dot (small)", RS2::DashDotLine2);
    doRegisterLineType(":ui/linetype04.png", "Dash Dot (large)", RS2::DashDotLineX2);
    doRegisterLineType(":ui/linetype05.png", "Divide", RS2::DivideLine);
    doRegisterLineType(":ui/linetype05.png", "Divide (tiny)", RS2::DivideLineTiny);
    doRegisterLineType(":ui/linetype05.png", "Divide (small)", RS2::DivideLine2);
    doRegisterLineType(":ui/linetype05.png", "Divide (large)", RS2::DivideLineX2);
    doRegisterLineType(":ui/linetype06.png", "Center", RS2::CenterLine);
    doRegisterLineType(":ui/linetype06.png", "Center (tiny)", RS2::CenterLineTiny);
    doRegisterLineType(":ui/linetype06.png", "Center (small)", RS2::CenterLine2);
    doRegisterLineType(":ui/linetype06.png", "Center (large)", RS2::CenterLineX2);
    doRegisterLineType(":ui/linetype07.png", "Border", RS2::BorderLine);
    doRegisterLineType(":ui/linetype07.png", "Border (tiny)", RS2::BorderLineTiny);
    doRegisterLineType(":ui/linetype07.png", "Border (small)", RS2::BorderLine2);
    doRegisterLineType(":ui/linetype07.png", "Border (large)", RS2::BorderLineX2);
}

/**
 * Here we just iterate over QT colors names and store them in the map
 * Also, we register names for standard colors as they are used in LibreCAD
 */
void LC_PenInfoRegistry::registerColorNames(){
    // register QT colors
    const QStringList &colorNamesList = QColor::colorNames();
    int count = colorNamesList.count();
    for (int i=0; i < count; i++){
        QString colorName = colorNamesList.at(i);
        QColor color(colorName);
        // todo - basically, localization of QT color names is needed for this mapping
        colorNamesMap.insert(color.rgba64(), QObject::tr(colorName.toStdString().c_str()));
    }

    // register standard LibreCAD Colors
    registerLCColor(Qt::red,"Red");
    registerLCColor(Qt::darkRed, "Dark Red");
    registerLCColor(Qt::yellow, "Yellow");
    registerLCColor(Qt::darkYellow, "Dark Yellow");
    registerLCColor(Qt::green, "Green");
    registerLCColor(Qt::darkGreen, "Dark Green");
    registerLCColor(Qt::cyan, "Cyan");
    registerLCColor(Qt::darkCyan, "Dark Cyan");
    registerLCColor(Qt::blue, "Blue");
    registerLCColor(Qt::darkBlue, "Dark Blue");
    registerLCColor(Qt::magenta, "Magenta");
    registerLCColor(Qt::darkMagenta, "Dark Magenta");
    registerLCColor(Qt::black, "Black / White");
    registerLCColor(Qt::gray,"Gray");
    registerLCColor(Qt::darkGray,"Dark Gray");
    registerLCColor(Qt::lightGray,"Light Gray");

}

void LC_PenInfoRegistry::registerLCColor(QColor color, QString colorName){
    standardLCColorNamesMap.insert(color.rgba64(), QObject::tr(colorName.toStdString().c_str()));
}

/**
 * Internal method that performs registration of line type information and stores one in the map
 * @param iconName name of icon for linetype
 * @param labelKey display name of linetype
 * @param lineType line type itself
 */

void LC_PenInfoRegistry::doRegisterLineType(const char* iconName, const char* labelKey, RS2::LineType lineType){
    QIcon icon = QIcon(iconName);
    lineTypesIconMap.insert(lineType, icon);
    QString label = QObject::tr(labelKey);
    lineTypesNamesMap.insert(lineType, label);
}

/**
 * Internal method for registration of line width
 * @param iconName icon for line width
 * @param labelKey  label to display in UI
 * @param lineWidth line width
 */
void LC_PenInfoRegistry::doRegisterLineWidth(const char* iconName, const char* labelKey, RS2::LineWidth lineWidth){
    QIcon icon = QIcon(iconName);
    lineWidthIconMap.insert(lineWidth, icon);
    QString label = QObject::tr(labelKey);
    lineWidthNamesMap.insert(lineWidth, label);
}
