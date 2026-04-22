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

#ifndef LC_PLOTSETTINGS_H
#define LC_PLOTSETTINGS_H
#include <QString>

#include "rs.h"
#include "rs_vector.h"

class RS_Graphic;
class QString;

struct LC_MarginsRect {
    double left{0.0};
    double top{0.0};
    double bottom{0.0};
    double right{0.0};

    bool operator ==(const LC_MarginsRect& v) const {
        return (left == v.left) && (top == v.top) && (bottom == v.bottom) && (right == v.right);
    }

    double getLeft() const {
        return left;
    }

    void setLeft(const double v) {
        left = v;
    }

    double getTop() const {
        return top;
    }

    void setTop(const double v) {
        top = v;
    }

    double getBottom() const {
        return bottom;
    }

    void setBottom(const double v) {
        bottom = v;
    }

    double getRight() const {
        return right;
    }

    void setRight(const double v) {
        right = v;
    }
};

class LC_PlotSettings {
public:
    explicit LC_PlotSettings(RS_Graphic* graphic)
        : m_graphic(graphic) {
    }
    QString getPaperSizeName() const;
    void setPaperSizeName(const QString& paperSizeName);
    double getPaperWidthMm() const;
    void setPaperWidthMm(double paperWidthMm);
    double getPaperHeightMm() const;
    void setPaperHeightMm(double paperHeightMm);
    double getOriginOffsetXMm() const;
    void setOriginOffsetXMm(double originOffsetXMm);
    double getOriginOffsetYMm() const;
    void setOriginOffsetYMm(double originOffsetYMm);
    double getPlotWindowLowerLeftX() const;
    void setPlotWindowLowerLeftX(double plotWindowLowerLeftX);
    double getPlotWindowLowerLeftY() const;
    void setPlotWindowLowerLeftY(double plotWindowLowerLeftY);
    double getPlotWindowUpperRightX() const;
    void setPlotWindowUpperRightX(double plotWindowUpperRightX);
    double getPlotWindowUpperRightY() const;
    void setPlotWindowUpperRightY(double plotWindowUpperRightY);
    double getCustomPrintScalePaperUnitsNumerator() const;
    void setCustomPrintScalePaperUnitsNumerator(double customPrintScalePaperUnitsNumerator);
    double getCustomPrintScaleDrawingUnitsDenominator() const;
    void setCustomPrintScaleDrawingUnitsDenominator(double customPrintScaleDrawingUnitsDenominator);
    int getPlotLayoutFlag() const;
    void setPlotLayoutFlag(int plotLayoutFlag);
    int getPlotPaperUnits() const;
    void setPlotPaperUnits(int plotPaperUnits);
    int getPlotRotation() const;
    void setPlotRotation(int plotRotation);
    int getPlotType() const;
    void setPlotType(int plotType);
    int getStandardScaleType() const;
    void setStandardScaleType(int standardScaleType);
    int getShadePlotMode() const;
    void setShadePlotMode(int shadePlotMode);
    int getShadePlotResolutionMode() const;
    void setShadePlotResolutionMode(int shadePlotResolutionMode);
    int getShadePlotCustomDpi() const;
    void setShadePlotCustomDpi(int shadePlotCustomDpi);
    double getStandardScaleFactor() const;
    void setStandardScaleFactor(double standardScaleFactor);
    double getPaperImageOriginX() const;
    void setPaperImageOriginX(double paperImageOriginX);
    double getPaperImageOriginY() const;
    void setPaperImageOriginY(double paperImageOriginY);
    QString getCurrentStyleName();
    void setCurrentStyleName(const QString& name);

    RS_Vector getPaperSize() const;
    void setPaperSize(const RS_Vector& s) const;
    RS_Vector getPrintAreaSize(bool total = true) const;

    //if set to true, will refuse to modify paper scale
    void setPaperScaleFixed(const bool fixed) {
        m_paperScaleFixed = fixed;
    }

    bool isPaperScaleFixed() const {
        return m_paperScaleFixed;
    }

    RS2::PaperFormat getPaperFormat(bool* landscape) const;
    void setPaperFormat(RS2::PaperFormat format, bool landscape);

    double getPaperScale() const;
    void setPaperScale(double s) const;

    /**
     * Paper margins in millimeters
     */
    void setMarginsInMm(double left, double top, double right, double bottom);

    void setMarginsInMm(const LC_MarginsRect& margins) {
        setMarginsInMm(margins.left, margins.top, margins.right, margins.bottom);
    }

    double getMarginLeftMm() const {
        return m_marginLeft_mm;
    }

    double getMarginTopMm() const {
        return m_marginTop_mm;
    }

    double getMarginRightMm() const {
        return m_marginRight_mm;
    }

    double getMarginBottomMm() const {
        return m_marginBottom_mm;
    }

    /**
     * Paper margins in graphic units
     */
    void setMarginsInUnits(double left, double top, double right, double bottom);
    LC_MarginsRect getMarginsInUnits() const;
    double getMarginLeftInUnits() const;
    double getMarginTopInUnits() const;
    double getMarginRightInUnits() const;
    double getMarginBottomInUnits() const;
    /**
     * Number of pages drawing occupies
     */
    void setPagesNum(int horiz, int vert);
    void setPagesNum(const QString& horizXvert);

    int getPagesNumHoriz() const {
        return m_pagesNumH;
    }

    int getPagesNumVert() const {
        return m_pagesNumV;
    }

    bool isBiggerThanPaper(const RS_Vector& size) const;

private:

    RS2::Unit getUnit() const;

    // Number of pages drawing occupies
    int m_pagesNumH = 1;
    int m_pagesNumV = 1;

    //if set to true, will refuse to modify paper scale
    bool m_paperScaleFixed = false;

    QString plotViewName; /*!< Plot view name, code 6 */

    enum PlotPaperUnits {
        Inches      = 0,
        Millimeters = 1,
        Pixels      = 2
    };

    QString m_plotViewName;
    QString m_paperSizeName;
    QString m_currentStyleName;
    // Paper margins in millimeters
    double m_marginLeft_mm = 0.;
    double m_marginTop_mm = 0.;
    double m_marginRight_mm = 0.;
    double m_marginBottom_mm = 0.;

    double m_paperWidth_mm = 0.0;
    double m_paperHeight_mm = 0.0;
    double originOffsetX_mm = 0.0;
    double originOffsetY_mm = 0.0;
    double m_plotWindowLowerLeftX  = 0.0;
    double m_plotWindowLowerLeftY = 0.0;
    double m_plotWindowUpperRightX = 0.0;
    double m_plotWindowUpperRightY = 0.0;
    double m_customPrintScalePaperUnitsNumerator = 0.0;
    double m_customPrintScaleDrawingUnitsDenominator = 0.0;
    int m_plotLayoutFlag = 0;
    int m_plotPaperUnits  = 0;
    int m_plotRotation = 0;
    int m_plotType = 0;
    int m_standardScaleType  = 0;
    int m_shadePlotMode = 0;
    int m_shadePlotResolutionMode  = 0;
    int m_shadePlotCustomDPI  = 0;
    double m_standardScaleFactor  = 0.0;
    double m_paperImageOriginX = 0.0;
    double m_paperImageOriginY = 0.0;

    // todo - probably should be replaced by layout later..
    RS_Graphic* m_graphic;
};

#endif
