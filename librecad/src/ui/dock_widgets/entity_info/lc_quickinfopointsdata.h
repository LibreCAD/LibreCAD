/****************************************************************************
*
* Class that holds the list of collected coordinates

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
#ifndef LC_QUICKINFOPOINTSDATA_H
#define LC_QUICKINFOPOINTSDATA_H

#include <QVector>
#include "lc_quickinfobasedata.h"

class LC_QuickInfoPointsData : public LC_QuickInfoBaseData{
  Q_DECLARE_TR_FUNCTIONS(LC_QuickInfoPointsData)
public:
    LC_QuickInfoPointsData();
    ~LC_QuickInfoPointsData() override;

    /**
     * Holds information about collected coordinate
     */
    struct PointInfo{
        PointInfo(const RS_Vector &data, const QString &label, const QString &value):data(data), label(label), value(value){}
        // coordinate itself
        RS_Vector data;
        // label value
        QString label;
        //display value of coordinate
        QString value;
        // distance to zero (depends on coordinates mode)
        QString distance;
        // angle to zero
        QString angle;
    };

    void processCoordinate(const RS_Vector &wcsPoint);
    bool updateForCoordinateViewMode(int mode) override;
    void clear() override;
    QString generateView(bool showDistanceAndAngle, bool forceUpdate = false);
    bool removeCoordinate(int index);
    void setPointInsertionIndex(int index){m_collectedPointsInsertionIndex = index;}
    RS_Vector getVectorForIndex(int index) const override;
    RS_Vector getCollectedCoordinate(int index) const {return m_collectedPoints.at(index)->data;}
    int getCollectedCoordinatesCount() const {return m_collectedPoints.size();}
    bool hasData() const override;
private:
    // index used for insertion of collected coordinates
    int m_collectedPointsInsertionIndex = -1;
    // list of collected coordinates
    QVector<PointInfo*> m_collectedPoints;
    void doUpdatePointsAttributes() const;
    PointInfo *createPointInfo(const RS_Vector &point, const RS_Vector &viewCoordinate, const QString &idxValue, bool relative) const;
};

#endif // LC_QUICKINFOPOINTSDATA_H
