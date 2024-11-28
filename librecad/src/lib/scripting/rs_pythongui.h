#pragma once

#ifndef RS_PYTHONGUI_H
#define RS_PYTHONGUI_H

class RS_PythonGui
{
public:
    RS_PythonGui();
    ~RS_PythonGui();

    void MessageBox(const char *msg);
    const char *OpenFileDialog(const char *title, const char *path, const char *fileExt);
    const char *GetStringDialog(const char *prombt);
    char ReadCharDialog();
    int GetIntDialog(const char *prombt);
    double GetDoubleDialog(const char *prombt);
    void Hello();
};

#endif // RS_PYTHONGUI_H
