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

#ifndef RS_PYTHONGUI_H
#define RS_PYTHONGUI_H
#include "rs_document.h"
#include "rs_graphic.h"
#include "rs_entitycontainer.h"
#include "rs_vector.h"

#include <array>

class RS_PythonGui
{
public:
    RS_PythonGui();
    ~RS_PythonGui();

    void prompt(const char *prompt);
    void command(const char *cmd);
    void initGet(const char *str="", int bit=0);
    void MessageBox(const char *msg);

    int GetIntDialog(const char *prompt);
    int getInt(const char *msg =  "Enter an integer: ") const;

    double GetDoubleDialog(const char *prompt);
    double getReal(const char *msg = "Enter a floating point number: ") const;
    double getDist(const char *msg = "Enter second point: ", const RS_Vector &basePoint=RS_Vector(0, 0)) const;
    double getOrient(const char *msg = "Enter a point: ", const RS_Vector &basePoint=RS_Vector(0, 0)) const;

    char ReadCharDialog();
    const std::string OpenFileDialog(const char *title, const char *fileName, const char *fileExt);
    const std::string GetStringDialog(const char *prompt);
    const std::string getString(const char *msg = "Enter a text: ");
    const std::string getKword(const char *msg);

    RS_Vector getPoint(const char *msg = "Enter a point: ", const RS_Vector basePoint=RS_Vector()) const;
    RS_Vector getCorner(const char *msg = "Enter second corner: ", const RS_Vector &basePoint=RS_Vector(0, 0)) const;

    RS_Document *getDocument() const;
    RS_Graphic *getGraphic() const;
    RS_EntityContainer* getContainer() const;
};

#endif // RS_PYTHONGUI_H
