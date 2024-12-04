#include "rs_python.h"
#include "rs_pythongui.h"
#include "rs_dialogs.h"
#include "rs_py_inputhandle.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

RS_PythonGui::RS_PythonGui()
{
}

RS_PythonGui::~RS_PythonGui()
{
}

void RS_PythonGui::MessageBox(const char *msg)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("LibreCAD");
    msgBox.setText(QObject::tr(msg));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

const char *RS_PythonGui::OpenFileDialog(const char *title, const char *fileName, const char *fileExt)
{
    return qUtf8Printable(QFileDialog::getOpenFileName(nullptr, QObject::tr(title), fileName, QObject::tr(fileExt)));
}

int RS_PythonGui::GetIntDialog(const char *prompt)
{
    return QInputDialog::getInt(nullptr,
                        "LibreCAD",
                        QObject::tr(prompt),
                         // , int value = 0, int min = -2147483647, int max = 2147483647, int step = 1, bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
                         0, -2147483647, 2147483647, 1, nullptr, Qt::WindowFlags());
}

double RS_PythonGui::GetDoubleDialog(const char *prompt)
{
    return QInputDialog::getDouble(nullptr,
                                   "LibreCAD",
                                   QObject::tr(prompt),
                                   // double value = 0, double min = -2147483647, double max = 2147483647, int decimals = 1, bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags(), double step = 1)
                                   0, -2147483647, 2147483647, 1, nullptr, Qt::WindowFlags(), 1);
}

const char *RS_PythonGui::GetStringDialog(const char *prompt)
{
    return qUtf8Printable(QInputDialog::getText(nullptr,
                                                "LibreCAD",
                                                QObject::tr(prompt),
                                                //QLineEdit::EchoMode mode = QLineEdit::Normal, const QString &text = QString(), bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags(), Qt::InputMethodHints inputMethodHints = Qt::ImhNone)
                                                QLineEdit::Normal, "", nullptr, Qt::WindowFlags(), Qt::ImhNone));
}

char RS_PythonGui::ReadCharDialog()
{
    return RS_InputDialog::readChar();
}

void RS_PythonGui::prompt(const char *prompt)
{
    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(prompt);
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcess(false);

        RS_Py_InputHandle::readLine(Py_CommandEdit);
        Py_CommandEdit->setPrompt(">>> ");
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("LibreCAD");
        msgBox.setText(prompt);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
    }
}
