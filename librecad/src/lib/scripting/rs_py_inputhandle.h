#ifndef RS_PY_INPUTHANDLE_H
#define RS_PY_INPUTHANDLE_H

#include "qg_py_commandedit.h"

class RS_Py_InputHandle : public QG_Py_CommandEdit
{
    Q_OBJECT
public:
    explicit RS_Py_InputHandle(QG_Py_CommandEdit *parent = nullptr);
    ~RS_Py_InputHandle() {}

    static QString readLine(QG_Py_CommandEdit *parent = nullptr)
    {
        RS_Py_InputHandle hdl(parent);
        return hdl.getString();
    }

    QString getString() const { return m_edit->text(); }

private:
    QG_Py_CommandEdit *m_edit;
};

#endif // RS_INPUTHANDLE_H
