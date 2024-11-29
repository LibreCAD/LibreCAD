#include "rs_python.h"
#include "rs_pythongui.h"
#include "rs_dialogs.h"
#include "rs_py_inputhandle.h"
#include "LCL.h"
#include "Types.h"
#include "Environment.h"

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

int RS_PythonGui::loadDialog(const char *fileName)
{
    std::string path = fileName;
    const std::filesystem::path p(path.c_str());
    if (!p.has_extension()) {
        path += ".dcl";
    }
    if (!std::filesystem::exists(path.c_str())) {
        return -1;
    }

    lclValuePtr dcl = loadDcl(path);
    const lclGui *container = VALUE_CAST(lclGui, dcl);

    if (dcl) {
        int uniq = container->value().dialog_Id;
        dclEnv->set(STRF("#builtin-gui(%d)", uniq), dcl);
        dclEnv->set("load_dialog_id", lcl::integer(uniq));
        return uniq;
    }
    return -1;
}

bool RS_PythonGui::newDialog(const char *name, int id)
{
    const lclGui*     gui     = DYNAMIC_CAST(lclGui, dclEnv->get(STRF("#builtin-gui(%d)", id)));
    lclValueVec*      items   = new lclValueVec(gui->value().tiles->size());
    std::copy(gui->value().tiles->begin(), gui->value().tiles->end(), items->begin());

    for (auto it = items->begin(), end = items->end(); it != end; it++) {
        const lclGui* dlg = DYNAMIC_CAST(lclGui, *it);
        qDebug() << "Dialog: " << dlg->value().name.c_str();
        if (dlg->value().name == name) {
            openTile(dlg);
            return true;
        }
    }
    return false;
}

int RS_PythonGui::startDialog()
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    if(dialogId)
    {
        for (auto & tile : dclTiles)
        {
            if (tile->value().dialog_Id == dialogId->value())
            {
                const lclWidget* dlg = static_cast<const lclWidget*>(tile);
                dlg->widget()->show();
                dlg->widget()->setFixedSize(dlg->widget()->geometry().width(),
                                            dlg->widget()->geometry().height());
                break;
            }
        }
    }

    /*
     * The start_dialog function returns the optional status passed to done_dialog.
     * The default value is 1 if the user presses OK, 0 if the user presses Cancel, or -1 if all dialog boxes are terminated with term_dialog.
     * If done_dialog is passed an integer status greater than 1, start_dialog returns this value, whose meaning is determined by the application.
     */

    return 0;
}

void RS_PythonGui::unloadDialog(int id)
{
    if(id)
    {
        for (auto & tile : dclTiles)
        {
            if (tile->value().dialog_Id == id)
            {
                const lclWidget* dlg = static_cast<const lclWidget*>(tile);
                delete dlg->widget();

                break;
            }
        }
    }

    for (int i = 0; i < (int) dclTiles.size(); i++)
    {
        if(dclTiles.at(i)->value().dialog_Id == id)
        {
            dclTiles.erase(dclTiles.begin()+i);
        }
    }

    dclEnv->set(STRF("#builtin-gui(%d)", id), lcl::nilValue());
    dclEnv->set("load_dialog_id", lcl::nilValue());
}

bool RS_PythonGui::doneDialog(int id)
{
    Q_UNUSED(id);
    /* int args = 0-1 */
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    if(dialogId)
    {
        for (auto & tile : dclTiles)
        {
            if (tile->value().dialog_Id == dialogId->value())
            {
                const lclWidget* dlg = static_cast<const lclWidget*>(tile);
                dlg->widget()->show();

                lclValueVec* items = new lclValueVec(2);
                items->at(0) = lcl::integer(dlg->widget()->x());
                items->at(1) = lcl::integer(dlg->widget()->y());
                dlg->widget()->close();
                // FIXME type list
                //return lcl::list(items);
                return true;
            }
        }
    }
    return false;
}

bool RS_PythonGui::setTile(const char *key, const char *val)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));
    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key)
        {
            switch (tile->value().id)
            {
            case EDIT_BOX:
            {
                const lclEdit* edit = static_cast<const lclEdit*>(tile);
                edit->edit()->setText(val);
            }
            break;
            case TEXT:
            {
                const lclLabel* l = static_cast<const lclLabel*>(tile);
                l->label()->setText(val);
            }
            case BUTTON:
            {
                const lclButton* b = static_cast<const lclButton*>(tile);
                b->button()->setText(val);
            }
            break;
            case RADIO_BUTTON:
            {
                const lclRadioButton* rb = static_cast<const lclRadioButton*>(tile);
                rb->button()->setText(val);
            }
            break;
            default:
                return false;
            }
            return true;
        }
    }
    return false;
}

bool RS_PythonGui::actionTile(const char *id, const char *action)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    lclValuePtr value = dclEnv->get(std::to_string(dialogId->value()) + "_" + id);
    qDebug() << "value->print(true)" << value->print(true).c_str();
    if (value->print(true).compare("nil") == 0) {
        dclEnv->set(std::to_string(dialogId->value()) + "_" + id, lcl::string(action));
        return true;
    }
    return false;
}

bool RS_PythonGui::modeTile(const char *key, int val)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }

        if (noQuotes(tile->value().key) == key)
        {
            switch (tile->value().id)
            {
            case EDIT_BOX:
            {
                qDebug() << "mode_tile EDIT_BOX";
                const lclEdit* e = static_cast<const lclEdit*>(tile);
                switch (val)
                {
                case 0:
                {
                    e->edit()->setEnabled(true);
                }
                break;
                case 1:
                {
                    e->edit()->setEnabled(false);
                }
                break;
                case 2:
                {
                    e->edit()->setFocus();
                }
                break;
                case 3:
                {
                    e->edit()->selectAll();
                }
                break;
                default:
                    return false;
                }
            }
            break;
            case LIST_BOX:
            {
                qDebug() << "mode_tile LIST_BOX";
                const lclListBox* e = static_cast<const lclListBox*>(tile);
                switch (val)
                {
                case 0:
                {
                    e->list()->setEnabled(true);
                }
                break;
                case 1:
                {
                    e->list()->setEnabled(false);
                }
                break;
                case 2:
                {
                    e->list()->setFocus();
                }
                break;
                case 3:
                {
                    e->list()->selectAll();
                }
                break;
                default:
                    return false;
                }
            }
            break;
            case BUTTON:
            {
                qDebug() << "mode_tile BUTTON";
                const lclButton* b = static_cast<const lclButton*>(tile);
                switch (val)
                {
                case 0:
                {
                    b->button()->setEnabled(true);
                }
                break;
                case 1:
                {
                    b->button()->setEnabled(false);
                }
                break;
                case 2:
                {
                    b->button()->setFocus();
                }
                break;
                default:
                    return false;
                }
            }
            break;
            case RADIO_BUTTON:
            {
                qDebug() << "mode_tile RADIO_BUTTON";
                const lclButton* rb = static_cast<const lclButton*>(tile);
                switch (val)
                {
                case 0:
                {
                    rb->button()->setEnabled(true);
                }
                break;
                case 1:
                {
                    rb->button()->setEnabled(false);
                }
                break;
                case 2:
                {
                    rb->button()->setFocus();
                }
                break;
                default:
                    return false;
                }
            }
            break;
            case TEXT:
            {
                qDebug() << "mode_tile TEXT";
                const lclLabel* l = static_cast<const lclLabel*>(tile);
                switch (val)
                {
                case 0:
                {
                    l->label()->setEnabled(true);
                }
                break;
                case 1:
                {
                    l->label()->setEnabled(false);
                }
                break;
                case 2:
                {
                    l->label()->setFocus();
                }
                break;
                default:
                    return false;
                }
            }
            break;
            case POPUP_LIST:
            {
                qDebug() << "mode_tile POPUP_LIST";
                const lclPopupList* pl = static_cast<const lclPopupList*>(tile);
                switch (val)
                {
                case 0:
                {
                    pl->list()->setEnabled(true);
                }
                break;
                case 1:
                {
                    pl->list()->setEnabled(false);
                }
                break;
                case 2:
                {
                    pl->list()->setFocus();
                }
                break;
                default:
                    return false;
                }
            }
            break;
            default:
                return false;
            }
            return true;
        }
    }
    return false;
}

const char *RS_PythonGui::startList(const char *key, int operation, int index)
{
    //FIXME !env->find => ""

    if (operation == 0)
    {
        dclEnv->set("start_list_operation", lcl::integer(2));
    }
    else
    {
        dclEnv->set("start_list_operation", lcl::integer(operation));
    }

    if (index == 0)
    {
        dclEnv->set("start_list_index", lcl::nilValue());
    }
    else
    {
        dclEnv->set("start_list_index", lcl::integer(index));
    }
    dclEnv->set("start_list_key", lcl::string(key));

    return key;
}

const char *RS_PythonGui::addList(const char *val)
{
    const lclString  *key       = VALUE_CAST(lclString, dclEnv->get("start_list_key"));
    const lclInteger *operation = VALUE_CAST(lclInteger, dclEnv->get("start_list_operation"));
    const lclInteger *dialogId  = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    if (key)
    {
        qDebug() << "add_list key: " << key->value().c_str();
        for (auto & tile : dclTiles)
        {
            if(tile->value().dialog_Id != dialogId->value())
            {
                continue;
            }
            if (noQuotes(tile->value().key) == key->value())
            {
                if (tile->value().id == LIST_BOX)
                {
                    qDebug() << "add_list got LIST_BOX";
                    const lclListBox *lb = static_cast<const lclListBox*>(tile);
                    if(operation->value() == 1)
                    {
                        if(dclEnv->get("start_list_index").ptr()->print(true).compare("nil") == 0)
                        {
                            return "";
                        }
                        const lclInteger *index = VALUE_CAST(lclInteger, dclEnv->get("start_list_index"));
                        QListWidgetItem *item = lb->list()->item(index->value());
                        item->setText(val);
                        return val;
                    }
                    if(operation->value() == 3)
                    {
                        lb->list()->clear();
                    }
                    if(operation->value() == 2 ||
                        operation->value() == 3)
                    {
                        lb->list()->addItem(new QListWidgetItem(val, lb->list()));
                        return val;
                    }
                }
                if (tile->value().id == POPUP_LIST)
                {
                    qDebug() << "add_list got POPUP_LIST";
                    const lclPopupList *pl = static_cast<const lclPopupList*>(tile);
                    if(operation->value() == 1)
                    {
                        if(dclEnv->get("start_list_index").ptr()->print(true).compare("nil") == 0)
                        {
                            return "";
                        }
                        const lclInteger *index = VALUE_CAST(lclInteger, dclEnv->get("start_list_index"));
                        pl->list()->setItemText(index->value(), val);
                        return val;
                    }
                    if(operation->value() == 3)
                    {
                        pl->list()->clear();
                    }
                    if(operation->value() == 2 ||
                        operation->value() == 3)
                    {
                        pl->list()->addItem(val);
                        return val;
                    }
                }
            }
        }
    }
    return "";
}

void RS_PythonGui::prompt(const char *prompt)
{
    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(prompt);
        Py_CommandEdit->doProcess(false);

        String result = RS_Py_InputHandle::readLine(Py_CommandEdit).toStdString();
        Q_UNUSED(result);
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

void RS_PythonGui::Hello()
{
    qDebug() << "Hello, LibreCAD!";
}
