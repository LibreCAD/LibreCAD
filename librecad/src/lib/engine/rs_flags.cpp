/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 Dongxu Li (dongxuli2011@gmail.com)
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

#include "rs_flags.h"

/** Constructor with initialisation to the given flags. */
RS_Flags::RS_Flags(unsigned f):
	flags(f)
{
}


unsigned RS_Flags::getFlags() const {
	return flags;
}

void RS_Flags::resetFlags() {
	flags=0;
}

void RS_Flags::setFlags(unsigned f) {
	flags=f;
}

void RS_Flags::setFlag(unsigned f) {
	flags |= f;
}

void RS_Flags::delFlag(unsigned f) {
	flags &= ~f;
}

void RS_Flags::toggleFlag(unsigned f) {
	flags ^= f;
}

bool RS_Flags::getFlag(unsigned f) const {
	return flags&f;
}

