#ifndef RS_LSP_INPUTHANDLE_H
#define RS_LSP_INPUTHANDLE_H

#include "qg_lsp_commandedit.h"

class RS_Lsp_InputHandle : public QG_Lsp_CommandEdit
{
    Q_OBJECT
public:
    explicit RS_Lsp_InputHandle(QG_Lsp_CommandEdit *parent = nullptr);
    ~RS_Lsp_InputHandle() {}

    static QString readLine(QG_Lsp_CommandEdit *parent = nullptr)
    {
        RS_Lsp_InputHandle hdl(parent);
        return hdl.getString();
    }

    QString getString() const { return m_edit->text(); }

private:
    QG_Lsp_CommandEdit *m_edit;
};

#endif // RS_INPUTHANDLE_H
