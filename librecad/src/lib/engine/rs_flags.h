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
#ifndef RS_FLAGS_H
#define RS_FLAGS_H

/**
 * Base class for objects which have flags.
 *
 * @author Andrew Mustun
 */
struct RS_Flags {
	//! \{Constructor with initialisation to the given flags.
	//! Default sets all flags to 0
	RS_Flags(unsigned f = 0);
	//! \}

	virtual ~RS_Flags() = default;

	unsigned getFlags() const;

	void resetFlags();

	void setFlags(unsigned f);

	void setFlag(unsigned f);

	void delFlag(unsigned f);

	void toggleFlag(unsigned f);

	bool getFlag(unsigned f) const;

private:
	unsigned flags = 0;
};

#endif
