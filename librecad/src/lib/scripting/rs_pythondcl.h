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

#pragma once

#ifndef RS_PYTHONDCL_H
#define RS_PYTHONDCL_H
#include <array>

class RS_PythonDcl
{
public:
    RS_PythonDcl();
    ~RS_PythonDcl();

    void unloadDialog(int id);
    void endList();
    void endImage();
    void termDialog();

    bool newDialog(const char *name, int id);
    std::array<int, 2> doneDialog(int res=-1);
    bool setTile(const char *key, const char *val);
    bool modeTile(const char *key, int mode);
    bool actionTile(const char *id, const char *action);

    int loadDialog(const char *filename);
    int dimxTile(const char *key);
    int dimyTile(const char *key);
    int startDialog();
    int fillImage(int x, int y, int width, int height, int color);
    int vectorImage(int x1, int y1, int x2, int y2, int color);

    const char *pixImage(int x1, int y1, int x2, int y2, const char *path);
    const char *textImage(int x1, int y1, int x2, int y2, const char *text, int color);

    const char *getTile(const char *key);
    const char *getAttr(const char *key, const char *attr);
    const char *startList(const char *key, int operation=-1, int index=-1);
    const char *addList(const char *val);
    const char *startImage(const char *key);
};

#endif // RS_PYTHONDCL_H
