/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

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
 ******************************************************************************/

#ifndef LC_CURSOROVERLAYINFO_H
#define LC_CURSOROVERLAYINFO_H

#include <QString>

#include "rs_vector.h"
#include "lc_overlayentity.h"

struct  LC_InfoCursorOptions{
    struct ZoneSetup;
    LC_InfoCursorOptions();
    ~LC_InfoCursorOptions();
    void setFontSize(int size);
    const ZoneSetup& zone(int index) const;
    ZoneSetup& zone(int index);
    int offset = 10;
    int fontSize = 10;
    QString fontName = "Verdana";

    struct Impl;
    const std::unique_ptr<Impl> m_pImpl;
};

class LC_InfoMessageBuilder{

public:
    LC_InfoMessageBuilder() {}

    explicit LC_InfoMessageBuilder(const QString& m) : msg(m) {
        msg.append("\n");
    }

    QString toString() {
        return msg;
    }

    void add(QString label, QString value = "") {
        if (msg.contains(label)) {
            value = "0";
        }
        msg.append(label);
        if (!value.isEmpty()) {
            msg.append(" ");
            msg.append(value);
        }
        msg.append("\n");
    }

    void cleanup() {
        msg = "";
    }
  protected:
    QString msg;
};


struct LC_InfoCursorData{
public:
    void clear(){
        zone1.clear();
        zone2.clear();
        zone3.clear();
        zone4.clear();
    }

    void setZone1(const QString &zone1) {
        LC_InfoCursorData::zone1 = zone1;
    }

    void setZone2(const QString &zone2) {
        LC_InfoCursorData::zone2 = zone2;
    }

    void setZone3(const QString &zone3) {
        LC_InfoCursorData::zone3 = zone3;
    }

    void setZone4(const QString &zone4) {
        LC_InfoCursorData::zone4 = zone4;
    }

    const QString &getZone1() const {
        return zone1;
    }

    const QString &getZone2() const {
        return zone2;
    }

    const QString &getZone3() const {
        return zone3;
    }

    const QString &getZone4() const {
        return zone4;
    }

protected:
    // bottom left
    QString zone1;
    // bottom right
    QString zone2;
    // top left
    QString zone3;
    // top right
    QString zone4;

};

struct LC_InfoCursorOverlayPrefs{
    bool enabled = true;
    bool showAbsolutePosition = false;
    bool showAbsolutePositionWCS = false;
    bool showRelativePositionDistAngle = false;
    bool showRelativePositionDeltas = true;
    bool showCommandPrompt = false;
    bool showSnapType = false;
    bool showEntityInfoOnCatch = true;
    bool showEntityInfoOnCreation = true;
    bool showEntityInfoOnModification = true;
    bool showLabels = false;
    bool multiLine = false;
    bool showCurrentActionName = true;
    LC_InfoCursorOptions options = LC_InfoCursorOptions();

    void loadSettings();
};

class LC_OverlayInfoCursor:public LC_OverlayDrawable{
public:
    LC_OverlayInfoCursor(const RS_Vector &coord, LC_InfoCursorOptions* cursorOverlaySettings);
    void setZonesData(LC_InfoCursorData *data);
    void draw(RS_Painter *painter) override;
    void clear();
    LC_InfoCursorData* getData(){return zonesData;}
    LC_InfoCursorData *getZonesData() const;
    LC_InfoCursorOptions *getOptions() const;
    void setOptions(LC_InfoCursorOptions *options);
    void setPos(const RS_Vector wPos){wcsPos = wPos;};
protected:
    LC_InfoCursorData* zonesData = nullptr;
    LC_InfoCursorOptions* options = nullptr;
    RS_Vector wcsPos;
};

#endif // LC_CURSOROVERLAYINFO_H
