#pragma once

#ifndef RS_PYTHONGUI_H
#define RS_PYTHONGUI_H
#include <array>

class RS_PythonGui
{
public:
    RS_PythonGui();
    ~RS_PythonGui();

    void Hello();
    void prompt(const char *prompt);
    void MessageBox(const char *msg);
    void unloadDialog(int id);

    bool newDialog(const char *name, int id);
    std::array<int, 2> doneDialog(int res=-1);
    bool setTile(const char *key, const char *val);
    bool modeTile(const char *key, int val);
    bool actionTile(const char *id, const char *action);

    int GetIntDialog(const char *prompt);
    int loadDialog(const char *fileName);
    int startDialog();

    double GetDoubleDialog(const char *prompt);

    char ReadCharDialog();
    const char *OpenFileDialog(const char *title, const char *fileName, const char *fileExt);
    const char *GetStringDialog(const char *prompt);
    const char *startList(const char *key, int operation, int index);
    const char *addList(const char *val);
};

#endif // RS_PYTHONGUI_H
