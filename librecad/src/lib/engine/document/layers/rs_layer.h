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

#ifndef RS_LAYER_H
#define RS_LAYER_H

#ifdef __hpux
#include <sys/_size_t.h>
#endif

#include <iosfwd>

#include "rs_pen.h"

class QString;

/**
 * Holds the data that defines a layer.
 */
struct RS_LayerData {
    RS_LayerData() = default;

    RS_LayerData(const QString& name, const RS_Pen& pen, bool frozen, bool locked);

    QString name; //!< Layer name
    RS_Pen pen; //!< default pen for this layer
    bool frozen{false}; //!< Frozen flag
    bool locked{false}; //!< Locked flag
    bool print{true}; //!< Print flag
    bool converted{false}; //!< Converted flag (CAM)
    bool construction{false};
    //!< a construction layer has entities of infinite length
                                    //!< and will never be printed out
    bool visibleInLayerList{true}; //!< visible in layer list
    bool selectedInLayerList{false}; //!< selected in layer list
};

/**
 * Class for representing a layer
 *
 * @author Andrew Mustun
 */
class RS_Layer {
public:

    static constexpr int NOT_DEFINED_LAYER_TYPE = -1;
    // Layer types
    enum LayerType{
        VIRTUAL,
        NORMAL,
        DIMENSIONAL,
        INFORMATIONAL,
        ALTERNATE_POSITION
    };

    explicit RS_Layer(const QString& name);
    //RS_Layer(const char* name);

    RS_Layer* clone() const;

    /** sets a new name for this layer. */
    void setName(const QString& name);

    /** @return the name of this layer. */
    QString getName() const;

    /** sets the default pen for this layer. */
    void setPen(const RS_Pen& pen);

    /** @return default pen for this layer. */
    RS_Pen getPen() const;

    /**
     * @retval true if this layer is frozen (invisible)
     * @retval false if this layer isn't frozen (visible)
     */
    bool isFrozen() const;

    /**
     * @retval true the layer has been converted already
     * @retval false the layer still needs to be converted
     */
    bool isConverted() const;

    /**
     * Sets the converted flag
     */
    void setConverted(bool c);

    /**
     * Toggles the visibility of this layer.
     * Freezes the layer if it's not frozen, thaws the layer otherwise
     */
    void toggle();

    /**
     * (De-)freezes this layer.
     *
     * @param freeze true: freeze, false: defreeze
     */
    void freeze(bool freeze);

    /**
     * Toggles the lock of this layer.
     */
    void toggleLock();

    /**
     * Toggles printing of this layer on / off.
     */
    void togglePrint();

    /**
     * Toggles construction attribute of this layer on / off.
     */
    void toggleConstruction();

    /**
     * Locks/Unlocks this layer.
     *
     * @param l true: lock, false: unlock
     */
    void lock(bool l);

    /**
     * return the LOCK state of the Layer
     */
    bool isLocked() const;

    /**
     * set visibility of layer in layer list
     *
     * @param l true: visible, false: invisible
     */
    void visibleInLayerList(bool l);

    /**
     * return the visibility of the Layer in layer list
     */
    bool isVisibleInLayerList() const;

    /**
     * set selection state of the layer in layer list
     *
     * @param val true: selected, false: deselected
     */
    void selectedInLayerList(bool val);

    /**
     * return selection state of the layer in layer list
     */
    bool isSelectedInLayerList() const;

    /**
     * set the PRINT state of the Layer
     *
     * @param print true: print layer, false: don't print layer
     */
    bool setPrint(bool print);

    /**
     * return the PRINT state of the Layer
     */
    bool isPrint() const;

    /**
     * whether the layer is a construction layer
     * The construction layer property is stored
     * in extended data in the DXF layer table
     */
    bool isConstruction() const;

    /**
     * set the construction attribute for the layer
     *
     * @param construction true: infinite lines, false: normal layer
     */
    bool setConstruction(bool construction);

    LayerType getLayerType()const {return m_layerType;}

    void setLayerType(LayerType layerType) {m_layerType = layerType;}

    friend std::ostream& operator <<(std::ostream& os, const RS_Layer& l);

private:
    //! Layer data
    RS_LayerData m_data;
    LayerType m_layerType = LayerType::NORMAL; // transient field, should be updated by LayerTreeWidget
};

#endif
