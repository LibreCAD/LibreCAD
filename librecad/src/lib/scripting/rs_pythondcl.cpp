#include "rs_python.h"
#include "rs_pythondcl.h"
#include "rs_dialogs.h"
#include "rs_py_inputhandle.h"
#include "LCL.h"
#include "Types.h"
#include "Environment.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

RS_PythonDcl::RS_PythonDcl()
{
}

RS_PythonDcl::~RS_PythonDcl()
{
}

int RS_PythonDcl::loadDialog(const char *fileName)
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

bool RS_PythonDcl::newDialog(const char *name, int id)
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

int RS_PythonDcl::startDialog()
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    if(dialogId)
    {
        for (auto & tile : dclTiles)
        {
            if (tile->value().dialog_Id == dialogId->value())
            {
                const lclDialog* dlg = static_cast<const lclDialog*>(tile);
                dlg->dialog()->show();
                dlg->dialog()->setFixedSize(dlg->dialog()->geometry().width(),
                                            dlg->dialog()->geometry().height());
                return dlg->dialog()->exec();
            }
        }
    }
    return -2;
}

void RS_PythonDcl::unloadDialog(int id)
{
    if(id)
    {
        for (auto & tile : dclTiles)
        {
            if (tile->value().dialog_Id == id)
            {
                const lclDialog* dlg = static_cast<const lclDialog*>(tile);
                delete dlg->dialog();

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

std::array<int, 2> RS_PythonDcl::doneDialog(int res)
{
    int result = -1;
    std::array<int, 2> dlgPos = {-0xffff , -0xffff};
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    if(dialogId)
    {
        for (auto & tile : dclTiles)
        {
            if (tile->value().dialog_Id == dialogId->value())
            {
                const lclDialog* dlg = static_cast<const lclDialog*>(tile);
                const lclInteger *dlg_result = VALUE_CAST(lclInteger, dclEnv->get(std::to_string(dialogId->value()) + "_dcl_result"));

                result = dlg_result->value();
                dlgPos = {dlg->dialog()->x(), dlg->dialog()->y()};

                qDebug() << "result:" << result;
                if (res > 1)
                {
                    result = res;
                }
                qDebug() << "res:" << res;
                dlg->dialog()->done(result);
                break;
            }
        }
    }
    return dlgPos;
}

bool RS_PythonDcl::setTile(const char *key, const char *val)
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
                break;
            case TOGGLE:
            {
                const lclToggle* tb = static_cast<const lclToggle*>(tile);
                if(String("0") == val)
                {
                    tb->toggle()->setChecked(false);
                }
                if(String("1") == val)
                {
                    tb->toggle()->setChecked(true);
                }
            }
                break;
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

bool RS_PythonDcl::actionTile(const char *id, const char *action)
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

const char *RS_PythonDcl::getTile(const char *key)
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
                qDebug() << "getTile EDIT_BOX";
                const lclEdit* e = static_cast<const lclEdit*>(tile);
                return qUtf8Printable(e->edit()->text());
            }
            break;
            case LIST_BOX:
            {
                qDebug() << "getTile LIST_BOX";
                const lclListBox* lb = static_cast<const lclListBox*>(tile);
                return std::to_string(lb->list()->currentRow()).c_str();
            }
            break;
            case BUTTON:
            {
                qDebug() << "getTile BUTTON";
                const lclButton* b = static_cast<const lclButton*>(tile);
                return qUtf8Printable(b->button()->text());
            }
            break;
            case RADIO_BUTTON:
            {
                qDebug() << "getTile RADIO_BUTTON";
                const lclButton* rb = static_cast<const lclButton*>(tile);
                return qUtf8Printable(rb->button()->text());
            }
            break;
            case TEXT:
            {
                qDebug() << "getTile TEXT";
                const lclLabel* l = static_cast<const lclLabel*>(tile);
                return qUtf8Printable(l->label()->text());
            }
            break;
            case POPUP_LIST:
            {
                qDebug() << "getTile POPUP_LIST";
                const lclPopupList* pl = static_cast<const lclPopupList*>(tile);
                return std::to_string(pl->list()->currentIndex()).c_str();
            }
            break;
            default:
                return "";
            }
        }
    }
    return "";
}

bool RS_PythonDcl::modeTile(const char *key, int val)
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
                qDebug() << "modeTile EDIT_BOX";
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
                qDebug() << "modeTile LIST_BOX";
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
                qDebug() << "modeTile BUTTON";
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
                qDebug() << "modeTile RADIO_BUTTON";
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
                qDebug() << "modeTile TEXT";
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
                qDebug() << "modeTile POPUP_LIST";
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

const char *RS_PythonDcl::startList(const char *key, int operation, int index)
{
    //FIXME !env->find => ""

    if (operation == -1)
    {
        dclEnv->set("start_list_operation", lcl::integer(2));
    }
    else
    {
        dclEnv->set("start_list_operation", lcl::integer(operation));
    }

    if (index == -1)
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

const char *RS_PythonDcl::addList(const char *val)
{
    const lclString  *key       = VALUE_CAST(lclString, dclEnv->get("start_list_key"));
    const lclInteger *operation = VALUE_CAST(lclInteger, dclEnv->get("start_list_operation"));
    const lclInteger *dialogId  = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    if (key)
    {
        qDebug() << "addList key: " << key->value().c_str();
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
                    qDebug() <<  "addList got LIST_BOX";
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
                        if(noQuotes(tile->value().value) != "")
                        {
                            bool ok;
                            int i = QString::fromStdString(noQuotes(tile->value().value)).toInt(&ok);

                            if (ok && lb->list()->count() == i)
                            {
                                lb->list()->setCurrentRow(i-1);
                            }
                        }
                        return val;
                    }
                }
                if (tile->value().id == POPUP_LIST)
                {
                    qDebug() <<  "addList got POPUP_LIST";
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
                        if(noQuotes(tile->value().value) != "")
                        {
                            bool ok;
                            int i = QString::fromStdString(noQuotes(tile->value().value)).toInt(&ok);

                            if (ok && pl->list()->count() == i)
                            {
                                pl->list()->setCurrentIndex(i-1);
                            }
                        }
                        return val;
                    }
                }
            }
        }
    }
    return "";
}

void RS_PythonDcl::endList()
{
    dclEnv->set("start_list_operation", lcl::nilValue());
    dclEnv->set("start_list_index", lcl::nilValue());
    dclEnv->set("start_list_key", lcl::nilValue());
}

int RS_PythonDcl::dimxTile(const char *key)
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
            case IMAGE:
            {
                return int(tile->value().width);
            }
            break;
            case IMAGE_BUTTON:
            {
                return 0;
            }
            break;
            default:
                return 0;
            }
        }
    }
    return 0;
}

int RS_PythonDcl::dimyTile(const char *key)
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
            case IMAGE:
            {
                return int(tile->value().height);
            }
            break;
            case IMAGE_BUTTON:
            {
                return 0;
            }
            break;
            default:
                return 0;
            }
        }
    }
    return 0;
}

int RS_PythonDcl::fillImage(int x1, int y1, int width, int height, int color)
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
            case IMAGE:
            {
                const lclImage* img = static_cast<const lclImage*>(tile);
                img->image()->addRect(x1, y1, width, height, color);
                return color;
            }
            break;
            case IMAGE_BUTTON:
            {
                return -1;
            }
            break;
            default:
                return -1;
            }
        }
    }
    return -1;
}

int RS_PythonDcl::vectorImage(int x1, int y1, int x2, int y2, int color)
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
            case IMAGE:
            {
                const lclImage* img = static_cast<const lclImage*>(tile);
                img->image()->addLine(x1, y1, x2, y2, color);
                return color;
            }
            break;
            case IMAGE_BUTTON:
            {
                return -1;
            }
            break;
            default:
                return -1;
            }
        }
    }
    return -1;
}

void RS_PythonDcl::endImage()
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
            case IMAGE:
            {
                const lclImage* img = static_cast<const lclImage*>(tile);
                img->image()->repaint();
            }
                break;
            case IMAGE_BUTTON:
            {

            }
                break;
            default: {}
                break;
            }
        }
    }
}

const char *RS_PythonDcl::startImage(const char *key)
{

    dclEnv->set("start_image_key", lcl::string(key));

    return key;

    // FIXMI check if not in current
    //return lcl::nilValue();
}
