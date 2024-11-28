#include "rs_py_inputhandle.h"
#include <QEventLoop>

RS_Py_InputHandle::RS_Py_InputHandle(QG_Py_CommandEdit *parent)
    : m_edit(parent)
{
    QEventLoop loop;
    connect(m_edit, SIGNAL(returnPressed()), &loop, SLOT(quit()));
    loop.exec();
}
