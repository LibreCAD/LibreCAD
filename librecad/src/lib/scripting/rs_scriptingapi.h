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
#ifndef RS_SCRIPTINGAPI_H
#define RS_SCRIPTINGAPI_H
#include "Python.h"
#include <QString>

#define RS_SCRIPTINGAPI RS_ScriptingApi::instance()

class RS_ScriptingApi
{
public:
    static RS_ScriptingApi* instance();
    ~RS_ScriptingApi();

    void command(QString &com);
    void endList();
    void endImage();
    void termDialog();
    void unloadDialog(int id);

    unsigned int entlast();
    unsigned int entnext(unsigned int current=0);

    int loadDialog(const char *filename);
    int startDialog();

    bool actionTile(const char *id, const char *action);
    bool addList(const char *val, std::string &result);
    bool dimxTile(const char *key, int &x);
    bool dimyTile(const char *key, int &y);
    bool doneDialog(int res, int &x, int &y);
    bool fillImage(int x, int y, int width, int height, int color);
    bool getAttr(const char *key, const char *attr, std::string &result);
    bool modeTile(const char *key, int mode);
    bool newDialog(const char *name, int id);
    bool pixImage(int x1, int y1, int x2, int y2, const char *path);
    bool setTile(const char *key, const char *val);
    bool textImage(int x1, int y1, int x2, int y2, const char *text, int color);
    bool vectorImage(int x1, int y1, int x2, int y2, int color);

    const std::string startImage(const char *key);
    const std::string startList(const char *key, int operation, int index);
    const std::string getTile(const char *key);

private:
    RS_ScriptingApi();
    static RS_ScriptingApi* unique;
};

#endif // RS_PYTHONCORE_H


