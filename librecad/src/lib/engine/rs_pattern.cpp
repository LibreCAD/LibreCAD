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


#include "rs_pattern.h"

#include "rs_system.h"
#include "rs_fileio.h"
#include "rs_layer.h"
#include "rs_debug.h"


/**
 * Constructor.
 *
 * @param fileName File name of a DXF file defining the pattern
 */
RS_Pattern::RS_Pattern(const QString& fileName)
        : RS_EntityContainer(nullptr)
		,fileName(fileName)
		,loaded(false)
{
	RS_DEBUG->print("RS_Pattern::RS_Pattern() ");
}


/**
 * Clone
 *
 * @author{Dongxu Li}
 */
RS_Entity* RS_Pattern::clone() const
{
    auto* cloned = new RS_Pattern(fileName);
    cloned->loaded = loaded;
    if (loaded) {
            for(auto* entity: *this)
            cloned->addEntity(isOwner() ? entity->clone() : entity);
        }
    return cloned;
}

/**
 * Loads the given pattern file into this pattern.
 * Entities other than lines are ignored.
 *
 * @param filename File name of the pattern file (without path and
 * extension or full path.
 */
bool RS_Pattern::loadPattern() {
    if (loaded) {
        return true;
    }

    RS_DEBUG->print("RS_Pattern::loadPattern");

    // Search for the appropriate pattern if we have only the name of the pattern:
    QString path;

    if (!fileName.endsWith(".dxf", Qt::CaseInsensitive)) {
        foreach (const QString& path0, RS_SYSTEM->getPatternList()) {
            if (QFileInfo(path0).baseName().toLower()==fileName.toLower()) {
                path = path0;
                RS_DEBUG->print("Pattern found: %s", path.toLatin1().data());
                break;
            }
        }
        if (path.isEmpty()) {
                RS_DEBUG->print("Pattern not found: %s", fileName.toLatin1().data());
        }
    }

    // We have the full path of the pattern:
    else {
        path = fileName;
    }

    // No pattern paths found:
    if (path.isEmpty()) {
        RS_DEBUG->print("No pattern \"%s\"available.", fileName.toLatin1().data());
        return false;
    }

	RS_Graphic gr;
	RS_FileIO::instance()->fileImport(gr, path);
    for(const auto* e: gr){
        if (e && (e->rtti()==RS2::EntityLine ||
				e->rtti()==RS2::EntityArc||
				e->rtti()==RS2::EntityEllipse
                  )) {
            RS_Layer* l = e->getLayer();
            RS_Entity* cl = e->clone();
            cl->reparent(this);
			if (l) {
                cl->setLayer(l->getName());
            }
            addEntity(cl);
        }
	}

    loaded = true;
    RS_DEBUG->print("RS_Pattern::loadPattern: OK");

    return true;
}

QString RS_Pattern::getFileName() const {
	return fileName;
}

