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

#include "lc_overlayentity.h"
#include "rs_vector.h"

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
    const std::unique_ptr<Impl> pImpl;
};

class LC_InfoMessageBuilder{

public:
    LC_InfoMessageBuilder() = default;

    explicit LC_InfoMessageBuilder(const QString& m) : m_msg(m) {
        m_msg.append("\n");
    }

    QString toString() {
        return m_msg;
    }

    void add(const QString& label, QString value = "") {
        if (m_msg.contains(label)) {
            value = "0";
        }
        m_msg.append(label);
        if (!value.isEmpty()) {
            m_msg.append(" ");
            m_msg.append(value);
        }
        m_msg.append("\n");
    }

    void cleanup() {
        m_msg.clear();
    }
  protected:
    QString m_msg;
};


struct LC_InfoCursorData{
    void clear(){
        m_zone1.clear();
        m_zone2.clear();
        m_zone3.clear();
        m_zone4.clear();
    }

    void setZone1(const QString &zone1) {
        m_zone1 = zone1;
    }

    void setZone2(const QString &zone2) {
        m_zone2 = zone2;
    }

    void setZone3(const QString &zone3) {
        m_zone3 = zone3;
    }

    void setZone4(const QString &zone4) {
        m_zone4 = zone4;
    }

    const QString &getZone1() const {
        return m_zone1;
    }

    const QString &getZone2() const {
        return m_zone2;
    }

    const QString &getZone3() const {
        return m_zone3;
    }

    const QString &getZone4() const {
        return m_zone4;
    }

protected:
    // bottom left
    QString m_zone1;
    // bottom right
    QString m_zone2;
    // top left
    QString m_zone3;
    // top right
    QString m_zone4;

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
    LC_InfoCursorOptions options;

    void loadSettings();
};

class LC_OverlayInfoCursor:public LC_OverlayDrawable{
public:
    LC_OverlayInfoCursor(const RS_Vector &coord, LC_InfoCursorOptions* cursorOverlaySettings);
    void setZonesData(LC_InfoCursorData *data);
    void draw(RS_Painter *painter) override;
    void clear() const;
    LC_InfoCursorData* getData() const {return m_zonesData;}
    LC_InfoCursorData *getZonesData() const;
    LC_InfoCursorOptions *getOptions() const;
    void setOptions(LC_InfoCursorOptions *options);
    void setPos(const RS_Vector& wPos){m_wcsPos = wPos;}
protected:
    LC_InfoCursorData* m_zonesData = nullptr;
    LC_InfoCursorOptions* m_options = nullptr;
    RS_Vector m_wcsPos;
};

#endif
