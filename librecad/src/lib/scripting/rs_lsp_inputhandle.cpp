#include "rs_lsp_inputhandle.h"
#include <QEventLoop>

RS_Lsp_InputHandle::RS_Lsp_InputHandle(QG_Lsp_CommandEdit *parent)
    : m_edit(parent)
{
    QEventLoop loop;
    connect(m_edit, SIGNAL(returnPressed()), &loop, SLOT(quit()));
    loop.exec();
}
