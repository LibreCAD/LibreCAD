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

#include <iostream>
#include <QString>
#include "rs_layer.h"

RS_LayerData::RS_LayerData(const QString& name,
						   const RS_Pen& pen,
						   bool frozen,
						   bool locked):
	name(name)
  ,pen(pen)
  ,frozen(frozen)
  ,locked(locked)
{
}

/**
 * Constructor.
 */
RS_Layer::RS_Layer(const QString& name):
	data(name, RS_Pen(Qt::black, RS2::Width00,RS2::SolidLine), false, false)
{
}

RS_Layer* RS_Layer::clone() const{
	return new RS_Layer(*this);
}

/** sets a new name for this layer. */
void RS_Layer::setName(const QString& name) {
	data.name = name;
}

/** @return the name of this layer. */
QString RS_Layer::getName() const {
	return data.name;
}

/** sets the default pen for this layer. */
void RS_Layer::setPen(const RS_Pen& pen) {
	data.pen = pen;
}

/** @return default pen for this layer. */
RS_Pen RS_Layer::getPen() const {
	return data.pen;
}

/**
 * @retval true if this layer is frozen (invisible)
 * @retval false if this layer isn't frozen (visible)
 */
bool RS_Layer::isFrozen() const {
	return data.frozen;
	//getFlag(RS2::FlagFrozen);
}

/**
 * @retval true the layer has been converted already
 * @retval false the layer still needs to be converted
 */
bool RS_Layer::isConverted() const {
	return data.converted;
}

/**
 * Sets the converted flag
 */
void RS_Layer::setConverted(bool c) {
	data.converted = c;
}

/**
 * Toggles the visibility of this layer.
 * Freezes the layer if it's not frozen, thaws the layer otherwise
 */
void RS_Layer::toggle() {
	//toggleFlag(RS2::FlagFrozen);
	data.frozen = !data.frozen;
}

/**
 * (De-)freezes this layer.
 *
 * @param freeze true: freeze, false: defreeze
 */
void RS_Layer::freeze(bool freeze) {
	data.frozen = freeze;
}

/**
 * Toggles the lock of this layer.
 */
void RS_Layer::toggleLock() {
	//toggleFlag(RS2::FlagFrozen);
	data.locked = !data.locked;
}

/**
 * Toggles printing of this layer on / off.
 */
void RS_Layer::togglePrint() {
	data.print = !data.print;
}

/**
 * Toggles construction attribute of this layer on / off.
 */
void RS_Layer::toggleConstruction() {
	data.construction = !data.construction;
}

/**
 * Locks/Unlocks this layer.
 *
 * @param l true: lock, false: unlock
 */
void RS_Layer::lock(bool l) {
	data.locked = l;
}

/**
 * return the LOCK state of the Layer
 */
bool RS_Layer::isLocked() const{
	return data.locked;
}

/**
 * set visibility of layer in layer list
 *
 * @param l true: visible, false: invisible
 */
void RS_Layer::visibleInLayerList(bool l) {
	data.visibleInLayerList = l;
}

/**
 * return the visibility of the Layer in layer list
 */
bool RS_Layer::isVisibleInLayerList() const{
	return data.visibleInLayerList;
}

/**
 * set the PRINT state of the Layer
 *
 * @param print true: print layer, false: don't print layer
 */
bool RS_Layer::setPrint( const bool print) {
	return data.print = print;
}

/**
 * return the PRINT state of the Layer
 */
bool RS_Layer::isPrint() const{
	return data.print;
}

/**
 * whether the layer is a construction layer
 * The construction layer property is stored
 * in extended data in the DXF layer table
 */
bool RS_Layer::isConstruction() const{
	return data.construction;
}

/**
 * set the construction attribute for the layer
 *
 * @param construction true: infinite lines, false: normal layer
 */
bool RS_Layer::setConstruction( const bool construction){
	data.construction = construction;
	return construction;
}

/**
 * Dumps the layers data to stdout.
 */
std::ostream& operator << (std::ostream& os, const RS_Layer& l) {
    os << " name: " << l.getName().toLatin1().data()
    << " pen: " << l.getPen()
    << " frozen: " << (int)l.isFrozen()
    << " address: " << &l
    << std::endl;
    return os;
}

