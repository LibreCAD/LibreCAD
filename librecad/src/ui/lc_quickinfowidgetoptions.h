/****************************************************************************
*
* Options for QuickInfo widget related functions

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
#ifndef LC_QUICKINFOWIDGETOPTIONS_H
#define LC_QUICKINFOWIDGETOPTIONS_H

#include "rs_pen.h"

class LC_QuickInfoOptions{

public:
    /**
     * pen used for highlighting points in drawing
     */
    RS_Pen pen;
    /**
     * Defines whether points path (line between collected points) should be shown on preview of points collection action
     */
    bool displayPointsPath = true;
    /**
     * Controls whether distance and angle values should be shown in collected coordinates list
     */
    bool displayDistanceAndAngle = true;
    /**
     * Controls whether in default action entities under cursor may be selected on mouse move with pressed CTRL/META key
     */
    bool selectEntitiesInDefaultActionByCTRL = true;
    /**
     * if set, within default action entities under cursor will be selected for info widget by mouse move automatically
     */
    bool autoSelectEntitiesInDefaultAction = true;
    /**
     * defined whether details for segments of polyline should be shown (or just vertexes, otherwise)
     */
    bool displayPolylineDetailed = true;
    /**
     * defines whether min/max for entities should be also included into entities properties.
     */
    bool displayEntityBoundaries = false;

    void load();
    void save() const;
};

#endif // LC_QUICKINFOWIDGETOPTIONS_H
