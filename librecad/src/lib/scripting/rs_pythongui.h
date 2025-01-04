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
    const char *OpenFileDialog(const char *title, const char *fileName, const char *fileExt);
    const char *GetStringDialog(const char *prompt);
    const char *GetString(const char *msg = "Enter a text: ");
    const char *getKword(const char *msg);

    RS_Vector getPoint(const char *msg = "Enter a point: ", const RS_Vector basePoint=RS_Vector()) const;
    RS_Vector getCorner(const char *msg = "Enter second corner: ", const RS_Vector &basePoint=RS_Vector(0, 0)) const;

    RS_Document *getDocument() const;
    RS_Graphic *getGraphic() const;
    RS_EntityContainer* getContainer() const;
};

#endif // RS_PYTHONGUI_H
