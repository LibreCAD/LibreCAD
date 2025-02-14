#include "rs_scripting_inputhandle.h"
#include <QEventLoop>

RS_Scripting_InputHandle::RS_Scripting_InputHandle(const QString &prombt, QLineEdit *parent)
    : m_edit(parent)
{
    m_size = prombt.size();
    QEventLoop loop;
    connect(m_edit, SIGNAL(returnPressed()), &loop, SLOT(quit()));
    loop.exec();
}

