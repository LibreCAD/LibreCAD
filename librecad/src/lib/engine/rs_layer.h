/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
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

    RS_LayerData(const QString& name,
                 const RS_Pen& pen,
                 bool frozen,
                 bool locked);

    QString name;                   //!< Layer name
    RS_Pen pen;                     //!< default pen for this layer
    bool frozen {false};            //!< Frozen flag
    bool locked {false};            //!< Locked flag
    bool print {true};              //!< Print flag
    bool converted {false};         //!< Converted flag (CAM)
    bool construction {false};      //!< a construction layer has entities of infinite length
                                    //!< and will never be printed out
    bool visibleInLayerList {true}; //!< visible in layer list
};



/**
 * Class for representing a layer
 *
 * @author Andrew Mustun
 */
class RS_Layer {
public:
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
     * set the PRINT state of the Layer
     *
     * @param print true: print layer, false: don't print layer
     */
	bool setPrint( const bool print);

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
	bool setConstruction( const bool construction);

    friend std::ostream& operator << (std::ostream& os, const RS_Layer& l);

private:
    //! Layer data
    RS_LayerData data;

};

#endif
