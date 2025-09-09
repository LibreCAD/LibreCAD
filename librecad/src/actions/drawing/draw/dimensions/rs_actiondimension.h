/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
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

#ifndef RS_ACTIONDIMENSION_H
#define RS_ACTIONDIMENSION_H

#include "rs.h"
#include "rs_previewactioninterface.h"

struct RS_DimensionData;

/**
 * Base class for dimension actions.
 *
 * @author Andrew Mustun
 */
class RS_ActionDimension:public RS_PreviewActionInterface {
    Q_OBJECT
public:
    RS_ActionDimension(const char *name, LC_ActionContext *actionContext, RS2::EntityType dimType, RS2::ActionType actionType = RS2::ActionNone);
    ~RS_ActionDimension() override;
    void init(int status) override;

    QString getText() const;
    void setText(const QString &t);
    const QString &getLabel() const;
    void setLabel(const QString &t);
    const QString &getTol1() const;
    void setTol1(const QString &t);
    const QString &getTol2() const;
    void setTol2(const QString &t);
    bool getDiameter() const;
    void setDiameter(bool d);
    static bool isDimensionAction(RS2::ActionType type);
    void resume() override; // fixme - sand - check?
    void setDimStyleName(const QString& styleName);
    QString getDimStyleName();
protected:
    /**
     * Generic dimension data.
     */
    std::unique_ptr<RS_DimensionData> m_dimensionData;
    RS2::EntityType m_dimTypeToCreate;
    QString m_label;
    QString m_tol1;
    QString m_tol2;
    bool m_diameter = false;
    LC_ActionOptionsWidget* createOptionsWidget() override;
    RS2::CursorType doGetMouseCursor(int status) override;
    bool m_previewShowsFullDimension = false;
    void readSettings();
    virtual void reset();
};

#endif
