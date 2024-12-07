#pragma once

#ifndef RS_PYTHONGUI_H
#define RS_PYTHONGUI_H
#include "rs_document.h"
#include "rs_graphic.h"

#include <array>

class RS_PythonGui
{
public:
    RS_PythonGui();
    ~RS_PythonGui();

    void prompt(const char *prompt);
    void command(const char *cmd);
    void MessageBox(const char *msg);

    int GetIntDialog(const char *prompt);
    double GetDoubleDialog(const char *prompt);

    char ReadCharDialog();
    const char *OpenFileDialog(const char *title, const char *fileName, const char *fileExt);
    const char *GetStringDialog(const char *prompt);

    RS_Document* getDocument() const;
    RS_Graphic* getGraphic() const;

};

#endif // RS_PYTHONGUI_H
