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
    bool modeTile(const char *key, int val);
    bool actionTile(const char *id, const char *action);

    int loadDialog(const char *fileName);
    int dimxTile(const char *key);
    int dimyTile(const char *key);
    int startDialog();
    int fillImage(int x1, int y1, int width, int height, int color);
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
