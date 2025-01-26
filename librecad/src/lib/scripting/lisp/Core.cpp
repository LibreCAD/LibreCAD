#include "rs_python.h"
#include "rs_lisp.h"
#include "LCL.h"
#include "Environment.h"
#include "StaticList.h"
#include "Types.h"
#include "lisp.h"
#include "rs_dialogs.h"
#include "rs_lsp_inputhandle.h"

#include "rs_eventhandler.h"
#include "rs_graphicview.h"
#include "rs_entity.h"
#include "rs_arc.h"
#include "rs_block.h"
#include "rs_circle.h"
#include "rs_ellipse.h"
#include "rs_line.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_hatch.h"
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_mtext.h"
#include "rs_point.h"
#include "rs_text.h"
#include "rs_layer.h"
#include "rs_entitycontainer.h"
#include "rs_actionselectsingle.h"
#include "lc_undosection.h"
#include "qc_applicationwindow.h"
#include "qg_actionhandler.h"
#include "intern/qc_actiongetpoint.h"
#include "intern/qc_actiongetcorner.h"
#include "intern/qc_actionentsel.h"

#include "rs_filterdxfrw.h"

#ifdef DEVELOPER

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include <cstdlib>
#include <cctype>
#include <climits>
#include <chrono>
#include <ctime>
#include <cmath>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <random>

#include <QEventLoop>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QPainter>
#include <QPen>

/* temp defined */
#include <regex>

unsigned int tmpFileCount = 0;

typedef std::regex Regex;
static const Regex intRegex("[+-]?[0-9]+|[+-]?0[xX][0-9A-Fa-f]");
static const Regex floatRegex("[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?");
static const Regex floatPointRegex("[.]{1}\\d+$");

#include <math.h>
#include <cmath>

#define CHECK_ARGS_IS(expected) \
    checkArgsIs(name.c_str(), expected, \
                  std::distance(argsBegin, argsEnd))

#define CHECK_ARGS_BETWEEN(min, max) \
    checkArgsBetween(name.c_str(), min, max, \
                       std::distance(argsBegin, argsEnd))

#define CHECK_ARGS_AT_LEAST(expected) \
    checkArgsAtLeast(name.c_str(), expected, \
                        std::distance(argsBegin, argsEnd))

#define FLOAT_PTR \
    (argsBegin->ptr()->type() == LCLTYPE::REAL)

#define INT_PTR \
    (argsBegin->ptr()->type() == LCLTYPE::INT)

#define NIL_PTR \
    (argsBegin->ptr()->print(true).compare("nil") == 0)

#define TRUE_PTR \
    (argsBegin->ptr()->print(true).compare("true") == 0)

#define T_PTR \
    (argsBegin->ptr()->print(true).compare("T") == 0)

#define FALSE_PTR \
    (argsBegin->ptr()->print(true).compare("false") == 0)

bool argsHasFloat(lclValueIter argsBegin, lclValueIter argsEnd)
{
    for (auto it = argsBegin; it != argsEnd; ++it) {
        if (it->ptr()->type() == LCLTYPE::REAL) {
            return true;
        }
    }
    return false;
}

#define ARGS_HAS_FLOAT \
    argsHasFloat(argsBegin, argsEnd)

#define AG_INT(name) \
    CHECK_IS_NUMBER(argsBegin->ptr()) \
    lclInteger* name = VALUE_CAST(lclInteger, *argsBegin++)

#define ADD_INT_VAL(val) \
    CHECK_IS_NUMBER(argsBegin->ptr()) \
    lclInteger val = dynamic_cast<lclInteger*>(argsBegin->ptr());

#define ADD_FLOAT_VAL(val) \
    CHECK_IS_NUMBER(argsBegin->ptr()) \
    lclDouble val = dynamic_cast<lclDouble*>(argsBegin->ptr());

#define ADD_LIST_VAL(val) \
    lclList val = dynamic_cast<lclList*>(argsBegin->ptr());

#define SET_INT_VAL(opr, checkDivByZero) \
    ADD_INT_VAL(*intVal) \
    intValue = intValue opr intVal->value(); \
    if (checkDivByZero) { \
        LCL_CHECK(intVal->value() != 0, "Division by zero"); }

#define SET_FLOAT_VAL(opr, checkDivByZero) \
    if (FLOAT_PTR) \
    { \
        ADD_FLOAT_VAL(*floatVal) \
        floatValue = floatValue opr floatVal->value(); \
        if (checkDivByZero) { \
            LCL_CHECK(floatVal->value() != 0.0, "Division by zero"); } \
    } \
    else \
    { \
        ADD_INT_VAL(*intVal) \
        floatValue = floatValue opr double(intVal->value()); \
        if (checkDivByZero) { \
            LCL_CHECK(intVal->value() != 0, "Division by zero"); } \
    }

static String printValues(lclValueIter begin, lclValueIter end,
                           const String& sep, bool readably);

static int countValues(lclValueIter begin, lclValueIter end);

static StaticList<lclBuiltIn*> handlers;

#define ARG(type, name) type* name = VALUE_CAST(type, *argsBegin++)

#define FUNCNAME(uniq) builtIn ## uniq
#define HRECNAME(uniq) handler ## uniq
#define BUILTIN_DEF(uniq, symbol) \
    static lclBuiltIn::ApplyFunc FUNCNAME(uniq); \
    static StaticList<lclBuiltIn*>::Node HRECNAME(uniq) \
        (handlers, new lclBuiltIn(symbol, FUNCNAME(uniq))); \
    lclValuePtr FUNCNAME(uniq)(const String& name, \
        lclValueIter argsBegin, lclValueIter argsEnd)

#define BUILTIN(symbol)  BUILTIN_DEF(__LINE__, symbol)

#define BUILIN_ALIAS(uniq) \
    FUNCNAME(uniq)(name, argsBegin, argsEnd)

#define BUILTIN_ISA(symbol, type) \
    BUILTIN(symbol) { \
        CHECK_ARGS_IS(1); \
        return lcl::boolean(DYNAMIC_CAST(type, *argsBegin)); \
    }

#define BUILTIN_IS(op, constant) \
    BUILTIN(op) { \
        CHECK_ARGS_IS(1); \
        return lcl::boolean(*argsBegin == lcl::constant()); \
    }

#define BUILTIN_INTOP(op, checkDivByZero) \
    BUILTIN(#op) { \
        BUILTIN_VAL(op, checkDivByZero); \
        }

#define BUILTIN_VAL(opr, checkDivByZero) \
    int args = CHECK_ARGS_AT_LEAST(0); \
    if (args == 0) { \
        return lcl::integer(0); \
    } \
    if (args == 1) { \
        if (FLOAT_PTR) { \
            ADD_FLOAT_VAL(*floatValue) \
            return lcl::ldouble(floatValue->value()); \
        } else { \
            ADD_INT_VAL(*intValue) \
            return lcl::integer(intValue->value()); \
        } \
    } \
    if (ARGS_HAS_FLOAT) { \
        BUILTIN_FLOAT_VAL(opr, checkDivByZero) \
    } else { \
        BUILTIN_INT_VAL(opr, checkDivByZero) \
    }

#define BUILTIN_FLOAT_VAL(opr, checkDivByZero) \
    [[maybe_unused]] double floatValue = 0; \
    SET_FLOAT_VAL(+, false); \
    argsBegin++; \
    do { \
        SET_FLOAT_VAL(opr, checkDivByZero); \
        argsBegin++; \
    } while (argsBegin != argsEnd); \
    return lcl::ldouble(floatValue);

#define BUILTIN_INT_VAL(opr, checkDivByZero) \
    [[maybe_unused]] int64_t intValue = 0; \
    SET_INT_VAL(+, false); \
    argsBegin++; \
    do { \
        SET_INT_VAL(opr, checkDivByZero); \
        argsBegin++; \
    } while (argsBegin != argsEnd); \
    return lcl::integer(intValue);

#define BUILTIN_FUNCTION(foo) \
    CHECK_ARGS_IS(1); \
    if (FLOAT_PTR) { \
        ADD_FLOAT_VAL(*lhs) \
        return lcl::ldouble(foo(lhs->value())); } \
    else { \
        ADD_INT_VAL(*lhs) \
        return lcl::ldouble(foo(lhs->value())); }

#define BUILTIN_OP_COMPARE(opr) \
    CHECK_ARGS_IS(2); \
    if (((argsBegin->ptr()->type() == LCLTYPE::LIST) && ((argsBegin + 1)->ptr()->type() == LCLTYPE::LIST)) || \
        ((argsBegin->ptr()->type() == LCLTYPE::VEC) && ((argsBegin + 1)->ptr()->type() == LCLTYPE::VEC))) { \
        ARG(lclSequence, lhs); \
        ARG(lclSequence, rhs); \
        return lcl::boolean(lhs->count() opr rhs->count()); } \
    if (ARGS_HAS_FLOAT) { \
        if (FLOAT_PTR) { \
            ADD_FLOAT_VAL(*floatLhs) \
            argsBegin++; \
            if (FLOAT_PTR) { \
                ADD_FLOAT_VAL(*floatRhs) \
                return lcl::boolean(floatLhs->value() opr floatRhs->value()); } \
            else { \
               ADD_INT_VAL(*intRhs) \
               return lcl::boolean(floatLhs->value() opr double(intRhs->value())); } } \
        else { \
            ADD_INT_VAL(*intLhs) \
            argsBegin++; \
            ADD_FLOAT_VAL(*floatRhs) \
            return lcl::boolean(double(intLhs->value()) opr floatRhs->value()); } } \
    else { \
        ADD_INT_VAL(*intLhs) \
        argsBegin++; \
        ADD_INT_VAL(*intRhs) \
        return lcl::boolean(intLhs->value() opr intRhs->value()); }

// helper foo to cast integer (64 bit) type to char (8 bit) type
unsigned char itoa64(const int64_t &sign)
{
    int64_t bit64[8];
    unsigned char result = 0;

    if(sign < 0)
    {
        std::cout << "Warning: out of char value!" << std::endl;
        return result;
    }

    for (int i = 0; i < 8; i++)
    {
        bit64[i] = (sign >> i) & 1;
        if (bit64[i])
        {
            result |= 1 << i;
        }
    }
    return result;
}

bool compareNat(const String& a, const String& b)
{
    //std::cout << a << " " << b << std::endl;
    if (a.empty()) {
        return true;
    }
    if (b.empty()) {
        return false;
    }
    if (std::isdigit(a[0], std::locale()) && !std::isdigit(b[0], std::locale())) {
        return false;
    }
    if (!std::isdigit(a[0], std::locale()) && std::isdigit(b[0], std::locale())) {
        return false;
    }
    if (!std::isdigit(a[0], std::locale()) && !std::isdigit(b[0], std::locale())) {
        //std::cout << "HIT no dig" << std::endl;
        if (a[0] == '.' &&
            b[0] == '.' &&
            a.size() > 1 &&
            b.size() > 1) {
            return (std::toupper(a[1], std::locale()) < std::toupper(b[1], std::locale()));
        }

        if (std::toupper(a[0], std::locale()) == std::toupper(b[0], std::locale())) {
            return compareNat(a.substr(1), b.substr(1));
        }
        return (std::toupper(a[0], std::locale()) < std::toupper(b[0], std::locale()));
    }

    // Both strings begin with digit --> parse both numbers
    std::istringstream issa(a);
    std::istringstream issb(b);
    int ia, ib;
    issa >> ia;
    issb >> ib;
    if (ia != ib)
        return ia < ib;

    // Numbers are the same --> remove numbers and recurse
    String anew, bnew;
    std::getline(issa, anew);
    std::getline(issb, bnew);
    return (compareNat(anew, bnew));
}

template <typename TP>
std::time_t to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
              + system_clock::now());
    return system_clock::to_time_t(sctp);
}

BUILTIN_ISA("atom?",        lclAtom);
BUILTIN_ISA("double?",      lclDouble);
BUILTIN_ISA("file?",        lclFile);
BUILTIN_ISA("integer?",     lclInteger);
BUILTIN_ISA("keyword?",     lclKeyword);
BUILTIN_ISA("list?",        lclList);
BUILTIN_ISA("map?",         lclHash);
BUILTIN_ISA("sequential?",  lclSequence);
BUILTIN_ISA("string?",      lclString);
BUILTIN_ISA("symbol?",      lclSymbol);
BUILTIN_ISA("vector?",      lclVector);

BUILTIN_INTOP(+,            false);
BUILTIN_INTOP(/,            true);
BUILTIN_INTOP(*,            false);

BUILTIN_IS("true?",         trueValue);
BUILTIN_IS("false?",        falseValue);
BUILTIN_IS("nil?",          nilValue);

BUILTIN("-")
{
    int args = CHECK_ARGS_AT_LEAST(0);

    if (args == 0)
    {
        return lcl::integer(0);
    }

    if (args == 1)
    {
        if (FLOAT_PTR)
        {
            ADD_FLOAT_VAL(*lhs)
            return lcl::ldouble(-lhs->value());
        }
        else
        {
            ADD_INT_VAL(*lhs)
            return lcl::integer(-lhs->value());
        }
    }

    if (ARGS_HAS_FLOAT)
    {
        BUILTIN_FLOAT_VAL(-, false);
    }
    else
    {
        BUILTIN_INT_VAL(-, false);
    }
}

BUILTIN("%")
{
    CHECK_ARGS_AT_LEAST(2);
    if (ARGS_HAS_FLOAT) {
        return lcl::nilValue();
    } else {
        BUILTIN_INT_VAL(%, false);
    }
}

BUILTIN("<=")
{
    BUILTIN_OP_COMPARE(<=);
}

BUILTIN(">=")
{
    BUILTIN_OP_COMPARE(>=);
}

BUILTIN("<")
{
    BUILTIN_OP_COMPARE(<);
}

BUILTIN(">")
{
    BUILTIN_OP_COMPARE(>);
}

BUILTIN("=")
{
    CHECK_ARGS_IS(2);
    const lclValue* lhs = (*argsBegin++).ptr();
    const lclValue* rhs = (*argsBegin++).ptr();

    return lcl::boolean(lhs->isEqualTo(rhs));
}

BUILTIN("/=")
{
    CHECK_ARGS_IS(2);
    const lclValue* lhs = (*argsBegin++).ptr();
    const lclValue* rhs = (*argsBegin++).ptr();

    return lcl::boolean(!lhs->isEqualTo(rhs));
}

BUILTIN("~ ")
{
    CHECK_ARGS_IS(1);
    if (FLOAT_PTR)
    {
        return lcl::nilValue();
    }
    else
    {
        ADD_INT_VAL(*lhs)
        return lcl::integer(~lhs->value());
    }
}

BUILTIN("1+")
{
    CHECK_ARGS_IS(1);
    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        return lcl::ldouble(lhs->value()+1);
    }
    else
    {
        ADD_INT_VAL(*lhs)
        return lcl::integer(lhs->value()+1);
    }
}

BUILTIN("1-")
{
    CHECK_ARGS_IS(1);
    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        return lcl::ldouble(lhs->value()-1);
    }
    else
    {
        ADD_INT_VAL(*lhs)
        return lcl::integer(lhs->value()-1);
    }
}

BUILTIN("abs")
{
    CHECK_ARGS_IS(1);
    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        return lcl::ldouble(abs(lhs->value()));
    }
    else
    {
        ADD_INT_VAL(*lhs)
        return lcl::integer(abs(lhs->value()));
    }
}

BUILTIN("action_tile") {
    CHECK_ARGS_IS(2);
    ARG(lclString, id);
    ARG(lclString, action);

    const lclInteger *dialogId  = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    lclValuePtr value = dclEnv->get(std::to_string(dialogId->value()) + "_" + id->value());
    qDebug() << "value->print(true)" << value->print(true).c_str();
    if (value->print(true).compare("nil") == 0) {
        dclEnv->set(std::to_string(dialogId->value()) + "_" + id->value(), lcl::string(action->value()));
        return lcl::trueValue();
    }
    return lcl::nilValue();
}

BUILTIN("add_list")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, val);

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
                            return lcl::nilValue();
                        }
                        const lclInteger *index = VALUE_CAST(lclInteger, dclEnv->get("start_list_index"));
                        QListWidgetItem *item = lb->list()->item(index->value());
                        item->setText(val->value().c_str());
                        return lcl::string(val->value());
                    }
                    if(operation->value() == 3)
                    {
                        lb->list()->clear();
                    }
                    if(operation->value() == 2 ||
                        operation->value() == 3)
                    {
                        lb->list()->addItem(new QListWidgetItem(val->value().c_str(), lb->list()));

                        if(noQuotes(tile->value().value) != "")
                        {
                            bool ok;
                            int i = QString::fromStdString(noQuotes(tile->value().value)).toInt(&ok);

                            if (ok && lb->list()->count() == i)
                            {
                                lb->list()->setCurrentRow(i-1);
                            }
                        }

                        return lcl::string(val->value());
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
                            return lcl::nilValue();
                        }
                        const lclInteger *index = VALUE_CAST(lclInteger, dclEnv->get("start_list_index"));
                        pl->list()->setItemText(index->value(), val->value().c_str());
                        return lcl::string(val->value());
                    }
                    if(operation->value() == 3)
                    {
                        pl->list()->clear();
                    }
                    if(operation->value() == 2 ||
                        operation->value() == 3)
                    {
                        pl->list()->addItem(val->value().c_str());

                        if(noQuotes(tile->value().value) != "")
                        {
                            bool ok;
                            int i = QString::fromStdString(noQuotes(tile->value().value)).toInt(&ok);

                            if (ok && pl->list()->count() == i)
                            {
                                pl->list()->setCurrentIndex(i-1);
                            }
                        }

                        return lcl::string(val->value());
                    }
                }
            }
        }
    }
    return lcl::nilValue();
}

BUILTIN("alert")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, str);

    QMessageBox msgBox;
    msgBox.setWindowTitle("LibreCAD");
    msgBox.setText(str->value().c_str());
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();

    return lcl::nilValue();
}

BUILTIN("apply")
{
    CHECK_ARGS_AT_LEAST(2);
    lclValuePtr op = *argsBegin++; // this gets checked in APPLY

    // both LISPs
    if (op->type() == LCLTYPE::SYM ||
        op->type() == LCLTYPE::LIST) {
        op = EVAL(op, NULL);
    }

    // Copy the first N-1 arguments in.
    lclValueVec args(argsBegin, argsEnd-1);

    // Then append the argument as a list.
    const lclSequence* lastArg = VALUE_CAST(lclSequence, *(argsEnd-1));
    for (int i = 0; i < lastArg->count(); i++) {
        args.push_back(lastArg->item(i));
    }

    return APPLY(op, args.begin(), args.end());
}

BUILTIN("ascii")
{
    CHECK_ARGS_IS(1);
    const lclValuePtr arg = *argsBegin++;

    if (const lclString* s = DYNAMIC_CAST(lclString, arg))
    {
        return lcl::integer(int(s->value().c_str()[0]));
    }

    return lcl::integer(0);
}

BUILTIN("assoc")
{
    CHECK_ARGS_AT_LEAST(1);
    //both LISPs
    if (!(argsBegin->ptr()->type() == LCLTYPE::MAP)) {
        lclValuePtr op = *argsBegin++;
        ARG(lclSequence, seq);

        const int length = seq->count();
        lclValueVec* items = new lclValueVec(length);
        std::copy(seq->begin(), seq->end(), items->begin());

        for (int i = 0; i < length; i++) {
            if (items->at(i)->type() == LCLTYPE::LIST) {
                lclList* list = VALUE_CAST(lclList, items->at(i));
                if (list->count() == 2) {
                    lclValueVec* duo = new lclValueVec(2);
                    std::copy(list->begin(), list->end(), duo->begin());
                    if (duo->begin()->ptr()->print(true).compare(op->print(true)) == 0) {
                        return list;
                    }
                }
                if (list->count() == 3) {
                    lclValueVec* dotted = new lclValueVec(3);
                    std::copy(list->begin(), list->end(), dotted->begin());
                    if (dotted->begin()->ptr()->print(true).compare(op->print(true)) == 0
                        && (dotted->at(1)->print(true).compare(".") == 0)
                    ) {
                        return list;
                    }
                }
            }
        }
        return lcl::nilValue();
    }
    ARG(lclHash, hash);

    return hash->assoc(argsBegin, argsEnd);
}

BUILTIN("atan")
{
    BUILTIN_FUNCTION(atan);
}

BUILTIN("atof")
{
    CHECK_ARGS_IS(1);
    const lclValuePtr arg = *argsBegin++;

    if (const lclString* s = DYNAMIC_CAST(lclString, arg))
    {
        if(std::regex_match(s->value().c_str(), intRegex) ||
            std::regex_match(s->value().c_str(), floatRegex))
            {
                return lcl::ldouble(atof(s->value().c_str()));
            }
    }
    return lcl::ldouble(0);
}

BUILTIN("atoi")
{
    CHECK_ARGS_IS(1);
    const lclValuePtr arg = *argsBegin++;

    if (const lclString* s = DYNAMIC_CAST(lclString, arg))
    {
        if (std::regex_match(s->value().c_str(), intRegex))
        {
            return lcl::integer(atoi(s->value().c_str()));
        }
        if (std::regex_match(s->value().c_str(), floatRegex))
        {
            return lcl::integer(atoi(std::regex_replace(s->value().c_str(),
                                                        floatPointRegex, "").c_str()));
        }
    }
    return lcl::integer(0);
}

BUILTIN("atom")
{
    CHECK_ARGS_IS(1);

    return lcl::atom(*argsBegin);
}

BUILTIN("boolean?")
{
    CHECK_ARGS_IS(1);
    {
        return lcl::boolean(argsBegin->ptr()->type() == LCLTYPE::BOOLEAN);
    }
}

BUILTIN("bound?")
{
    CHECK_ARGS_IS(1);
    if (EVAL(*argsBegin, replEnv)->print(true).compare("nil") == 0)
    {
        return lcl::falseValue();
    }
    else
    {
        const lclSymbol* sym = DYNAMIC_CAST(lclSymbol, *argsBegin);
        if(!sym)
        {
            return lcl::falseValue();
        }
        else
        {
            if (replEnv->get(sym->value()) == lcl::nilValue())
            {
                return lcl::falseValue();
            }
        }
    }
    return lcl::trueValue();
}

BUILTIN("boundp")
{
    CHECK_ARGS_IS(1);
    if (EVAL(*argsBegin, replEnv)->print(true).compare("nil") == 0)
    {
        return lcl::nilValue();
    }
    else
    {
        const lclSymbol* sym = DYNAMIC_CAST(lclSymbol, *argsBegin);
        if(!sym)
        {
            return lcl::nilValue();
        }
        else
        {
            if (replEnv->get(sym->value()) == lcl::nilValue())
            {
                return lcl::nilValue();
            }
        }
    }
    return lcl::trueValue();
}

BUILTIN("car")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, seq);

    LCL_CHECK(0 < seq->count(), "Index out of range");

    return seq->first();
}

BUILTIN("cadr")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, seq);

    LCL_CHECK(1 < seq->count(), "Index out of range");

    return seq->item(1);
}

BUILTIN("caddr")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, seq);

    LCL_CHECK(2 < seq->count(), "Index out of range");

    return seq->item(2);
}

BUILTIN("cdr")
{
    CHECK_ARGS_IS(1);
    if (*argsBegin == lcl::nilValue()) {
        return lcl::list(new lclValueVec(0));
    }
    ARG(lclSequence, seq);
    if (seq->isDotted()) {
            return seq->dotted();
    }
    return seq->rest();
}


BUILTIN("chr")
{
    CHECK_ARGS_IS(1);
    unsigned char sign = 0;

    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        auto sign64 = static_cast<std::int64_t>(lhs->value());
        sign = itoa64(sign64);
    }
    else
    {
        ADD_INT_VAL(*lhs)
        sign = itoa64(lhs->value());
    }

    return lcl::string(String(1 , sign));
}

BUILTIN("close")
{
    CHECK_ARGS_IS(1);
    ARG(lclFile, pf);

    return pf->close();
}

BUILTIN("command")
{
    CHECK_ARGS_AT_LEAST(1);

    QG_ActionHandler* actionHandler = nullptr;
    actionHandler = QC_ApplicationWindow::getAppWindow()->getActionHandler();
    QString cmd = "";

    for (auto it = argsBegin; it != argsEnd; ++it)
    {
        switch(it->ptr()->type())
        {
            case LCLTYPE::STR:
            {
                const lclString* s = DYNAMIC_CAST(lclString,*it);
                cmd = s->value().c_str();
                cmd = cmd.simplified();

                if (actionHandler) {
                    actionHandler->command(cmd);
                    qDebug() << "command:" << cmd;
                    cmd = "";
                }
            }
                break;
            case LCLTYPE::LIST:
            {
                const lclList* l = DYNAMIC_CAST(lclList, *it);

                for (int i = 0; i < l->count(); i++)
                {
                    cmd += l->item(i)->print(true).c_str();
                    if (i < l->count()-1)
                    {
                        cmd += ",";
                    }
                }

                cmd = cmd.simplified();

                if (actionHandler) {
                    actionHandler->command(cmd);
                    qDebug() << "command:" << cmd;
                    cmd = "";
                }
            }
                break;

            case LCLTYPE::INT:
            case LCLTYPE::REAL:
            {
                cmd += it->ptr()->print(true).c_str();
                cmd = cmd.simplified();

                if (actionHandler) {
                    actionHandler->command(cmd);
                    qDebug() << "command:" << cmd;
                    cmd = "";
                }
            }
                break;
            default:
            {
                if (actionHandler) {
                    actionHandler->command("");
                    qDebug() << "command: ''";
                    cmd = "";
                }
            }
                break;
        }
        std::cout << "parameter: " << it->ptr()->print(true) << " type: " << (int)it->ptr()->type() << std::endl;
    }

    return lcl::nilValue();
}

BUILTIN("concat")
{
    Q_UNUSED(name);
    int count = 0;
    for (auto it = argsBegin; it != argsEnd; ++it) {
        const lclSequence* seq = VALUE_CAST(lclSequence, *it);
        count += seq->count();
    }

    lclValueVec* items = new lclValueVec(count);
    int offset = 0;
    for (auto it = argsBegin; it != argsEnd; ++it) {
        const lclSequence* seq = STATIC_CAST(lclSequence, *it);
        std::copy(seq->begin(), seq->end(), items->begin() + offset);
        offset += seq->count();
    }

    return lcl::list(items);
}
#if 0
BUILTIN("bla")
{
    return BUILIN_ALIAS(543);
    //return builtIn540(name, argsBegin, argsEnd);
}
#endif
BUILTIN("conj")
{
    CHECK_ARGS_AT_LEAST(1);
    ARG(lclSequence, seq);

    return seq->conj(argsBegin, argsEnd);
}

BUILTIN("cons")
{
    CHECK_ARGS_IS(2);
    lclValuePtr first = *argsBegin++;
    lclValuePtr second = *argsBegin;

    if (second->type() == LCLTYPE::INT ||
        second->type() == LCLTYPE::REAL ||
        second->type() == LCLTYPE::STR)
    {
        lclValueVec* items = new lclValueVec(3);
        items->at(0) = first;
        items->at(1) = new lclSymbol(".");
        items->at(2) = second;
        return lcl::list(items);
    }

    ARG(lclSequence, rest);

    lclValueVec* items = new lclValueVec(1 + rest->count());
    items->at(0) = first;
    std::copy(rest->begin(), rest->end(), items->begin() + 1);

    return lcl::list(items);
}

BUILTIN("contains?")
{
    CHECK_ARGS_IS(2);
    if (*argsBegin == lcl::nilValue()) {
        return *argsBegin;
    }
    ARG(lclHash, hash);
    return lcl::boolean(hash->contains(*argsBegin));
}

BUILTIN("copyright")
{
    CHECK_ARGS_IS(0);
    QFile f(":/readme.md");
    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        return lcl::nilValue();
    }
    QTextStream in(&f);
    std::cout << in.readAll().toStdString();

    return lcl::nilValue();
}

BUILTIN("cos")
{
    BUILTIN_FUNCTION(cos);
}

BUILTIN("count")
{
    CHECK_ARGS_IS(1);
    if (*argsBegin == lcl::nilValue()) {
        return lcl::integer(0);
    }

    ARG(lclSequence, seq);
    return lcl::integer(seq->count());
}

BUILTIN("credits")
{
    CHECK_ARGS_IS(0);
    std::cout << "Thanks to all the people supporting LibreCAD for supporting LibreCAD development. See https://dokuwiki.librecad.org for more information." << std::endl;
    return lcl::nilValue();
}

BUILTIN("deref")
{
    CHECK_ARGS_IS(1);
    ARG(lclAtom, atom);

    return atom->deref();
}

BUILTIN("dimx_tile")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, key);

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
                return lcl::integer(int(tile->value().width));
            }
            break;
            case IMAGE_BUTTON:
            {
                return lcl::integer(int(tile->value().width));
            }
            break;
            default:
                return lcl::nilValue();
            }
        }
    }
    return lcl::nilValue();
}

BUILTIN("dimy_tile")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, key);

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
                return lcl::integer(int(tile->value().height));
            }
            break;
            case IMAGE_BUTTON:
            {
                return lcl::integer(int(tile->value().height));
            }
            break;
            default:
                return lcl::nilValue();
            }
        }
    }
    return lcl::nilValue();
}

BUILTIN("dissoc")
{
    CHECK_ARGS_AT_LEAST(1);
    ARG(lclHash, hash);

    return hash->dissoc(argsBegin, argsEnd);
}

BUILTIN("done_dialog") {
    int args = CHECK_ARGS_BETWEEN(0, 1);
    int result = -1;
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

                lclValueVec* items = new lclValueVec(2);
                items->at(0) = lcl::integer(dlg->dialog()->x());
                items->at(1) = lcl::integer(dlg->dialog()->y());

                if (args == 1)
                {
                    AG_INT(val);
                    if (val->value() > 1)
                    {
                        result = val->value();
                    }
                }
                // FIXME do try eval list??
                dlg->dialog()->done(result);
                return lcl::list(items);
            }
        }
    }
    return lcl::nilValue();
}

BUILTIN("empty?")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, seq);

    return lcl::boolean(seq->isEmpty());
}

BUILTIN("end_image")
{
    CHECK_ARGS_IS(0);
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
                    const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                    img->button()->repaint();
                }
                break;
                default: {}
            }
        }
    }

    return dclEnv->set("start_image_key",  lcl::nilValue());
}

BUILTIN("end_list")
{
    CHECK_ARGS_IS(0);
    dclEnv->set("start_list_operation", lcl::nilValue());
    dclEnv->set("start_list_index", lcl::nilValue());
    return dclEnv->set("start_list_key", lcl::nilValue());
}

BUILTIN("entdel")
{
    CHECK_ARGS_IS(1);
    ARG(lclEname, en);

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();
    RS_EntityContainer* entityContainer = graphicView->getContainer();
    LC_UndoSection undo(doc, graphicView);

    if(entityContainer->count())
    {
        for (auto e: *entityContainer) {
            if (e->getId() == en->value())
            {
                e->setSelected(false);
                e->changeUndoState();
                undo.addUndoable(e);
                graphicView->redraw(RS2::RedrawDrawing);
                return lcl::ename(en->value());
            }
        }
    }

    return lcl::nilValue();
}

BUILTIN("entget")
{
    CHECK_ARGS_IS(1);
    ARG(lclEname, en);

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_GraphicView* graphicView = appWin->getGraphicView();
    RS_EntityContainer* entityContainer = graphicView->getContainer();

    if(entityContainer->count())
    {
        for (auto e: *entityContainer) {
            if (e->getId() == en->value())
            {
                lclValueVec *entity = new lclValueVec;

                lclValueVec *ename = new lclValueVec(3);
                ename->at(0) = lcl::integer(-1);
                ename->at(1) = lcl::symbol(".");
                ename->at(2) = lcl::ename(en->value());

                lclValueVec *ebname = new lclValueVec(3);
                ebname->at(0) = lcl::integer(330);
                ebname->at(1) = lcl::symbol(".");
                ebname->at(2) = lcl::ename(en->value());

                lclValueVec *handle = new lclValueVec(3);
                handle->at(0) = lcl::integer(5);
                handle->at(1) = lcl::symbol(".");
                handle->at(2) = lcl::string("6A");

                lclValueVec *acdb = new lclValueVec(3);
                acdb->at(0) = lcl::integer(100);
                acdb->at(1) = lcl::symbol(".");
                acdb->at(2) = lcl::string("AcDbEntity");

                lclValueVec *mspace = new lclValueVec(3);
                mspace->at(0) = lcl::integer(67);
                mspace->at(1) = lcl::symbol(".");
                mspace->at(2) = lcl::integer(0);

                lclValueVec *layoutTabName = new lclValueVec(3);
                layoutTabName->at(0) = lcl::integer(100);
                layoutTabName->at(1) = lcl::symbol(".");
                layoutTabName->at(2) = lcl::string("Model");

                lclValueVec *layer = new lclValueVec(3);
                layer->at(0) = lcl::integer(8);
                layer->at(1) = lcl::symbol(".");
                layer->at(2) = lcl::string(e->getLayer()->getName().toStdString());

                lclValueVec *extrDir = new lclValueVec(4);
                extrDir->at(0) = lcl::integer(210);
                extrDir->at(1) = lcl::ldouble(0.0);
                extrDir->at(2) = lcl::ldouble(0.0);
                extrDir->at(3) = lcl::ldouble(1.0);

                switch (e->rtti())
                {
                    case RS2::EntityPoint:
                    {
                        RS_Point* p = (RS_Point*)e;
                        entity->push_back(lcl::list(ename));

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("POINT");
                        entity->push_back(lcl::list(name));

                        entity->push_back(lcl::list(ebname));
                        entity->push_back(lcl::list(handle));
                        entity->push_back(lcl::list(acdb));
                        entity->push_back(lcl::list(mspace));
                        entity->push_back(lcl::list(layoutTabName));
                        entity->push_back(lcl::list(layer));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbPoint");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *pnt = new lclValueVec(4);
                        pnt->at(0) = lcl::integer(10);
                        pnt->at(1) = lcl::ldouble(p->getPos().x);
                        pnt->at(2) = lcl::ldouble(p->getPos().y);
                        pnt->at(3) = lcl::ldouble(p->getPos().z);
                        entity->push_back(lcl::list(pnt));

                        entity->push_back(lcl::list(extrDir));

                        return lcl::list(entity);
                    }
                    break;

                    case RS2::EntityLine:
                    {
                        RS_Line* l = (RS_Line*)e;
                        entity->push_back(lcl::list(ename));

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("LINE");
                        entity->push_back(lcl::list(name));

                        entity->push_back(lcl::list(ebname));
                        entity->push_back(lcl::list(handle));
                        entity->push_back(lcl::list(acdb));
                        entity->push_back(lcl::list(mspace));
                        entity->push_back(lcl::list(layoutTabName));
                        entity->push_back(lcl::list(layer));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbLine");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *startpnt = new lclValueVec(4);
                        startpnt->at(0) = lcl::integer(10);
                        startpnt->at(1) = lcl::ldouble(l->getStartpoint().x);
                        startpnt->at(2) = lcl::ldouble(l->getStartpoint().y);
                        startpnt->at(3) = lcl::ldouble(l->getStartpoint().z);
                        entity->push_back(lcl::list(startpnt));

                        lclValueVec *endpnt = new lclValueVec(4);
                        endpnt->at(0) = lcl::integer(11);
                        endpnt->at(1) = lcl::ldouble(l->getEndpoint().x);
                        endpnt->at(2) = lcl::ldouble(l->getEndpoint().y);
                        endpnt->at(3) = lcl::ldouble(l->getEndpoint().z);
                        entity->push_back(lcl::list(endpnt));

                        entity->push_back(lcl::list(extrDir));

                        return lcl::list(entity);
                    }
                    break;

                    case RS2::EntityArc:
                    {

                        RS_Arc* a = (RS_Arc*)e;
                        entity->push_back(lcl::list(ename));
#if 0
                        a->getStartpoint();
                        a->getEndpoint();
                        (int)a->isReversed();
#endif

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("ARC");
                        entity->push_back(lcl::list(name));

                        entity->push_back(lcl::list(ebname));
                        entity->push_back(lcl::list(handle));
                        entity->push_back(lcl::list(acdb));
                        entity->push_back(lcl::list(mspace));
                        entity->push_back(lcl::list(layoutTabName));
                        entity->push_back(lcl::list(layer));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbArc");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *center = new lclValueVec(4);
                        center->at(0) = lcl::integer(10);
                        center->at(1) = lcl::ldouble(a->getCenter().x);
                        center->at(2) = lcl::ldouble(a->getCenter().y);
                        center->at(3) = lcl::ldouble(a->getCenter().z);
                        entity->push_back(lcl::list(center));

                        lclValueVec *radius = new lclValueVec(3);
                        radius->at(0) = lcl::integer(40);
                        radius->at(1) = lcl::symbol(".");
                        radius->at(2) = lcl::ldouble(a->getRadius());
                        entity->push_back(lcl::list(radius));

                        lclValueVec *startAngle = new lclValueVec(3);
                        startAngle->at(0) = lcl::integer(50);
                        startAngle->at(1) = lcl::symbol(".");
                        startAngle->at(2) = lcl::ldouble(a->getAngle1());
                        entity->push_back(lcl::list(startAngle));

                        lclValueVec *endAngle = new lclValueVec(3);
                        endAngle->at(0) = lcl::integer(51);
                        endAngle->at(1) = lcl::symbol(".");
                        endAngle->at(2) = lcl::ldouble(a->getAngle2());
                        entity->push_back(lcl::list(endAngle));

                        entity->push_back(lcl::list(extrDir));

                        return lcl::list(entity);
                    }
                    break;

                    case RS2::EntityCircle:
                    {
                        RS_Circle* c = (RS_Circle*)e;
                        entity->push_back(lcl::list(ename));

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("CIRCLE");
                        entity->push_back(lcl::list(name));

                        entity->push_back(lcl::list(ebname));
                        entity->push_back(lcl::list(handle));
                        entity->push_back(lcl::list(acdb));
                        entity->push_back(lcl::list(mspace));
                        entity->push_back(lcl::list(layoutTabName));
                        entity->push_back(lcl::list(layer));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbCircle");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *center = new lclValueVec(4);
                        center->at(0) = lcl::integer(10);
                        center->at(1) = lcl::ldouble(c->getCenter().x);
                        center->at(2) = lcl::ldouble(c->getCenter().y);
                        center->at(3) = lcl::ldouble(c->getCenter().z);
                        entity->push_back(lcl::list(center));

                        lclValueVec *radius = new lclValueVec(3);
                        radius->at(0) = lcl::integer(40);
                        radius->at(1) = lcl::symbol(".");
                        radius->at(2) = lcl::ldouble(c->getRadius());
                        entity->push_back(lcl::list(radius));

                        entity->push_back(lcl::list(extrDir));

                        return lcl::list(entity);
                    }
                    break;

                case RS2::EntityEllipse:
                {
                    RS_Ellipse* ellipse=static_cast<RS_Ellipse*>(e);
                    entity->push_back(lcl::list(ename));

                    lclValueVec *name = new lclValueVec(3);
                    name->at(0) = lcl::integer(0);
                    name->at(1) = lcl::symbol(".");
                    name->at(2) = lcl::string("ELLIPSE");
                    entity->push_back(lcl::list(name));

                    entity->push_back(lcl::list(ebname));
                    entity->push_back(lcl::list(handle));
                    entity->push_back(lcl::list(acdb));
                    entity->push_back(lcl::list(mspace));
                    entity->push_back(lcl::list(layoutTabName));
                    entity->push_back(lcl::list(layer));

                    lclValueVec *acdbL = new lclValueVec(3);
                    acdbL->at(0) = lcl::integer(100);
                    acdbL->at(1) = lcl::symbol(".");
                    acdbL->at(2) = lcl::string("AcDbEllipse");
                    entity->push_back(lcl::list(acdbL));

                    lclValueVec *center = new lclValueVec(4);
                    center->at(0) = lcl::integer(10);
                    center->at(1) = lcl::ldouble(ellipse->getCenter().x);
                    center->at(2) = lcl::ldouble(ellipse->getCenter().y);
                    center->at(3) = lcl::ldouble(ellipse->getCenter().z);
                    entity->push_back(lcl::list(center));

                    lclValueVec *majorpnt = new lclValueVec(4);
                    majorpnt->at(0) = lcl::integer(11);
                    majorpnt->at(1) = lcl::ldouble(ellipse->getMajorP().x);
                    majorpnt->at(2) = lcl::ldouble(ellipse->getMajorP().y);
                    majorpnt->at(3) = lcl::ldouble(ellipse->getMajorP().z);
                    entity->push_back(lcl::list(majorpnt));

                    entity->push_back(lcl::list(extrDir));

                    lclValueVec *ratio = new lclValueVec(3);
                    ratio->at(0) = lcl::integer(40);
                    ratio->at(1) = lcl::symbol(".");
                    ratio->at(2) = lcl::ldouble(ellipse->getRatio());
                    entity->push_back(lcl::list(ratio));

                    // 41 42



                    return lcl::list(entity);
                }
                break;

                    case RS2::EntityDimAligned:
                    {
#if 0
                        RS_DimAligned* d = (RS_DimAligned*)e;
                        d->getDefinitionPoint();
                        d->getExtensionPoint1();
                        d->getExtensionPoint2();
                        d->getText().toLatin1().data();
                        d->getLabel().toLatin1().data();
#endif
                    }
                    break;

                    case RS2::EntityDimLinear:
                    {
#if 0
                        RS_DimLinear* d = (RS_DimLinear*)e;
                        d->getDefinitionPoint();
                        d->getExtensionPoint1();
                        d->getExtensionPoint2();
                        d->getText().toLatin1().data();
                        d->getLabel().toLatin1().data();
#endif
                    }
                    break;

                    case RS2::EntityInsert:
                    {
                        RS_Insert* i = (RS_Insert*)e;

                        entity->push_back(lcl::list(ename));

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("INSERT");
                        entity->push_back(lcl::list(name));

                        entity->push_back(lcl::list(ebname));
                        entity->push_back(lcl::list(handle));
                        entity->push_back(lcl::list(acdb));
                        entity->push_back(lcl::list(mspace));
                        entity->push_back(lcl::list(layoutTabName));
                        entity->push_back(lcl::list(layer));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbBlockReference");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *blockName = new lclValueVec(3);
                        blockName->at(0) = lcl::integer(2);
                        blockName->at(1) = lcl::symbol(".");
                        blockName->at(2) = lcl::string(i->getName().toStdString());
                        entity->push_back(lcl::list(blockName));

                        lclValueVec *center = new lclValueVec(4);
                        center->at(0) = lcl::integer(10);
                        center->at(1) = lcl::ldouble(i->getInsertionPoint().x);
                        center->at(2) = lcl::ldouble(i->getInsertionPoint().y);
                        center->at(3) = lcl::ldouble(i->getInsertionPoint().z);
                        entity->push_back(lcl::list(center));

                        lclValueVec *scaleX = new lclValueVec(3);
                        scaleX->at(0) = lcl::integer(41);
                        scaleX->at(1) = lcl::symbol(".");
                        scaleX->at(2) = lcl::ldouble(i->getScale().x);
                        entity->push_back(lcl::list(scaleX));

                        lclValueVec *scaleY = new lclValueVec(3);
                        scaleY->at(0) = lcl::integer(42);
                        scaleY->at(1) = lcl::symbol(".");
                        scaleY->at(2) = lcl::ldouble(i->getScale().y);
                        entity->push_back(lcl::list(scaleY));

                        lclValueVec *radius = new lclValueVec(3);
                        radius->at(0) = lcl::integer(50);
                        radius->at(1) = lcl::symbol(".");
                        radius->at(2) = lcl::ldouble(i->getRadius());
                        entity->push_back(lcl::list(radius));

                        lclValueVec *columnCount = new lclValueVec(3);
                        columnCount->at(0) = lcl::integer(70);
                        columnCount->at(1) = lcl::symbol(".");
                        columnCount->at(2) = lcl::ldouble(i->getCols());
                        entity->push_back(lcl::list(columnCount));

                        lclValueVec *rowCount = new lclValueVec(3);
                        rowCount->at(0) = lcl::integer(71);
                        rowCount->at(1) = lcl::symbol(".");
                        rowCount->at(2) = lcl::ldouble(i->getCols());
                        entity->push_back(lcl::list(rowCount));

                        lclValueVec *columnSpacing = new lclValueVec(3);
                        columnSpacing->at(0) = lcl::integer(44);
                        columnSpacing->at(1) = lcl::symbol(".");
                        columnSpacing->at(2) = lcl::ldouble(i->getSpacing().y);
                        entity->push_back(lcl::list(columnSpacing));

                        lclValueVec *rowSpacing = new lclValueVec(3);
                        rowSpacing->at(0) = lcl::integer(45);
                        rowSpacing->at(1) = lcl::symbol(".");
                        rowSpacing->at(2) = lcl::ldouble(i->getSpacing().x);
                        entity->push_back(lcl::list(rowSpacing));

                        entity->push_back(lcl::list(extrDir));

                        return lcl::list(entity);
                    }
                    break;

                    case RS2::EntityMText:
                    {
                        RS_MText* t = (RS_MText*)e;

                        entity->push_back(lcl::list(ename));

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("MTEXT");
                        entity->push_back(lcl::list(name));

                        entity->push_back(lcl::list(ebname));
                        entity->push_back(lcl::list(handle));
                        entity->push_back(lcl::list(acdb));
                        entity->push_back(lcl::list(mspace));
                        entity->push_back(lcl::list(layoutTabName));
                        entity->push_back(lcl::list(layer));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbMText");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *pnt = new lclValueVec(4);
                        pnt->at(0) = lcl::integer(10);
                        pnt->at(1) = lcl::ldouble(t->getInsertionPoint().x);
                        pnt->at(2) = lcl::ldouble(t->getInsertionPoint().y);
                        pnt->at(3) = lcl::ldouble(t->getInsertionPoint().z);
                        entity->push_back(lcl::list(pnt));

                        lclValueVec *textHeight = new lclValueVec(3);
                        textHeight->at(0) = lcl::integer(40);
                        textHeight->at(1) = lcl::symbol(".");
                        textHeight->at(2) = lcl::ldouble(t->getUsedTextHeight());
                        entity->push_back(lcl::list(textHeight));

                        lclValueVec *width = new lclValueVec(3);
                        width->at(0) = lcl::integer(41);
                        width->at(1) = lcl::symbol(".");
                        width->at(2) = lcl::ldouble(t->getWidth());
                        entity->push_back(lcl::list(width));

                        lclValueVec *alignment = new lclValueVec(3);
                        alignment->at(0) = lcl::integer(71);
                        alignment->at(1) = lcl::symbol(".");
                        alignment->at(2) = lcl::integer(t->getAlignment());
                        entity->push_back(lcl::list(alignment));

                        lclValueVec *drawingDirection = new lclValueVec(3);
                        drawingDirection->at(0) = lcl::integer(72);
                        drawingDirection->at(1) = lcl::symbol(".");

                        RS_MTextData::MTextDrawingDirection dir = t->getDrawingDirection();
                        unsigned int dxfDir = 5;

                        switch(dir)
                        {
                            case RS_MTextData::MTextDrawingDirection::LeftToRight:
                                dxfDir = 1;
                                break;
                            case RS_MTextData::MTextDrawingDirection::RightToLeft:
                                dxfDir = 2;
                                break;
                            case RS_MTextData::MTextDrawingDirection::TopToBottom:
                                dxfDir = 3;
                                break;
                            default:
                                dxfDir = 5;
                        }

                        drawingDirection->at(2) = lcl::integer(dxfDir);
                        entity->push_back(lcl::list(drawingDirection));

                        lclValueVec *text = new lclValueVec(3);
                        text->at(0) = lcl::integer(1);
                        text->at(1) = lcl::symbol(".");
                        text->at(2) = lcl::string(qUtf8Printable(t->getText()));
                        entity->push_back(lcl::list(text));

                        if (t->getStyle() != "STANDARD")
                        {
                            lclValueVec *style = new lclValueVec(3);
                            style->at(0) = lcl::integer(7);
                            style->at(1) = lcl::symbol(".");
                            style->at(2) = lcl::string(qUtf8Printable(t->getText()));
                            entity->push_back(lcl::list(style));
                        }

                        lclValueVec *height = new lclValueVec(3);
                        height->at(0) = lcl::integer(43);
                        height->at(1) = lcl::symbol(".");
                        height->at(2) = lcl::ldouble(t->getHeight());
                        entity->push_back(lcl::list(height));

                        lclValueVec *angle = new lclValueVec(3);
                        angle->at(0) = lcl::integer(50);
                        angle->at(1) = lcl::symbol(".");
                        angle->at(2) = lcl::ldouble(t->getAngle());
                        entity->push_back(lcl::list(angle));

                        lclValueVec *lineSpacingStyle = new lclValueVec(3);
                        lineSpacingStyle->at(0) = lcl::integer(73);
                        lineSpacingStyle->at(1) = lcl::symbol(".");

                        RS_MTextData::MTextLineSpacingStyle style = t->getLineSpacingStyle();
                        unsigned int styleDxf = 2;

                        if (style == RS_MTextData::MTextLineSpacingStyle::AtLeast)
                        {
                            styleDxf = 1;
                        }

                        lineSpacingStyle->at(2) = lcl::integer(styleDxf);
                        entity->push_back(lcl::list(lineSpacingStyle));

                        lclValueVec *textSpace = new lclValueVec(3);
                        textSpace->at(0) = lcl::integer(44);
                        textSpace->at(1) = lcl::symbol(".");
                        textSpace->at(2) = lcl::ldouble(t->getLineSpacingFactor());
                        entity->push_back(lcl::list(textSpace));

                        entity->push_back(lcl::list(extrDir));

                        return lcl::list(entity);
                    }
                    break;

                    case RS2::EntityText:
                    {
                        RS_Text* t = (RS_Text*)e;
                        entity->push_back(lcl::list(ename));

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("TEXT");
                        entity->push_back(lcl::list(name));

                        entity->push_back(lcl::list(ebname));
                        entity->push_back(lcl::list(handle));
                        entity->push_back(lcl::list(acdb));
                        entity->push_back(lcl::list(mspace));
                        entity->push_back(lcl::list(layoutTabName));
                        entity->push_back(lcl::list(layer));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbText");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *pnt = new lclValueVec(4);
                        pnt->at(0) = lcl::integer(10);
                        pnt->at(1) = lcl::ldouble(t->getInsertionPoint().x);
                        pnt->at(2) = lcl::ldouble(t->getInsertionPoint().y);
                        pnt->at(3) = lcl::ldouble(t->getInsertionPoint().z);
                        entity->push_back(lcl::list(pnt));

                        lclValueVec *height = new lclValueVec(3);
                        height->at(0) = lcl::integer(40);
                        height->at(1) = lcl::symbol(".");
                        height->at(2) = lcl::ldouble(t->getHeight());
                        entity->push_back(lcl::list(height));

                        lclValueVec *text = new lclValueVec(3);
                        text->at(0) = lcl::integer(1);
                        text->at(1) = lcl::symbol(".");
                        text->at(2) = lcl::string(qUtf8Printable(t->getText()));
                        entity->push_back(lcl::list(text));

                        if (t->getAngle() != 0.0)
                        {
                            lclValueVec *angle = new lclValueVec(3);
                            angle->at(0) = lcl::integer(50);
                            angle->at(1) = lcl::symbol(".");
                            angle->at(2) = lcl::ldouble(t->getAngle());
                            entity->push_back(lcl::list(angle));
                        }

                        if (t->getStyle() != "STANDARD")
                        {
                            lclValueVec *style = new lclValueVec(3);
                            style->at(0) = lcl::integer(50);
                            style->at(1) = lcl::symbol(".");
                            style->at(2) = lcl::string(qUtf8Printable(t->getText()));
                            entity->push_back(lcl::list(style));
                        }

                        entity->push_back(lcl::list(extrDir));

                        return lcl::list(entity);
                    }
                    break;

                    case RS2::EntityHatch:
                    {
#if 0
                        RS_Hatch* h = (RS_Hatch*)e;

                        qUtf8Printable(h->getPattern());
                        h->getScale();
                        (int)h->isSolid();

                        entity->push_back(lcl::list(ename));

                        lclValueVec *name = new lclValueVec(3);
                        name->at(0) = lcl::integer(0);
                        name->at(1) = lcl::symbol(".");
                        name->at(2) = lcl::string("HATCH");
                        entity->push_back(lcl::list(name));

                        entity->push_back(lcl::list(ebname));
                        entity->push_back(lcl::list(handle));
                        entity->push_back(lcl::list(acdb));
                        entity->push_back(lcl::list(mspace));
                        entity->push_back(lcl::list(layoutTabName));
                        entity->push_back(lcl::list(layer));

                        lclValueVec *acdbL = new lclValueVec(3);
                        acdbL->at(0) = lcl::integer(100);
                        acdbL->at(1) = lcl::symbol(".");
                        acdbL->at(2) = lcl::string("AcDbHatch");
                        entity->push_back(lcl::list(acdbL));

                        lclValueVec *startpnt = new lclValueVec(4);
                        startpnt->at(0) = lcl::integer(10);
                        startpnt->at(1) = lcl::ldouble(l->getStartpoint().x);
                        startpnt->at(2) = lcl::ldouble(l->getStartpoint().y);
                        startpnt->at(3) = lcl::ldouble(l->getStartpoint().z);
                        entity->push_back(lcl::list(startpnt));

                        lclValueVec *endpnt = new lclValueVec(4);
                        endpnt->at(0) = lcl::integer(11);
                        endpnt->at(1) = lcl::ldouble(l->getEndpoint().x);
                        endpnt->at(2) = lcl::ldouble(l->getEndpoint().y);
                        endpnt->at(3) = lcl::ldouble(l->getEndpoint().z);
                        entity->push_back(lcl::list(endpnt));

                        entity->push_back(lcl::list(extrDir));

                        return lcl::list(entity);

#endif
                    }
                    break;

                    default:
                        delete entity;
                        delete ename;
                        delete handle;
                        delete acdb;
                        delete mspace;
                        delete layoutTabName;
                        delete layer;
                        delete layer;
                        delete extrDir;

                        return lcl::nilValue();
                        break;
                }
                return lcl::nilValue();
            }
        }
    }

    return lcl::nilValue();
}

BUILTIN("entlast")
{
    CHECK_ARGS_IS(0);

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_GraphicView* graphicView = appWin->getGraphicView();
    RS_EntityContainer* entityContainer = graphicView->getContainer();

    if(entityContainer->count())
    {
        RS_Entity* entity = entityContainer->lastEntity();
        unsigned long int id = entity->getId();
        return lcl::ename(id);
    }

    return lcl::nilValue();
}

BUILTIN("entmake")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, seq);

    const int length = seq->count();
    lclValueVec* items = new lclValueVec(length);
    std::copy(seq->begin(), seq->end(), items->begin());
    double radius1 = 0.0;
    double radius2 = 0.0;
    double radius3 = 0.0;
    double angle1 = 0.0;
    double angle2 = 0.0;
    double scale1 = 1.0;
    double scale2 = 1.0;
    std::vector<std::vector<double>> gc_ten;
    std::vector<std::vector<double>> gc_eleven;
    String ename = "";
    String text = "";
    String style = "";
    String layer = "";

    for (int i = 0; i < length; i++) {
        if (items->at(i)->type() == LCLTYPE::LIST ||
                items->at(i)->type() == LCLTYPE::VEC) {
            lclSequence* list = VALUE_CAST(lclSequence, items->at(i));

            if (list->begin()->ptr()->print(true).compare("0") == 0
                && (list->item(1)->print(true).compare(".") == 0)
            ) {
                const lclString *n = VALUE_CAST(lclString, list->item(2));
                ename = n->value();
            }
            if (list->begin()->ptr()->print(true).compare("1") == 0
                && (list->item(1)->print(true).compare(".") == 0)
            ) {
                const lclString *t = VALUE_CAST(lclString, list->item(2));
                text = t->value();
            }
            if (list->begin()->ptr()->print(true).compare("7") == 0
                && (list->item(1)->print(true).compare(".") == 0)
            ) {
                const lclString *s = VALUE_CAST(lclString, list->item(2));
                style = s->value();
            }
            if (list->begin()->ptr()->print(true).compare("8") == 0
                && (list->item(1)->print(true).compare(".") == 0)
            ) {
                const lclString *l = VALUE_CAST(lclString, list->item(2));
                layer = l->value();
            }
            if (list->begin()->ptr()->print(true).compare("10") == 0
                && list->count() > 2)
            {
                if (list->count() == 3)
                {
                    const lclDouble *x = VALUE_CAST(lclDouble, list->item(1));
                    const lclDouble *y = VALUE_CAST(lclDouble, list->item(2));
                    gc_ten.push_back({ x->value(), y->value(), 0.0 });
                }
                else
                {
                    const lclDouble *x = VALUE_CAST(lclDouble, list->item(1));
                    const lclDouble *y = VALUE_CAST(lclDouble, list->item(2));
                    const lclDouble *z = VALUE_CAST(lclDouble, list->item(3));
                    gc_ten.push_back({ x->value(), y->value(), z->value()});
                }
            }
            if (list->begin()->ptr()->print(true).compare("11") == 0
                && list->count() > 2)
            {
                if (list->count() == 3)
                {
                    const lclDouble *x = VALUE_CAST(lclDouble, list->item(1));
                    const lclDouble *y = VALUE_CAST(lclDouble, list->item(2));
                    gc_eleven.push_back({ x->value(), y->value(), 0.0 });
                }
                else
                {
                    const lclDouble *x = VALUE_CAST(lclDouble, list->item(1));
                    const lclDouble *y = VALUE_CAST(lclDouble, list->item(2));
                    const lclDouble *z = VALUE_CAST(lclDouble, list->item(3));
                    gc_eleven.push_back({ x->value(), y->value(), z->value()});
                }
            }
            if (list->begin()->ptr()->print(true).compare("40") == 0
                && (list->item(1)->print(true).compare(".") == 0)
            ) {
                const lclDouble *r1 = VALUE_CAST(lclDouble, list->item(2));
                radius1 = r1->value();
            }
            if (list->begin()->ptr()->print(true).compare("41") == 0
                && (list->item(1)->print(true).compare(".") == 0)
            ) {
                const lclDouble *r2 = VALUE_CAST(lclDouble, list->item(2));
                radius2 = r2->value();
            }
            if (list->begin()->ptr()->print(true).compare("42") == 0
                && (list->item(1)->print(true).compare(".") == 0)
            ) {
                const lclDouble *r3 = VALUE_CAST(lclDouble, list->item(2));
                radius3 = r3->value();
            }
            if (list->begin()->ptr()->print(true).compare("44") == 0
                && (list->item(1)->print(true).compare(".") == 0)
            ) {
                const lclDouble *sc1 = VALUE_CAST(lclDouble, list->item(2));
                scale1 = sc1->value();
            }
            if (list->begin()->ptr()->print(true).compare("45") == 0
                && (list->item(1)->print(true).compare(".") == 0)
            ) {
                const lclDouble *sc2 = VALUE_CAST(lclDouble, list->item(2));
                scale2 = sc2->value();
            }
            if (list->begin()->ptr()->print(true).compare("50") == 0
                && (list->item(1)->print(true).compare(".") == 0)
            ) {
                const lclDouble *a1 = VALUE_CAST(lclDouble, list->item(2));
                angle1 = a1->value();
            }
            if (list->begin()->ptr()->print(true).compare("51") == 0
                && (list->item(1)->print(true).compare(".") == 0)
            ) {
                const lclDouble *a2 = VALUE_CAST(lclDouble, list->item(2));
                angle2 = a2->value();
            }
        }
    }

    if (ename.compare("") == 0)
    {
        return lcl::nilValue();
    }

    auto& appWin=QC_ApplicationWindow::getAppWindow();
    RS_Document* d = appWin->getDocument();

    if (d) {
        RS_Graphic* graphic = (RS_Graphic*)d;
        if (!graphic) {
            return lcl::nilValue();
        }

        if (ename.compare("LINE") == 0 &&
                std::distance(gc_ten.begin(), gc_ten.end()) &&
                std::distance(gc_eleven.begin(), gc_eleven.end()))
        {
            RS_Line *line;
            line = new RS_Line(graphic, RS_Vector(gc_ten.at(0).at(0),
                                                  gc_ten.at(0).at(1),
                                                  gc_ten.at(0).at(2)),
                                        RS_Vector(gc_eleven.at(0).at(0),
                                                  gc_eleven.at(0).at(1),
                                                  gc_eleven.at(0).at(2)));
            //line->setPen(RS_Pen(RS_Color(128, 128, 128), RS2::Width01, RS2::SolidLine));
            graphic->addEntity(line);

        }

        if (ename.compare("CIRCLE") == 0 &&
                std::distance(gc_ten.begin(), gc_ten.end()) &&
                radius1 != 0.0)
        {
            RS_Circle *circle = new RS_Circle(graphic, RS_CircleData(RS_Vector(gc_ten.at(0).at(0),
                                                                               gc_ten.at(0).at(1),
                                                                               gc_ten.at(0).at(2)),
                                                                     radius1));
            graphic->addEntity(circle);

        }

        if (ename.compare("ARC") == 0 &&
                std::distance(gc_ten.begin(), gc_ten.end()))
        {
            RS_Arc *arc = new RS_Arc(graphic, RS_ArcData(RS_Vector(gc_ten.at(0).at(0),
                                                                   gc_ten.at(0).at(1),
                                                                   gc_ten.at(0).at(2)),
                                                         radius1, angle1, angle2, false));
            graphic->addEntity(arc);
        }

        if (ename.compare("ELLIPSE") == 0 &&
                std::distance(gc_ten.begin(), gc_ten.end()) &&
                std::distance(gc_eleven.begin(), gc_eleven.end()))
        {

            RS_EllipseData data;
            data.center = RS_Vector(gc_ten.at(0).at(0),gc_ten.at(0).at(1),gc_ten.at(0).at(2));
            data.majorP = RS_Vector(gc_eleven.at(0).at(0),gc_eleven.at(0).at(1),gc_eleven.at(0).at(2));
            data.ratio = radius1;
            RS_Ellipse *ellipse = new RS_Ellipse(graphic, data);
            graphic->addEntity(ellipse);

        }

        RS_GraphicView* v = appWin->getGraphicView();
        if (v) {
            v->redraw();
        }

    }
    return lcl::nilValue();
}

BUILTIN("entmod")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, seq);
    unsigned int entityId = 0;
    bool found = false;

    for (int i = 0; i < seq->count(); i++)
    {
        const lclList *list = VALUE_CAST(lclList, seq->item(i));

        if (list->isDotted() && list->item(0)->print(true).compare("0") == 0)
        {
            const lclEname *ename = VALUE_CAST(lclEname, list->item(2));
            entityId = ename->value();
            found = true;
            break;
        }
    }

    if (!found)
    {
        return lcl::nilValue();
    }

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_GraphicView* graphicView = appWin->getGraphicView();
    RS_EntityContainer* entityContainer = graphicView->getContainer();


    if(entityContainer->count())
    {
        for (auto entity: *entityContainer)
        {
            if (entity->getId() == entityId)
            {
                RS_Pen pen = entity->getPen();

                for (int i = 0; i < seq->count(); i++)
                {
                    const lclList *list = VALUE_CAST(lclList, seq->item(i));

                    // lineType
                    if (list->isDotted() && list->item(0)->print(true).compare("6") == 0)
                    {
                        const lclString *ltype = VALUE_CAST(lclString, list->item(2));
                        pen.setLineType(RS_FilterDXFRW::nameToLineType(ltype->value().c_str()));
                        continue;
                    }
#if 0
                    // TextStyle
                    if (list->isDotted() && list->item(0)->print(true).compare("7") == 0)
                    {
                        const lclString *tstyle = VALUE_CAST(lclString, list->item(2));
                        continue;
                    }
#endif
                    // layer
                    if (list->isDotted() && list->item(0)->print(true).compare("8") == 0)
                    {
                        const lclString *layer = VALUE_CAST(lclString, list->item(2));
                        entity->setLayer(layer->value().c_str());
                        continue;
                    }

                    // lineWidth
                    if (list->isDotted() && list->item(0)->print(true).compare("48") == 0)
                    {
#if 0
                        const lclDouble *lwidth = VALUE_CAST(lclDouble, list->item(2));
                        pen.setWidth(lwidth->value());
#endif
                        continue;
                    }

                    // color
                    if (list->isDotted() && list->item(0)->print(true).compare("62") == 0)
                    {
                        const lclInteger *color = VALUE_CAST(lclInteger, list->item(2));
                        pen.setColor(RS_FilterDXFRW::numberToColor(color->value()));
                        continue;
                    }
#if 0
                    if (list->isDotted() && list->item(0)->print(true).compare("0") == 0)
                    {
                        continue;
                    }
#endif
                }
                break;
            }
        }
    }
    else
    {
        return lcl::nilValue();
    }

    return lcl::nilValue();
}

BUILTIN("entnext")
{
    int args = CHECK_ARGS_BETWEEN(0, 1);

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_GraphicView* graphicView = appWin->getGraphicView();
    RS_EntityContainer* entityContainer = graphicView->getContainer();

    if(entityContainer->count())
    {
        if (args == 0)
        {
            RS_Entity* entity = entityContainer->first();
            unsigned long int id = entity->getId();
            return lcl::ename(id);
        }
        else
        {
            ARG(lclEname, en);

            for (unsigned int i = 0; i < entityContainer->count(); i++)
            {
                if (entityContainer->entityAt(i)->getId() == en->value() &&
                    i+1 < entityContainer->count())
                {
                    return lcl::ename(entityContainer->entityAt(i+1)->getId());
                }
            }
        }
    }

    return lcl::nilValue();
}

BUILTIN("entsel")
{
    int args = CHECK_ARGS_BETWEEN(0, 1);
    QString prompt = "Select object:";

    if (args == 1 && !NIL_PTR)
    {
        ARG(lclString, str);
        prompt = str->value().c_str();
    }

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        return lcl::nilValue();
    }

    if (Lisp_CommandEdit != nullptr)
    {
        Lisp_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
        Lisp_CommandEdit->setFocus();
    }

    QC_ActionEntSel* a = new QC_ActionEntSel(*doc, *graphicView);
    if (a)
    {
        if (!(prompt.isEmpty()))
        {
            a->setMessage(prompt);
        }
        graphicView->killAllActions();
        graphicView->setCurrentAction(a);

        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!graphicView->getEventHandler()->hasAction())
                break;
        }

        if (a->isCompleted())
        {
            graphicView->killAllActions();

            if (Lisp_CommandEdit != nullptr)
            {
                if (Lisp_CommandEdit->dockName().compare("Lisp Ide") == 0)
                {
                    Lisp_CommandEdit->setPrompt("_$ ");
                }
                else
                {
                    Lisp_CommandEdit->setPrompt(QObject::tr("Command: "));
                }
            }

            if (a->wasCanceled())
            {
                return lcl::nilValue();
            }

            lclValueVec *ptn = new lclValueVec(3);
            ptn->at(0) = lcl::ldouble(a->getPoint().x);
            ptn->at(1) = lcl::ldouble(a->getPoint().y);
            ptn->at(2) = lcl::ldouble(a->getPoint().z);

            lclValueVec *res = new lclValueVec(2);
            res->at(0) = lcl::ename((unsigned long) a->getEntityId());
            res->at(1) = lcl::list(ptn);
            return lcl::list(res);
        }
    }

    graphicView->killAllActions();

    if (Lisp_CommandEdit != nullptr)
    {
        if (Lisp_CommandEdit->dockName().compare("Lisp Ide") == 0)
        {
            Lisp_CommandEdit->setPrompt("_$ ");
        }
        else
        {
            Lisp_CommandEdit->setPrompt(QObject::tr("Command: "));
        }
    }

    return lcl::nilValue();
}

BUILTIN("eval")
{
    CHECK_ARGS_IS(1);
    return EVAL(*argsBegin, NULL);
}

BUILTIN("exit")
{
    CHECK_ARGS_IS(0);

    throw -1;

    return lcl::nilValue();
}

BUILTIN("exp")
{
    BUILTIN_FUNCTION(exp);
}

BUILTIN("expt")
{
    CHECK_ARGS_IS(2);

    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        argsBegin++;
        if (FLOAT_PTR)
        {
            ADD_FLOAT_VAL(*rhs)
            return lcl::ldouble(pow(lhs->value(),
                                    rhs->value()));
        }
        else
        {
            ADD_INT_VAL(*rhs)
            return lcl::ldouble(pow(lhs->value(),
                                    double(rhs->value())));
        }
    }
    else
    {
        ADD_INT_VAL(*lhs)
        argsBegin++;
        if (FLOAT_PTR)
        {
            ADD_FLOAT_VAL(*rhs)
            return lcl::ldouble(pow(double(lhs->value()),
                                    rhs->value()));
        }
        else
        {
            ADD_INT_VAL(*rhs)
            auto result = static_cast<std::int64_t>(pow(double(lhs->value()),
                                    double(rhs->value())));
            return lcl::integer(result);
        }
    }
}

BUILTIN("fill_image")
{
    CHECK_ARGS_IS(5);
    AG_INT(x1);
    AG_INT(y1);
    AG_INT(width);
    AG_INT(height);
    AG_INT(color);

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
                img->image()->addRect(x1->value(), y1->value(), width->value(), height->value(), color->value());
                return lcl::integer(color->value());
            }
            break;
            case IMAGE_BUTTON:
            {
                const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                img->button()->addRect(x1->value(), y1->value(), width->value(), height->value(), color->value());
                return lcl::integer(color->value());
            }
            break;
            default:
                return lcl::nilValue();
            }
        }
    }
    return lcl::nilValue();
}

BUILTIN("first")
{
    CHECK_ARGS_IS(1);
    if (*argsBegin == lcl::nilValue()) {
        return lcl::nilValue();
    }
    ARG(lclSequence, seq);
    return seq->first();
}

BUILTIN("fix")
{
    CHECK_ARGS_IS(1);

    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        return lcl::integer(floor(lhs->value()));
    }
    else
    {
        ADD_INT_VAL(*lhs)
        return lcl::integer(lhs->value());
    }
}

BUILTIN("float")
{
    CHECK_ARGS_IS(1);

    if (FLOAT_PTR)
    {
        ADD_FLOAT_VAL(*lhs)
        return lcl::ldouble(lhs->value());
    }
    else
    {
        ADD_INT_VAL(*lhs)
        return lcl::ldouble(double(lhs->value()));
    }
}

BUILTIN("fn?")
{
    CHECK_ARGS_IS(1);
    lclValuePtr arg = *argsBegin++;

    // Lambdas are functions, unless they're macros.
    if (const lclLambda* lambda = DYNAMIC_CAST(lclLambda, arg)) {
        return lcl::boolean(!lambda->isMacro());
    }
    // Builtins are functions.
    return lcl::boolean(DYNAMIC_CAST(lclBuiltIn, arg));
}

BUILTIN("get")
{
    CHECK_ARGS_IS(2);
    if (*argsBegin == lcl::nilValue()) {
        return *argsBegin;
    }
    ARG(lclHash, hash);
    return hash->get(*argsBegin);
}

BUILTIN("get_attr")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, key);
    ARG(lclString, attr);
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (getDclAttributeId(attr->value())) {
                case ACTION:
                    return lcl::string(tile->value().action);
                case ALIGNMENT:
                    return lcl::string(std::to_string((int)tile->value().alignment));
                case ALLOW_ACCEPT:
                    return lcl::string(boolToString(tile->value().allow_accept));
                case ASPECT_RATIO:
                    return lcl::string(std::to_string(tile->value().aspect_ratio));
                case BIG_INCREMENT:
                    return lcl::string(std::to_string(tile->value().big_increment));
                case CHILDREN_ALIGNMENT:
                {
                    for (int i = 0; i < MAX_DCL_POS; i++)
                    {
                        if (tile->value().children_alignment == dclPosition[i].pos)
                        {
                            return lcl::string(dclPosition[i].name);
                        }
                    }
                    break;
                }
                case CHILDREN_FIXED_HEIGHT:
                    return lcl::string(boolToString(tile->value().children_fixed_height));
                case CHILDREN_FIXED_WIDTH:
                    return lcl::string(boolToString(tile->value().children_fixed_width));
                case COLOR:
                {
                    for (int i = 0; i < MAX_DCL_COLOR; i++)
                    {
                        if (tile->value().color == dclColor[i].color)
                        {
                            return lcl::string(dclColor[i].name);
                        }
                    }
                    return lcl::string(std::to_string((int)tile->value().color));
                    break;
                }
                case EDIT_LIMIT:
                    return lcl::string(std::to_string(tile->value().edit_limit));
                case EDIT_WIDTH:
                    return lcl::string(std::to_string(tile->value().edit_width));
                case FIXED_HEIGHT:
                    return lcl::string(boolToString(tile->value().fixed_height));
                case FIXED_WIDTH:
                    return lcl::string(boolToString(tile->value().fixed_width));
                case FIXED_WIDTH_FONT:
                    return lcl::string(boolToString(tile->value().fixed_width_font));
                case HEIGHT:
                    return lcl::string(std::to_string(tile->value().height));
                case INITIAL_FOCUS:
                    return lcl::string(tile->value().initial_focus);
                case IS_BOLD:
                    return lcl::string(boolToString(tile->value().is_bold));
                case IS_CANCEL:
                    return lcl::string(boolToString(tile->value().is_cancel));
                case IS_DEFAULT:
                    return lcl::string(boolToString(tile->value().is_default));
                case IS_ENABLED:
                    return lcl::string(boolToString(tile->value().is_enabled));
                case IS_TAB_STOP:
                    return lcl::string(boolToString(tile->value().is_tab_stop));
                case KEY:
                    return lcl::string(tile->value().key);
                case LABEL:
                    return lcl::string(tile->value().label);
                case LAYOUT:
                {
                    for (int i = 0; i < MAX_DCL_POS; i++)
                    {
                        if (tile->value().layout == dclPosition[i].pos)
                        {
                            return lcl::string(dclPosition[i].name);
                        }
                    }
                }
                    break;
                case LIST:
                    return lcl::string(tile->value().list);
                case MAX_VALUE:
                    return lcl::string(std::to_string(tile->value().max_value));
                case MIN_VALUE:
                    return lcl::string(std::to_string(tile->value().min_value));
                case MNEMONIC:
                    return lcl::string(tile->value().mnemonic);
                case MULTIPLE_SELECT:
                    return lcl::string(boolToString(tile->value().multiple_select));
                case PASSWORD_CHAR:
                    return lcl::string(tile->value().password_char);
                case SMALL_INCREMENT:
                    return lcl::string(std::to_string(tile->value().small_increment));
                case TABS:
                    return lcl::string(tile->value().tabs);
                case TAB_TRUNCATE:
                    return lcl::string(boolToString(tile->value().tab_truncate));
                case VALUE:
                    return lcl::string(tile->value().value);
                case WIDTH:
                    return lcl::string(std::to_string(tile->value().width));
                default:
                    return lcl::nilValue();
            }
        }
    }
    return lcl::nilValue();
}

BUILTIN("get_tile")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, key);
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
            case EDIT_BOX:
            {
                const lclEdit* e = static_cast<const lclEdit*>(tile);
                return lcl::string(e->edit()->text().toStdString());
            }
            break;
            case LIST_BOX:
            {
                const lclListBox* lb = static_cast<const lclListBox*>(tile);
                return lcl::string(std::to_string(lb->list()->currentRow()));
            }
            break;
            case BUTTON:
            {
                const lclButton* b = static_cast<const lclButton*>(tile);
                return lcl::string(b->button()->text().toStdString());
            }
            break;
            case RADIO_BUTTON:
            {
                const lclButton* rb = static_cast<const lclButton*>(tile);
                return lcl::string(rb->button()->text().toStdString());
            }
            break;
            case TEXT:
            {
                const lclLabel* l = static_cast<const lclLabel*>(tile);
                return lcl::string(l->label()->text().toStdString());
            }
            break;
            case POPUP_LIST:
            {
                const lclPopupList* pl = static_cast<const lclPopupList*>(tile);
                return lcl::string(std::to_string(pl->list()->currentIndex()));
            }
            break;
            default:
                return lcl::string("");
            }
        }
    }
    return lcl::string("");
}

BUILTIN("getcorner")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    bool second = false;
    QString prompt = QObject::tr("Enter a point: ");
    double x=0, y=0, z=0;

    if (args >= 1)
    {
        if (NIL_PTR)
        {
            argsBegin++;
        }

        if (argsBegin->ptr()->type() == LCLTYPE::STR)
        {
            ARG(lclString, msg);
            prompt = QObject::tr(msg->value().c_str());
        }

        if (argsBegin->ptr()->type() == LCLTYPE::LIST)
        {
            ARG(lclSequence, ptn);
            if (ptn->count() == 2)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT) &&
                    (ptn->item(2)->type() == LCLTYPE::REAL || ptn->item(2)->type() == LCLTYPE::INT))
                {
                    second = true;
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }
                }
            }

            if (ptn->count() == 3)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT) &&
                    (ptn->item(2)->type() == LCLTYPE::REAL || ptn->item(2)->type() == LCLTYPE::INT))
                {
                    second = true;
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }

                    if (ptn->item(2)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Z = VALUE_CAST(lclDouble, ptn->item(2));
                        z = Z->value();
                    }
                    if (ptn->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Z = VALUE_CAST(lclInteger, ptn->item(2));
                        z = double(Z->value());
                    }
                }
            }
        }
    }

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        return lcl::nilValue();
    }

    if (Lisp_CommandEdit != nullptr)
    {
        Lisp_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
        Lisp_CommandEdit->setFocus();
    }

    QC_ActionGetCorner* a = new QC_ActionGetCorner(*doc, *graphicView);
    if (a)
    {
        QPointF *point = new QPointF;
        QPointF *base = nullptr;
        bool status = false;

        if(second)
        {
            base = new QPointF(x, y);
        }

        if (!(prompt.isEmpty()))
        {
            a->setMessage(prompt);
        }
        graphicView->killAllActions();
        graphicView->setCurrentAction(a);

        a->setBasepoint(base);

        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!graphicView->getEventHandler()->hasAction())
                break;
        }

        if (a->isCompleted() && !a->wasCanceled())
        {
            a->getPoint(point);
            status = true;
        }
        //RLZ: delete QC_ActionGetPoint. Investigate how to kill only this action
        graphicView->killAllActions();

        if (Lisp_CommandEdit != nullptr)
        {
            if (Lisp_CommandEdit->dockName().compare("Lisp Ide") == 0)
            {
                Lisp_CommandEdit->setPrompt("_$ ");
            }
            else
            {
                Lisp_CommandEdit->setPrompt(QObject::tr("Command: "));
            }
        }

        if(status)
        {
            lclValueVec *ptn = new lclValueVec(3);
            ptn->at(0) = lcl::ldouble(point->x());
            ptn->at(1) = lcl::ldouble(point->y());
            ptn->at(2) = lcl::ldouble(z);
            delete point;
            if (base)
            {
                delete base;
            }
            return lcl::list(ptn);
        }
        else
        {
            delete point;
            if (base)
            {
                delete base;
            }
        }
    }

    return lcl::nilValue();
}

BUILTIN("getdist")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    QString prompt = QObject::tr("Enter second point: ");
    double x=0, y=0, z=0;
    Q_UNUSED(z)

    if (args >= 1)
    {
        if (NIL_PTR)
        {
            argsBegin++;
        }

        if (argsBegin->ptr()->type() == LCLTYPE::STR)
        {
            ARG(lclString, msg);
            prompt = QObject::tr(msg->value().c_str());
        }

        if (argsBegin->ptr()->type() == LCLTYPE::LIST)
        {
            ARG(lclSequence, ptn);
            if (ptn->count() == 2)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT) &&
                    (ptn->item(2)->type() == LCLTYPE::REAL || ptn->item(2)->type() == LCLTYPE::INT))
                {
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }
                }
            }

            if (ptn->count() == 3)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT) &&
                    (ptn->item(2)->type() == LCLTYPE::REAL || ptn->item(2)->type() == LCLTYPE::INT))
                {
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }

                    if (ptn->item(2)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Z = VALUE_CAST(lclDouble, ptn->item(2));
                        z = Z->value();
                    }
                    if (ptn->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Z = VALUE_CAST(lclInteger, ptn->item(2));
                        z = double(Z->value());
                    }
                }
            }
        }
    }

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        return lcl::nilValue();
    }

    if (Lisp_CommandEdit != nullptr)
    {
        Lisp_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
        Lisp_CommandEdit->setFocus();
    }

    QC_ActionGetPoint* a = new QC_ActionGetPoint(*doc, *graphicView);
    if (a)
    {
        QPointF *point = new QPointF;
        QPointF *base = new QPointF(x, y);
        bool status = false;

        if (!(prompt.isEmpty()))
        {
            a->setMessage(prompt);
        }
        graphicView->killAllActions();
        graphicView->setCurrentAction(a);

        a->setBasepoint(base);

        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!graphicView->getEventHandler()->hasAction())
                break;
        }

        if (a->isCompleted() && !a->wasCanceled())
        {
            a->getPoint(point);
            status = true;
        }
        //RLZ: delete QC_ActionGetPoint. Investigate how to kill only this action
        graphicView->killAllActions();

        if (Lisp_CommandEdit != nullptr)
        {
            if (Lisp_CommandEdit->dockName().compare("Lisp Ide") == 0)
            {
                Lisp_CommandEdit->setPrompt("_$ ");
            }
            else
            {
                Lisp_CommandEdit->setPrompt(QObject::tr("Command: "));
            }
        }

        if(status)
        {
            double dist = std::sqrt(std::pow(x - point->x(), 2)
                                  + std::pow(y - point->y(), 2));

            delete point;
            delete base;
            return lcl::ldouble(dist);
        }
        else
        {
            delete point;
            delete base;
        }
    }

    return lcl::nilValue();
}

BUILTIN("getenv")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, str);

    if (const char* env_p = std::getenv(str->value().c_str())) {
        String env = env_p;
        return lcl::string(env);
    }
    return lcl::nilValue();
}

BUILTIN("getfiled")
{
    CHECK_ARGS_IS(4);
    ARG(lclString, title);
    ARG(lclString, def);
    ARG(lclString, ext);
    ARG(lclInteger, flags);
    QString fileName;
    QString path = def->value().c_str();
    QString fileExt = "(*.";
    fileExt += ext->value().c_str();
    fileExt += ")";

    if (flags->value() & 1)
    {
        if (flags->value() & 4) {
            path += ".";
            path += ext->value().c_str();
        }

        QFileDialog saveFile;
        if (flags->value() & 32) {
            saveFile.setAcceptMode(QFileDialog::AcceptSave);
            saveFile.setOptions(QFileDialog::DontConfirmOverwrite);
        }
        fileName = saveFile.getSaveFileName(nullptr, title->value().c_str(), path, fileExt);
        //dialog.exec();
        if (fileName.size())
        {
            if (flags->value() & 4) {
                fileName += ".";
                fileName += ext->value().c_str();
            }
            return lcl::string(qUtf8Printable(fileName));
        }
    }

    if (flags->value() & 2)
    {
        if (!(flags->value() & 4)) {
            fileExt = "(*.dxf)";
        }
        if (!(flags->value() & 16)) {
            // pfad abschliesen
        }
        if (fileExt.size() == 0) {
            fileExt = "(*)";
        }
        fileName = QFileDialog::getOpenFileName(nullptr, title->value().c_str(), path, fileExt);
        if (fileName.size())
        {
            return lcl::string(qUtf8Printable(fileName));
        }
    }

    /*
     * not implemented yet
     *
     * 8 (bit 3) -- If this bit is set and bit 0 is not set, getfiled performs a library
     *  search for the file name entered. If it finds the file and its directory in the library search path,
     *  it strips the path and returns only the file name.
     *  (It does not strip the path name if it finds that a file of the same name is in a different directory.)
     *
     * 64  (bit 6) -- Do not transfer the remote file if the user specifies a URL.
     *
     * 128 (bit 7) -- Do not allow URLs at all.
     *
     */

    return lcl::nilValue();
}

BUILTIN("getint")
{
    int args = CHECK_ARGS_BETWEEN(0, 1);
    int x = 0;
    QString prompt = "Enter an integer: ";
    QString result;

    if (args == 1)
    {
        ARG(lclString, str);
        prompt = str->value().c_str();
    }

    if (Lisp_CommandEdit != nullptr)
    {
        while (1) {
            Lisp_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
            Lisp_CommandEdit->setFocus();
            Lisp_CommandEdit->doProcess(false);

            result = RS_Lsp_InputHandle::readLine(Lisp_CommandEdit);
            if (std::regex_match(qUtf8Printable(result), intRegex))
            {
                x = result.toInt();
                break;
            }
        }

        if (Lisp_CommandEdit->dockName().compare("Lisp Ide") == 0)
        {
            Lisp_CommandEdit->setPrompt("_$ ");
        }
        else
        {
            Lisp_CommandEdit->setPrompt(QObject::tr("Command: "));
        }
        return lcl::integer(x);
    }
    else
    {
        x = QInputDialog::getInt(nullptr,
                "LibreCAD",
                QObject::tr(qUtf8Printable(prompt)),
                // , int value = 0, int min = -2147483647, int max = 2147483647, int step = 1, bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
                0, -2147483647, 2147483647, 1, nullptr, Qt::WindowFlags());
        return lcl::integer(x);
    }
}

BUILTIN("getkword") {
    CHECK_ARGS_IS(1);
    ARG(lclString, msg);
    const lclInteger* bit = VALUE_CAST(lclInteger, shadowEnv->get("initget_bit"));
    const lclString* pat = VALUE_CAST(lclString, shadowEnv->get("initget_string"));

    std::vector<String> StringList;
    String del = " ";
    String result;
    String pattern = pat->value();

    auto pos = pattern.find(del);
    while (pos != String::npos) {
        StringList.push_back(pattern.substr(0, pos));
        pattern.erase(0, pos + del.length());
        pos = pattern.find(del);
    }
    StringList.push_back(pattern);

    if (Lisp_CommandEdit != nullptr)
    {
        while (1) {
            Lisp_CommandEdit->setPrompt(QObject::tr(msg->value().c_str()));
            Lisp_CommandEdit->setFocus();
            Lisp_CommandEdit->doProcess(false);
            result = RS_Lsp_InputHandle::readLine(Lisp_CommandEdit).toStdString();

            for (auto &it : StringList) {
                if (it == result) {
                    if (Lisp_CommandEdit->dockName().compare("Lisp Ide") == 0)
                    {
                        Lisp_CommandEdit->setPrompt("_$ ");
                    }
                    else
                    {
                        Lisp_CommandEdit->setPrompt(QObject::tr("Command: "));
                    }
                    return lcl::string(result);
                }
            }
            if ((bit->value() & 1) != 1) {
                if (Lisp_CommandEdit->dockName().compare("Lisp Ide") == 0)
                {
                    Lisp_CommandEdit->setPrompt("_$ ");
                }
                else
                {
                    Lisp_CommandEdit->setPrompt(QObject::tr("Command: "));
                }
                return lcl::nilValue();
            }
        }
    }
    else
    {
        while (1) {
            result = QInputDialog::getText(nullptr,
                        "LibreCAD",
                        QObject::tr(msg->value().c_str()),
                        QLineEdit::Normal, "", nullptr, Qt::WindowFlags(), Qt::ImhNone
                     ).toStdString();

            for (auto &it : StringList) {
                if (it == result) {
                    return lcl::string(result);
                }
            }
            if ((bit->value() & 1) != 1) {
                return lcl::nilValue();
            }
        }
    }

    return lcl::nilValue();
}

BUILTIN("getorient")
{
    /*
     * independent of the variables ANGDIR
     *
     * getangle to getorient
     * is only set as alias
     * nomaly depends of the variables ANGDIR
     * but is not implemented yet
     *
     */

    int args = CHECK_ARGS_BETWEEN(0, 2);
    QString prompt = QObject::tr("Enter second point: ");
    double x=0, y=0, z=0;
    Q_UNUSED(z)

    if (args >= 1)
    {
        if (NIL_PTR)
        {
            argsBegin++;
        }

        if (argsBegin->ptr()->type() == LCLTYPE::STR)
        {
            ARG(lclString, msg);
            prompt = QObject::tr(msg->value().c_str());
        }

        if (argsBegin->ptr()->type() == LCLTYPE::LIST)
        {
            ARG(lclSequence, ptn);
            if (ptn->count() == 2)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT) &&
                    (ptn->item(2)->type() == LCLTYPE::REAL || ptn->item(2)->type() == LCLTYPE::INT))
                {
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }
                }
            }

            if (ptn->count() == 3)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT) &&
                    (ptn->item(2)->type() == LCLTYPE::REAL || ptn->item(2)->type() == LCLTYPE::INT))
                {
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }

                    if (ptn->item(2)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Z = VALUE_CAST(lclDouble, ptn->item(2));
                        z = Z->value();
                    }
                    if (ptn->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Z = VALUE_CAST(lclInteger, ptn->item(2));
                        z = double(Z->value());
                    }
                }
            }
        }
    }

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        return lcl::nilValue();
    }

    if (Lisp_CommandEdit != nullptr)
    {
        Lisp_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
        Lisp_CommandEdit->setFocus();
    }

    QC_ActionGetPoint* a = new QC_ActionGetPoint(*doc, *graphicView);
    if (a)
    {
        QPointF *point = new QPointF;
        QPointF *base = new QPointF(x, y);
        bool status = false;

        if (!(prompt.isEmpty()))
        {
            a->setMessage(prompt);
        }
        graphicView->killAllActions();
        graphicView->setCurrentAction(a);

        a->setBasepoint(base);

        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!graphicView->getEventHandler()->hasAction())
                break;
        }

        if (a->isCompleted() && !a->wasCanceled())
        {
            a->getPoint(point);
            status = true;
        }
        //RLZ: delete QC_ActionGetPoint. Investigate how to kill only this action
        graphicView->killAllActions();

        if (Lisp_CommandEdit != nullptr)
        {
            if (Lisp_CommandEdit->dockName().compare("Lisp Ide") == 0)
            {
                Lisp_CommandEdit->setPrompt("_$ ");
            }
            else
            {
                Lisp_CommandEdit->setPrompt(QObject::tr("Command: "));
            }
        }

        if(status)
        {
            double rad = std::atan2(point->y() - y, point->x() - x);
            delete point;
            delete base;
            return lcl::ldouble(rad);
        }
        else
        {
            delete point;
            delete base;
        }
    }

    return lcl::nilValue();
}

BUILTIN("getpoint")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    bool second = false;
    QString prompt = QObject::tr("Enter a point: ");
    double x, y, z=0;

    if (args >= 1)
    {
        if (NIL_PTR)
        {
            argsBegin++;
        }

        if (argsBegin->ptr()->type() == LCLTYPE::STR)
        {
            ARG(lclString, msg);
            prompt = QObject::tr(msg->value().c_str());
        }

        if (argsBegin->ptr()->type() == LCLTYPE::LIST)
        {
            ARG(lclSequence, ptn);
            if (ptn->count() == 2)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT) &&
                    (ptn->item(2)->type() == LCLTYPE::REAL || ptn->item(2)->type() == LCLTYPE::INT))
                {
                    second = true;
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }
                }
            }

            if (ptn->count() == 3)
            {
                if ((ptn->item(0)->type() == LCLTYPE::REAL || ptn->item(0)->type() == LCLTYPE::INT) &&
                    (ptn->item(1)->type() == LCLTYPE::REAL || ptn->item(1)->type() == LCLTYPE::INT) &&
                    (ptn->item(2)->type() == LCLTYPE::REAL || ptn->item(2)->type() == LCLTYPE::INT))
                {
                    second = true;
                    if (ptn->item(0)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *X = VALUE_CAST(lclDouble, ptn->item(0));
                        x = X->value();
                    }
                    if (ptn->item(0)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *X = VALUE_CAST(lclInteger, ptn->item(0));
                        x = double(X->value());
                    }
                    if (ptn->item(1)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Y = VALUE_CAST(lclDouble, ptn->item(1));
                        y = Y->value();
                    }
                    if (ptn->item(1)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Y = VALUE_CAST(lclInteger, ptn->item(1));
                        y = double(Y->value());
                    }

                    if (ptn->item(2)->type() == LCLTYPE::REAL)
                    {
                        const lclDouble *Z = VALUE_CAST(lclDouble, ptn->item(2));
                        z = Z->value();
                    }
                    if (ptn->item(2)->type() == LCLTYPE::INT)
                    {
                        const lclInteger *Z = VALUE_CAST(lclInteger, ptn->item(2));
                        z = double(Z->value());
                    }
                }
            }
        }
    }

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        return lcl::nilValue();
    }

    if (Lisp_CommandEdit != nullptr)
    {
        Lisp_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
        Lisp_CommandEdit->setFocus();
    }

    QC_ActionGetPoint* a = new QC_ActionGetPoint(*doc, *graphicView);
    if (a)
    {
        QPointF *point = new QPointF;
        QPointF *base = nullptr;
        bool status = false;

        if (!(prompt.isEmpty()))
        {
            a->setMessage(prompt);
        }
        graphicView->killAllActions();
        graphicView->setCurrentAction(a);

        if (second)
        {
            base = new QPointF(x, y);
            a->setBasepoint(base);
        }

        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!graphicView->getEventHandler()->hasAction())
                break;
        }

        if (a->isCompleted() && !a->wasCanceled())
        {
            a->getPoint(point);
            status = true;
        }
        //RLZ: delete QC_ActionGetPoint. Investigate how to kill only this action
        graphicView->killAllActions();

        if (Lisp_CommandEdit != nullptr)
        {
            if (Lisp_CommandEdit->dockName().compare("Lisp Ide") == 0)
            {
                Lisp_CommandEdit->setPrompt("_$ ");
            }
            else
            {
                Lisp_CommandEdit->setPrompt(QObject::tr("Command: "));
            }
        }

        if(status)
        {
            lclValueVec *ptn = new lclValueVec(3);
            ptn->at(0) = lcl::ldouble(point->x());
            ptn->at(1) = lcl::ldouble(point->y());
            ptn->at(2) = lcl::ldouble(z);
            delete point;
            if (base)
            {
                delete base;
            }
            return lcl::list(ptn);
        }
        else
        {
            delete point;
            if (base)
            {
                delete base;
            }
        }
    }

    return lcl::nilValue();
}

BUILTIN("getreal")
{
    int args = CHECK_ARGS_BETWEEN(0, 1);
    double x = 0;
    QString prompt = "Enter a floating point number: ";
    QString result;

    if (args == 1)
    {
        ARG(lclString, str);
        prompt = str->value().c_str();
    }
    if (Lisp_CommandEdit != nullptr)
    {
        while (1) {
            Lisp_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
            Lisp_CommandEdit->setFocus();
            Lisp_CommandEdit->doProcess(false);

            result = RS_Lsp_InputHandle::readLine(Lisp_CommandEdit);
            if (std::regex_match(qUtf8Printable(result), floatRegex))
            {
                x = result.toDouble();
                break;
            }
        }

        if (Lisp_CommandEdit->dockName().compare("Lisp Ide") == 0)
        {
            Lisp_CommandEdit->setPrompt("_$ ");
        }
        else
        {
            Lisp_CommandEdit->setPrompt(QObject::tr("Command: "));
        }
        return lcl::ldouble(x);
    }
    else
    {
        x = QInputDialog::getDouble(nullptr,
                "LibreCAD",
                QObject::tr(qUtf8Printable(prompt)),
                // double value = 0, double min = -2147483647, double max = 2147483647, int decimals = 1, bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags(), double step = 1)
                0, -2147483647, 2147483647, 1, nullptr, Qt::WindowFlags(), 1
            );
        return lcl::ldouble(x);
    }
}

BUILTIN("getstring")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    QString s = "";
    QString prompt = "Enter a text: ";

    if (args == 2)
    {
        //FIXME T or nil or not exists imput with " " space
        argsBegin++;
    }
    if (args >= 1)
    {
        //FIXME T or nil or not exists imput with " " space
        ARG(lclString, str);
        prompt = str->value().c_str();
    }
    if (Lisp_CommandEdit != nullptr)
    {
        Lisp_CommandEdit->setPrompt(prompt);
        Lisp_CommandEdit->setFocus();
        Lisp_CommandEdit->doProcess(false);

        String result = RS_Lsp_InputHandle::readLine(Lisp_CommandEdit).toStdString();

        if (Lisp_CommandEdit->dockName().compare("Lisp Ide") == 0)
        {
            Lisp_CommandEdit->setPrompt("_$ ");
        }
        else
        {
            Lisp_CommandEdit->setPrompt(QObject::tr("Command: "));
        }
        return lcl::string(result);
    }
    else
    {
        s = QInputDialog::getText(nullptr,
                                  "LibreCAD",
                                  QObject::tr(qUtf8Printable(prompt)),
                                  //QLineEdit::EchoMode mode = QLineEdit::Normal, const QString &text = QString(), bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags(), Qt::InputMethodHints inputMethodHints = Qt::ImhNone)
                                  QLineEdit::Normal, "", nullptr, Qt::WindowFlags(), Qt::ImhNone);
        return lcl::string(s.toStdString());
    }

    return lcl::string(s.toStdString());
}

BUILTIN("getvar") {
    CHECK_ARGS_IS(1);
    lclSymbol* sym = VALUE_CAST(lclSymbol, *argsBegin);
    lclValuePtr value = shadowEnv->get(sym->value());
    if (value) {
        return value;
    }
    return lcl::nilValue();
}

BUILTIN("hash-map")
{
    Q_UNUSED(name);
    return lcl::hash(argsBegin, argsEnd, true);
}

BUILTIN("help")
{
    int args = CHECK_ARGS_BETWEEN(0, 1);

    QDir directory(QDir::currentPath());
    QString librebrowser = directory.absoluteFilePath("librebrowser");

    if(QFile::exists(librebrowser))
    {
        if (args == 1)
        {
            ARG(lclString, str);
            librebrowser += " '";
            librebrowser += str->value().c_str();
            librebrowser += "' &";
        }
        system(qUtf8Printable(librebrowser));
    }
    else
    {
        std::cout << "poor Help call 'SOS' :-b" << std::endl;
        return lcl::nilValue();
    }

    return lcl::nilValue();
}

BUILTIN("initget") {
    int args = CHECK_ARGS_BETWEEN(1, 2);
    if (argsBegin->ptr()->type() == LCLTYPE::INT && args == 2) {
        shadowEnv->set("initget_bit", EVAL(*argsBegin++, NULL));
        shadowEnv->set("initget_string", EVAL(*argsBegin, NULL));
    }
    else {
        //qDebug() << "initget EVAL" << EVAL(*argsBegin, NULL)->print(true).c_str();
        shadowEnv->set("initget_bit", lcl::integer(0));
        shadowEnv->set("initget_string", EVAL(*argsBegin, NULL));
    }
    return lcl::nilValue();
}

BUILTIN("keys")
{
    CHECK_ARGS_IS(1);
    ARG(lclHash, hash);
    return hash->keys();
}

BUILTIN("keyword")
{
    CHECK_ARGS_IS(1);
    const lclValuePtr arg = *argsBegin++;
    if (lclKeyword* s = DYNAMIC_CAST(lclKeyword, arg))
      return s;
    if (const lclString* s = DYNAMIC_CAST(lclString, arg))
      return lcl::keyword(":" + s->value());
    LCL_FAIL("keyword expects a keyword or string");
    return lcl::nilValue();
}

BUILTIN("last")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, seq);

    LCL_CHECK(0 < seq->count(), "Index out of range");
    return seq->item(seq->count()-1);
}

BUILTIN("license")
{
    CHECK_ARGS_IS(0);
    QFile f(":/gpl-2.0.txt");
    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        return lcl::nilValue();
    }
    QTextStream in(&f);
    std::cout << in.readAll().toStdString();

    return lcl::nilValue();
}

BUILTIN("list")
{
    Q_UNUSED(name);
    return lcl::list(argsBegin, argsEnd);
}

BUILTIN("listp")
{
    CHECK_ARGS_IS(1);
    return (DYNAMIC_CAST(lclList, *argsBegin)) ? lcl::trueValue() : lcl::nilValue();
}

BUILTIN("load_dialog") {
    CHECK_ARGS_IS(1);
    ARG(lclString, arg);
    String path = arg->value();
    const std::filesystem::path p(path.c_str());
    if (!p.has_extension()) {
        path += ".dcl";
    }
    if (!std::filesystem::exists(path.c_str())) {
        return lcl::integer(-1);
    }

    lclValuePtr dcl = loadDcl(path);
    const lclGui *container = VALUE_CAST(lclGui, dcl);

    if (dcl) {
        int uniq = container->value().dialog_Id;
        dclEnv->set(STRF("#builtin-gui(%d)", uniq), dcl);
        dclEnv->set("load_dialog_id", lcl::integer(uniq));
        return lcl::integer(uniq);
    }
    return lcl::integer(-1);
}

BUILTIN("log")
{
    BUILTIN_FUNCTION(log);
}

BUILTIN("logand")
{
    int argCount = CHECK_ARGS_AT_LEAST(0);
    int result = 0;
    [[maybe_unused]] double floatValue = 0;
    [[maybe_unused]] int64_t intValue = 0;

    if (argCount == 0) {
        return lcl::integer(0);
    }
    else {
        CHECK_IS_NUMBER(argsBegin->ptr());
        if (INT_PTR) {
            ADD_INT_VAL(*intVal);
            intValue = intVal->value();
            if (argCount == 1) {
                return lcl::integer(intValue);
            }
            else {
                result = intValue;
            }
        }
        else {
            ADD_FLOAT_VAL(*floatVal);
            floatValue = floatVal->value();
            if (argCount == 1) {
                return lcl::integer(int(floatValue));
            }
            else {
                result = int(floatValue);
            }
        }
    }
    for (auto it = argsBegin; it != argsEnd; it++) {
        CHECK_IS_NUMBER(it->ptr());
        if (it->ptr()->type() == LCLTYPE::INT) {
            const lclInteger* i = VALUE_CAST(lclInteger, *it);
            result = result & i->value();
        }
        else {
            const lclDouble* i = VALUE_CAST(lclDouble, *it);
            result = result & int(i->value());
        }
    }
    return lcl::integer(result);
}

BUILTIN("log10")
{
    CHECK_ARGS_IS(1);
    if (FLOAT_PTR) {
        ADD_FLOAT_VAL(*lhs)
        if (lhs->value() < 0) {
            return lcl::nilValue();
        }
        return lcl::ldouble(log10(lhs->value())); }
    else {
        ADD_INT_VAL(*lhs)
        if (lhs->value() < 0) {
            return lcl::nilValue();
        }
        return lcl::ldouble(log10(lhs->value())); }
}

BUILTIN("macro?")
{
    CHECK_ARGS_IS(1);

    // Macros are implemented as lambdas, with a special flag.
    const lclLambda* lambda = DYNAMIC_CAST(lclLambda, *argsBegin);
    return lcl::boolean((lambda != NULL) && lambda->isMacro());
}

BUILTIN("map")
{
    CHECK_ARGS_IS(2);
    lclValuePtr op = *argsBegin++; // this gets checked in APPLY
    ARG(lclSequence, source);

    const int length = source->count();
    lclValueVec* items = new lclValueVec(length);
    auto it = source->begin();
    for (int i = 0; i < length; i++) {
      items->at(i) = APPLY(op, it+i, it+i+1);
    }

    return  lcl::list(items);
}

BUILTIN("mapcar")
{
    int argCount = CHECK_ARGS_AT_LEAST(2);
    int i = 0;
    int count = 0;
    int offset = 0;
    int listCount = argCount-1;
    //int listCounts[listCount];
    std::vector<int> listCounts(static_cast<int>(listCount));
    const lclValuePtr op = EVAL(argsBegin++->ptr(), NULL);

    for (auto it = argsBegin++; it != argsEnd; it++) {
        const lclSequence* seq = VALUE_CAST(lclSequence, *it);
        listCounts[i++] = seq->count();
        offset += seq->count();
        if (count < seq->count()) {
            count = seq->count();
        }
    }

    //int newListCounts[count];
    //std::vector<int> newListCounts(static_cast<int>(count));
    std::vector<int> newListCounts(count);
    //lclValueVec* valItems[count]; // FIXME [-Wvla-cxx-extension]
    std::vector<lclValueVec *> valItems(count);
    lclValueVec* items = new lclValueVec(offset);
    lclValueVec* result = new lclValueVec(count);

    offset = 0;
    for (auto it = --argsBegin; it != argsEnd; ++it) {
        const lclSequence* seq = STATIC_CAST(lclSequence, *it);
        std::copy(seq->begin(), seq->end(), items->begin() + offset);
        offset += seq->count();
    }

    for (auto l = 0; l < count; l++) {
        newListCounts[l] = 0;
        valItems[l] = { new lclValueVec(listCount+1) };
        valItems[l]->at(0) = op;
    }

    offset = 0;
    for (auto n = 0; n < listCount; n++) {
        for (auto l = 0; l < count; l++) {
            if (listCounts[n] > l) {
                valItems[l]->at(n + 1) = items->at(offset + l);
                newListCounts[l] += 1;
            }
        }
        offset += listCounts[n];
    }

    for (auto l = 0; l < count; l++) {
        for (auto v = listCount - newListCounts[l]; v > 0; v--) {
            valItems[l]->erase(std::next(valItems[l]->begin()));
        }
        lclList* List = new lclList(valItems[l]);
        result->at(l) = EVAL(List, NULL);
    }
    return lcl::list(result);
}


BUILTIN("max")
{
    int count = CHECK_ARGS_AT_LEAST(1);
    bool hasFloat = ARGS_HAS_FLOAT;
    bool unset = true;
    [[maybe_unused]] double floatValue = 0;
    [[maybe_unused]] int64_t intValue = 0;

    if (count == 1)
    {
        if (hasFloat) {
            ADD_FLOAT_VAL(*floatVal);
            floatValue = floatVal->value();
            return lcl::ldouble(floatValue);
        }
        else {
            ADD_INT_VAL(*intVal);
            intValue = intVal->value();
            return lcl::integer(intValue);
        }
    }

    if (hasFloat) {
        do {
            if (FLOAT_PTR) {
                if (unset) {
                    unset = false;
                    ADD_FLOAT_VAL(*floatVal);
                    floatValue = floatVal->value();
                }
                else {
                    ADD_FLOAT_VAL(*floatVal)
                    if (floatVal->value() > floatValue) {
                        floatValue = floatVal->value();
                    }
                }
            }
            else {
                if (unset) {
                    unset = false;
                    ADD_INT_VAL(*intVal);
                    floatValue = double(intVal->value());
                }
                else {
                    ADD_INT_VAL(*intVal);
                    if (intVal->value() > floatValue)
                    {
                        floatValue = double(intVal->value());
                    }
                }
            }
            argsBegin++;
        } while (argsBegin != argsEnd);
        return lcl::ldouble(floatValue);
    } else {
        ADD_INT_VAL(*intVal);
        intValue = intVal->value();
        argsBegin++;
        do {
            ADD_INT_VAL(*intVal);
            if (intVal->value() > intValue)
            {
                intValue = intVal->value();
            }
            argsBegin++;
        } while (argsBegin != argsEnd);
        return lcl::integer(intValue);
    }
}

BUILTIN("member?")
{
    CHECK_ARGS_IS(2);
    lclValuePtr op = *argsBegin++;
    ARG(lclSequence, seq);

    const int length = seq->count();
    lclValueVec* items = new lclValueVec(length);
    std::copy(seq->begin(), seq->end(), items->begin());

    for (int i = 0; i < length; i++) {
        if (items->at(i)->print(true).compare(op->print(true)) == 0) {
            return lcl::trueValue();
        }
    }
    return lcl::nilValue();
}

BUILTIN("meta")
{
    CHECK_ARGS_IS(1);
    lclValuePtr obj = *argsBegin++;

    return obj->meta();
}

BUILTIN("min")
{
    int count = CHECK_ARGS_AT_LEAST(1);
    bool hasFloat = ARGS_HAS_FLOAT;
    bool unset = true;
    [[maybe_unused]] double floatValue = 0;
    [[maybe_unused]] int64_t intValue = 0;

    if (count == 1)
    {
        if (hasFloat) {
            ADD_FLOAT_VAL(*floatVal);
            floatValue = floatVal->value();
            return lcl::ldouble(floatValue);
        }
        else {
            ADD_INT_VAL(*intVal);
            intValue = intVal->value();
            return lcl::integer(intValue);
        }
    }

    if (hasFloat) {
        do {
            if (FLOAT_PTR) {
                if (unset) {
                    unset = false;
                    ADD_FLOAT_VAL(*floatVal);
                    floatValue = floatVal->value();
                }
                else {
                    ADD_FLOAT_VAL(*floatVal)
                    if (floatVal->value() < floatValue) {
                        floatValue = floatVal->value();
                    }
                }
            }
            else {
                if (unset) {
                    unset = false;
                    ADD_INT_VAL(*intVal);
                    floatValue = double(intVal->value());
                }
                else {
                    ADD_INT_VAL(*intVal);
                    if (intVal->value() < floatValue) {
                        floatValue = double(intVal->value());
                    }
                }
            }
            argsBegin++;
        } while (argsBegin != argsEnd);
        return lcl::ldouble(floatValue);
    } else {
        ADD_INT_VAL(*intVal);
        intValue = intVal->value();
        argsBegin++;
        do {
            ADD_INT_VAL(*intVal);
            if (intVal->value() < intValue) {
                intValue = intVal->value();
            }
            argsBegin++;
        } while (argsBegin != argsEnd);
        return lcl::integer(intValue);
    }
}

BUILTIN("minus?")
{
    CHECK_ARGS_IS(1);
    if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::REAL)
    {
        lclDouble* val = VALUE_CAST(lclDouble, EVAL(*argsBegin, NULL));
        return lcl::boolean(val->value() < 0.0);

    }
    else if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::INT)
    {
        lclInteger* val = VALUE_CAST(lclInteger, EVAL(*argsBegin, NULL));
        return lcl::boolean(val->value() < 0);
    }
    return lcl::falseValue();
}

BUILTIN("minusp")
{
    CHECK_ARGS_IS(1);
    if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::REAL)
    {
        lclDouble* val = VALUE_CAST(lclDouble, EVAL(*argsBegin, NULL));
        return val->value() < 0 ? lcl::trueValue() : lcl::nilValue();
    }
    else if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::INT)
    {
        lclInteger* val = VALUE_CAST(lclInteger, EVAL(*argsBegin, NULL));
        return val->value() < 0 ? lcl::trueValue() : lcl::nilValue();
    }
    return lcl::nilValue();
}

BUILTIN("mode_tile")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, key);
    AG_INT(val);
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
            case EDIT_BOX:
            {
                qDebug() << "mode_tile EDIT_BOX";
                const lclEdit* e = static_cast<const lclEdit*>(tile);
                switch (val->value())
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
                    return lcl::nilValue();
                }
            }
            break;
            case LIST_BOX:
            {
                qDebug() << "mode_tile LIST_BOX";
                const lclListBox* e = static_cast<const lclListBox*>(tile);
                switch (val->value())
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
                    return lcl::nilValue();
                }
            }
            break;
            case BUTTON:
            {
                qDebug() << "mode_tile BUTTON";
                const lclButton* b = static_cast<const lclButton*>(tile);
                switch (val->value())
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
                    return lcl::nilValue();
                }
            }
            break;
            case RADIO_BUTTON:
            {
                qDebug() << "mode_tile RADIO_BUTTON";
                const lclButton* rb = static_cast<const lclButton*>(tile);
                switch (val->value())
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
                    return lcl::nilValue();
                }
            }
            break;
            case TEXT:
            {
                qDebug() << "mode_tile TEXT";
                const lclLabel* l = static_cast<const lclLabel*>(tile);
                switch (val->value())
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
                    return lcl::nilValue();
                }
            }
            break;
            case POPUP_LIST:
            {
                qDebug() << "mode_tile POPUP_LIST";
                const lclPopupList* pl = static_cast<const lclPopupList*>(tile);
                switch (val->value())
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
                    return lcl::nilValue();
                }
            }
            break;
            default:
                return lcl::nilValue();
            }
            return lcl::trueValue();
        }
    }
    return lcl::nilValue();
}

BUILTIN("nth")
{
    // twisted parameter for both LISPs!
    CHECK_ARGS_IS(2);
    int i;

    if(INT_PTR)
    {
        AG_INT(index);
        ARG(lclSequence, seq);
        i = index->value();
        LCL_CHECK(i >= 0 && i < seq->count(), "Index out of range");
        return seq->item(i);
    }
    else if(FLOAT_PTR) {
        // add dummy for error msg
        AG_INT(index);
        [[maybe_unused]] const String dummy = index->print(true);
        return lcl::nilValue();
    }
    else {
        ARG(lclSequence, seq);
        AG_INT(index);
        i = index->value();
        LCL_CHECK(i >= 0 && i < seq->count(), "Index out of range");
        return seq->item(i);
    }
}

BUILTIN("null")
{
    CHECK_ARGS_IS(1);
    if (NIL_PTR) {
        return lcl::trueValue();
    }
    return lcl::nilValue();
}

BUILTIN("number?")
{
    CHECK_ARGS_IS(1);
    return lcl::boolean(DYNAMIC_CAST(lclInteger, *argsBegin) ||
            DYNAMIC_CAST(lclDouble, *argsBegin));
}

BUILTIN("numberp")
{
    CHECK_ARGS_IS(1);
    return (DYNAMIC_CAST(lclInteger, *argsBegin) ||
            DYNAMIC_CAST(lclDouble, *argsBegin)) ? lcl::trueValue() : lcl::nilValue();
}

BUILTIN("open")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, filename);
    ARG(lclString, m);
    const char mode = std::tolower(m->value().c_str()[0]);
    lclFile* pf = new lclFile(filename->value().c_str(), mode);

    return pf->open();
}

BUILTIN("pix_image")
{
    CHECK_ARGS_IS(5);
    AG_INT(x1);
    AG_INT(y1);
    AG_INT(width);
    AG_INT(height);
    ARG(lclString, filename);

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
                img->image()->addPicture(x1->value(), y1->value(), width->value(), height->value(), tile->value().aspect_ratio, filename->value().c_str());
                return lcl::string(filename->value());
            }
            break;
            case IMAGE_BUTTON:
            {
                const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                img->button()->addPicture(x1->value(), y1->value(), width->value(), height->value(), tile->value().aspect_ratio, filename->value().c_str());
                return lcl::string(filename->value());
            }
            break;
            default:
                return lcl::nilValue();
            }
        }
    }
    return lcl::nilValue();
}

BUILTIN("polar")
{
    CHECK_ARGS_IS(3);
    double angle = 0;
    double dist = 0;
    double x = 0;
    double y = 0;
    [[maybe_unused]] double z = 0;

    ARG(lclSequence, seq);

    if (FLOAT_PTR) {
        ADD_FLOAT_VAL(*floatAngle)
        angle = floatAngle->value();
    }
    else
    {
        ADD_INT_VAL(*intAngle)
        angle = double(intAngle->value());
    }
    if (FLOAT_PTR) {
        ADD_FLOAT_VAL(*floatDist)
        dist = floatDist->value();
    }
    else
    {
        ADD_INT_VAL(*intDist)
        dist = double(intDist->value());
    }

    if(seq->count() == 2)
    {
        CHECK_IS_NUMBER(seq->item(0))
        if (seq->item(0)->type() == LCLTYPE::INT)
        {
            const lclInteger* intX = VALUE_CAST(lclInteger, seq->item(0));
            x = double(intX->value());
        }
        if (seq->item(0)->type() == LCLTYPE::REAL)
        {
            const lclDouble* floatX = VALUE_CAST(lclDouble, seq->item(0));
            x = floatX->value();
        }
        CHECK_IS_NUMBER(seq->item(1))
        if (seq->item(1)->type() == LCLTYPE::INT)
        {
            const lclInteger* intY = VALUE_CAST(lclInteger, seq->item(1));
            y = double(intY->value());
        }
        if (seq->item(1)->type() == LCLTYPE::REAL)
        {
            const lclDouble* floatY = VALUE_CAST(lclDouble, seq->item(1));
            y = floatY->value();
        }

        lclValueVec* items = new lclValueVec(2);
        items->at(0) = lcl::ldouble(x + dist * sin(angle));
        items->at(1) = lcl::ldouble(y + dist * cos(angle));
        return lcl::list(items);
    }

    if(seq->count() == 3)
    {
        if (seq->item(0)->type() == LCLTYPE::INT)
        {
            const lclInteger* intX = VALUE_CAST(lclInteger, seq->item(0));
            x = double(intX->value());
        }
        if (seq->item(0)->type() == LCLTYPE::REAL)
        {
            const lclDouble* floatX = VALUE_CAST(lclDouble, seq->item(0));
            x = floatX->value();
        }
        CHECK_IS_NUMBER(seq->item(1))
        if (seq->item(1)->type() == LCLTYPE::INT)
        {
            const lclInteger* intY = VALUE_CAST(lclInteger, seq->item(1));
            y = double(intY->value());
        }
        if (seq->item(1)->type() == LCLTYPE::REAL)
        {
            const lclDouble* floatY = VALUE_CAST(lclDouble, seq->item(1));
            y = floatY->value();
        }
        CHECK_IS_NUMBER(seq->item(2))
        if (seq->item(2)->type() == LCLTYPE::INT)
        {
            const lclInteger* intY = VALUE_CAST(lclInteger, seq->item(2));
            z = double(intY->value());
        }
        if (seq->item(2)->type() == LCLTYPE::REAL)
        {
            const lclDouble* floatY = VALUE_CAST(lclDouble, seq->item(2));
            z = floatY->value();
        }
        lclValueVec* items = new lclValueVec(3);
        items->at(0) = lcl::ldouble(x + dist * sin(angle));
        items->at(1) = lcl::ldouble(y + dist * cos(angle));
        items->at(2) = lcl::ldouble(z);
        return lcl::list(items);
    }
    return lcl::nilValue();
}

BUILTIN("pr-str")
{
    Q_UNUSED(name);
    return lcl::string(printValues(argsBegin, argsEnd, " ", true));
}

BUILTIN("prin1")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    if (args == 0) {
        std::cout << std::endl;
        return lcl::nullValue();
    }
    lclFile* pf = NULL;
    LCLTYPE type = argsBegin->ptr()->type();
    String boolean = argsBegin->ptr()->print(true);
    lclValueIter value = argsBegin;

    if (args == 2) {
        argsBegin++;
        if (argsBegin->ptr()->print(true).compare("nil") != 0) {
            pf = VALUE_CAST(lclFile, *argsBegin);
        }
    }
    if (boolean == "nil") {
        if (pf) {
            pf->writeLine("\"nil\"");
        }
        else {
            std::cout << "\"nil\"";
        }
            return lcl::nilValue();
    }
    if (boolean == "false") {
        if (pf) {
            pf->writeLine("\"false\"");
        }
        else {
            std::cout << "\"false\"";
        }
            return lcl::falseValue();
    }
    if (boolean == "true") {
        if (pf) {
            pf->writeLine("\"true\"");
        }
        else {
            std::cout << "\"true\"";
        }
            return lcl::trueValue();
    }
    if (boolean == "T") {
        if (pf) {
            pf->writeLine("\"T\"");
        }
        else {
            std::cout << "\"T\"";
        }
            return lcl::trueValue();
    }

    switch(type) {
        case LCLTYPE::FILE: {
            lclFile* f = VALUE_CAST(lclFile, *value);
            char filePtr[32];
            sprintf(filePtr, "%p", f->value());
            const String file = filePtr;
            if (pf) {
                pf->writeLine("\"" + file + "\"");
            }
            else {
                std::cout << "\"" << file << "\"";
            }
            return f;
        }
        case LCLTYPE::INT: {
            lclInteger* i = VALUE_CAST(lclInteger, *value);
            if (pf) {
                pf->writeLine("\"" + i->print(true) + "\"");
            }
            else {
                std::cout << "\"" << i->print(true) << "\"";
            }
            return i;
        }
        case LCLTYPE::LIST: {
            lclList* list = VALUE_CAST(lclList, *value);
            if (pf) {
                pf->writeLine("\"" + list->print(true) + "\"");
            }
            else {
                std::cout << "\"" << list->print(true) << "\"";
            }
            return list;
        }
        case LCLTYPE::MAP: {
            lclHash* hash = VALUE_CAST(lclHash, *value);
            if (pf) {
                pf->writeLine("\"" + hash->print(true) + "\"");
            }
            else {
                std::cout << "\"" << hash->print(true) << "\"";
            }
            return hash;
         }
        case LCLTYPE::REAL: {
            lclDouble* d = VALUE_CAST(lclDouble, *value);
            if (pf) {
                pf->writeLine("\"" + d->print(true) + "\"");
            }
            else {
                std::cout << "\"" << d->print(true) << "\"";
            }
            return d;
        }
        case LCLTYPE::STR: {
            lclString* str = VALUE_CAST(lclString, *value);
            if (pf) {
                pf->writeLine("\"" + str->value() + "\"");
            }
            else {
                std::cout << "\"" << str->value() << "\"";
            }
            return str;
        }
        case LCLTYPE::SYM: {
            lclSymbol* sym = VALUE_CAST(lclSymbol, *value);
            if (pf) {
                pf->writeLine("\"" + sym->value() + "\"");
            }
            else {
                std::cout << "\"" << sym->value() << "\"";
            }
            return sym;
        }
        case LCLTYPE::VEC: {
            lclVector* vector = VALUE_CAST(lclVector, *value);
            if (pf) {
                pf->writeLine("\"" + vector->print(true) + "\"");
            }
            else {
                std::cout << "\"" << vector->print(true) << "\"";
            }
            return vector;
        }
        case LCLTYPE::KEYW: {
            lclKeyword* keyword = VALUE_CAST(lclKeyword, *value);
            if (pf) {
                pf->writeLine("\"" + keyword->print(true) + "\"");
            }
            else {
                std::cout << "\"" << keyword->print(true) << "\"";
            }
            return keyword;
        }
        default: {
            if (pf) {
                pf->writeLine("\"nil\"");
            }
            else {
                std::cout << "\"nil\"";
            }
            return lcl::nilValue();
        }
    }

    if (pf) {
        pf->writeLine("\"nil\"");
    }
    else {
        std::cout << "\"nil\"";
    }
        return lcl::nilValue();
}

BUILTIN("princ")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    if (args == 0) {
        std::cout << std::endl;
        fflush(stdout);
        return lcl::nullValue();
    }
    lclFile* pf = NULL;
    LCLTYPE type = argsBegin->ptr()->type();
    String boolean = argsBegin->ptr()->print(true);
    lclValueIter value = argsBegin;

    if (args == 2) {
        argsBegin++;
        if (argsBegin->ptr()->print(true).compare("nil") != 0) {
            pf = VALUE_CAST(lclFile, *argsBegin);
        }
    }
    if (boolean == "nil") {
        if (pf) {
            pf->writeLine("nil");
        }
        else {
            std::cout << "nil";
        }
            return lcl::nilValue();
    }
    if (boolean == "false") {
        if (pf) {
            pf->writeLine("false");
        }
        else {
            std::cout << "false";
        }
            return lcl::falseValue();
    }
    if (boolean == "true") {
        if (pf) {
            pf->writeLine("true");
        }
        else {
            std::cout << "true";
        }
            return lcl::trueValue();
    }
    if (boolean == "T") {
        if (pf) {
            pf->writeLine("T");
        }
        else {
            std::cout << "T";
        }
            return lcl::trueValue();
    }

    switch(type) {
        case LCLTYPE::FILE: {
            lclFile* f = VALUE_CAST(lclFile, *value);
            char filePtr[32];
            sprintf(filePtr, "%p", f->value());
            const String file = filePtr;
            if (pf) {
                pf->writeLine(file);
            }
            else {
                std::cout << file;
            }
            return f;
        }
        case LCLTYPE::INT: {
            lclInteger* i = VALUE_CAST(lclInteger, *value);
            if (pf) {
                pf->writeLine(i->print(true));
            }
            else {
                std::cout << i->print(true);
            }
            return i;
        }
        case LCLTYPE::LIST: {
            lclList* list = VALUE_CAST(lclList, *value);
            if (pf) {
                pf->writeLine(list->print(true));
            }
            else {
                std::cout << list->print(true);
            }
            return list;
        }
        case LCLTYPE::MAP: {
            lclHash* hash = VALUE_CAST(lclHash, *value);
            if (pf) {
                pf->writeLine(hash->print(true));
            }
            else {
                std::cout << hash->print(true);
            }
            return hash;
         }
        case LCLTYPE::REAL: {
            lclDouble* d = VALUE_CAST(lclDouble, *value);
            if (pf) {
                pf->writeLine(d->print(true));
            }
            else {
                std::cout << d->print(true);
            }
            return d;
        }
        case LCLTYPE::STR: {
            lclString* str = VALUE_CAST(lclString, *value);
            if (pf) {
                pf->writeLine(str->value());
            }
            else {
                std::cout << str->value();
                fflush(stdout);
            }
            return str;
        }
        case LCLTYPE::SYM: {
            lclSymbol* sym = VALUE_CAST(lclSymbol, *value);
            if (pf) {
                pf->writeLine(sym->value());
            }
            else {
                std::cout << sym->value();
            }
            return sym;
        }
        case LCLTYPE::VEC: {
            lclVector* vector = VALUE_CAST(lclVector, *value);
            if (pf) {
                pf->writeLine(vector->print(true));
            }
            else {
                std::cout << vector->print(true);
            }
            return vector;
        }
        case LCLTYPE::KEYW: {
            lclKeyword* keyword = VALUE_CAST(lclKeyword, *value);
            if (pf) {
                pf->writeLine(keyword->print(true));
            }
            else {
                std::cout << keyword->print(true);
            }
            return keyword;
        }
        default: {
            if (pf) {
                pf->writeLine("nil");
            }
            else {
                std::cout << "nil";
            }
            return lcl::nilValue();
        }
        fflush(stdout);
    }

    if (pf) {
        pf->writeLine("nil");
    }
    else {
        std::cout << "nil";
    }
        return lcl::nilValue();
}

BUILTIN("print")
{
    int args = CHECK_ARGS_BETWEEN(0, 2);
    if (args == 0) {
        std::cout << std::endl;
        return lcl::nullValue();
    }
    lclFile* pf = NULL;
    LCLTYPE type = argsBegin->ptr()->type();
    String boolean = argsBegin->ptr()->print(true);
    lclValueIter value = argsBegin;

    if (args == 2) {
        argsBegin++;
        if (argsBegin->ptr()->print(true).compare("nil") != 0) {
            pf = VALUE_CAST(lclFile, *argsBegin);
        }
    }
    if (boolean == "nil") {
        if (pf) {
            pf->writeLine("\n\"nil\" ");
        }
        else {
            std::cout << "\n\"nil\" ";
        }
            return lcl::nilValue();
    }
    if (boolean == "false") {
        if (pf) {
            pf->writeLine("\n\"false\" ");
        }
        else {
            std::cout << "\n\"false\" ";
        }
            return lcl::falseValue();
    }
    if (boolean == "true") {
        if (pf) {
            pf->writeLine("\n\"true\" ");
        }
        else {
            std::cout << "\n\"true\" ";
        }
            return lcl::trueValue();
    }
    if (boolean == "T") {
        if (pf) {
            pf->writeLine("\n\"T\" ");
        }
        else {
            std::cout << "\n\"T\" ";
        }
            return lcl::trueValue();
    }

    switch(type) {
        case LCLTYPE::FILE: {
            lclFile* f = VALUE_CAST(lclFile, *value);
            char filePtr[32];
            sprintf(filePtr, "%p", f->value());
            const String file = filePtr;
            if (pf) {
                pf->writeLine("\n\"" + file + "\" ");
            }
            else {
                std::cout << "\n\"" << file << "\" ";
            }
            return f;
        }
        case LCLTYPE::INT: {
            lclInteger* i = VALUE_CAST(lclInteger, *value);
            if (pf) {
                pf->writeLine("\n\"" + i->print(true) + "\" ");
            }
            else {
                std::cout << "\n\"" << i->print(true) << "\" ";
            }
            return i;
        }
        case LCLTYPE::LIST: {
            lclList* list = VALUE_CAST(lclList, *value);
            if (pf) {
                pf->writeLine("\n\"" + list->print(true) + "\" ");
            }
            else {
                std::cout << "\n\"" << list->print(true) << "\" ";
            }
            return list;
        }
        case LCLTYPE::MAP: {
            lclHash* hash = VALUE_CAST(lclHash, *value);
            if (pf) {
                pf->writeLine("\n\"" + hash->print(true) + "\" ");
            }
            else {
                std::cout << "\n\"" << hash->print(true) << "\" ";
            }
            return hash;
         }
        case LCLTYPE::REAL: {
            lclDouble* d = VALUE_CAST(lclDouble, *value);
            if (pf) {
                pf->writeLine("\n\"" + d->print(true) + "\" ");
            }
            else {
                std::cout << "\n\"" << d->print(true) << "\" ";
            }
            return d;
        }
        case LCLTYPE::STR: {
            lclString* str = VALUE_CAST(lclString, *value);
            if (pf) {
                pf->writeLine("\n\"" + str->value() + "\" ");
            }
            else {
                std::cout << "\n\"" << str->value() << "\" ";
            }
            return str;
        }
        case LCLTYPE::SYM: {
            lclSymbol* sym = VALUE_CAST(lclSymbol, *value);
            if (pf) {
                pf->writeLine("\n\"" + sym->value() + "\" ");
            }
            else {
                std::cout << "\n\"" << sym->value() << "\" ";
            }
            return sym;
        }
        case LCLTYPE::VEC: {
            lclVector* vector = VALUE_CAST(lclVector, *value);
            if (pf) {
                pf->writeLine("\n\"" + vector->print(true) + "\" ");
            }
            else {
                std::cout << "\n\"" << vector->print(true) << "\" ";
            }
            return vector;
        }
        case LCLTYPE::KEYW: {
            lclKeyword* keyword = VALUE_CAST(lclKeyword, *value);
            if (pf) {
                pf->writeLine("\n\"" + keyword->print(true) + "\" ");
            }
            else {
                std::cout << "\n\"" << keyword->print(true) << "\" ";
            }
            return keyword;
        }
        default: {
            if (pf) {
                pf->writeLine("\n\"nil\" ");
            }
            else {
                std::cout << "\n\"nil\" ";
            }
            return lcl::nilValue();
        }
    }

    if (pf) {
        pf->writeLine("\n\"nil\" ");
    }
    else {
        std::cout << "\n\"nil\" ";
    }
        return lcl::nilValue();
}

BUILTIN("println")
{
    Q_UNUSED(name);
    std::cout << printValues(argsBegin, argsEnd, " ", false) << std::endl;
    return lcl::nilValue();
}

BUILTIN("prn")
{
    Q_UNUSED(name);
    std::cout << printValues(argsBegin, argsEnd, " ", true) << std::endl;
    return lcl::nilValue();
}

BUILTIN("prompt")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, str);

    if (Lisp_CommandEdit != nullptr)
    {
        Lisp_CommandEdit->setPrompt(str->value().c_str());
        Lisp_CommandEdit->setFocus();
        Lisp_CommandEdit->doProcess(false);

        String result = RS_Lsp_InputHandle::readLine(Lisp_CommandEdit).toStdString();
        Q_UNUSED(result);

        if (Lisp_CommandEdit->dockName().compare("Lisp Ide") == 0)
        {
            Lisp_CommandEdit->setPrompt("_$ ");
        }
        else
        {
            Lisp_CommandEdit->setPrompt(QObject::tr("Command: "));
        }
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("LibreCAD");
        msgBox.setText(str->value().c_str());
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
    }
    return lcl::nilValue();
}

BUILTIN("py-eval-float")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, com);
    double result;
    QString error;
    int err = RS_PYTHON->evalFloat(com->value().c_str(), result, error);
    if (err == 0)
    {
        return lcl::ldouble(result);
    }
    std::cout << error.toStdString() << std::endl;
    LCL_FAIL("'py-eval-float' exec python failed");
    return lcl::nilValue();
}

BUILTIN("py-eval-integer")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, com);
    int result;
    QString error;
    int err = RS_PYTHON->evalInteger(com->value().c_str(), result, error);
    if (err == 0)
    {
        return lcl::integer(result);
    }
    std::cout << error.toStdString() << std::endl;
    LCL_FAIL("'py-eval-integer' exec python failed");
     return lcl::nilValue();
}

BUILTIN("py-eval-string")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, com);
    QString result;
    QString error;
    int err = RS_PYTHON->evalString(com->value().c_str(), result, error);
    if (err == 0)
    {
        return lcl::string(result.toStdString());
    }
    std::cout << error.toStdString() << std::endl;
    LCL_FAIL("'py-eval-string' exec python failed");
    return lcl::nilValue();
}

BUILTIN("py-eval-value")
{
    CHECK_ARGS_IS(1);
    bool is_map = false;
    ARG(lclString, com);
    QString result;
    QString error;
    std::string command = "print(";
    command += com->value();
    command += ")";
    int err = RS_PYTHON->evalString(command.c_str(), result, error);

    if (err == 0)
    {
        for (auto i = result.size()-1; i > 0; --i)
        {
            if (result.at(i) == ':')
            {
                is_map = true;
                result.remove(i-1, 2);
                continue;
            }

            if (is_map && result.at(i) == '\'')
            {
                is_map = false;
                result.replace(i, 1, ":");
            }
        }

        static QRegularExpression dq = QRegularExpression(QStringLiteral("[\"]"));
        static QRegularExpression q = QRegularExpression(QStringLiteral("[']"));
        static QRegularExpression rb = QRegularExpression(QStringLiteral("[(]"));
        result.remove(',');
        result.replace(dq, "\\\"");
        result.replace(q, "\"");
        result.replace(rb, "'(");
        return EVAL(readStr(result.toStdString()), NULL);
    }
    std::cout << error.toStdString();
    LCL_FAIL("'py-eval-value' exec python failed");
    return lcl::nilValue();
}

BUILTIN("py-eval-vector")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, com);
    v3_t result;
    int err = RS_PYTHON->evalVector(com->value().c_str(), result);

    if (err == 0)
    {
        lclValueVec* items = new lclValueVec(3);
        items->at(0) = lcl::ldouble(result.x);
        items->at(1) = lcl::ldouble(result.y);
        items->at(2) = lcl::ldouble(result.z);
        return lcl::list(items);
    }
    LCL_FAIL("'py-eval-vector' exec python failed");
    return lcl::nilValue();
}

BUILTIN("py-simple-string")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, com);
    return lcl::integer(RS_PYTHON->runString(com->value().c_str()));
}

BUILTIN("py-simple-file")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, com);
    return lcl::integer(RS_PYTHON->runFile(com->value().c_str()));
}

BUILTIN("rand")
{
    CHECK_ARGS_IS(0);
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(0.0, 1.0);

    return lcl::ldouble(dis(gen));
}

BUILTIN("rand-int")
{
    CHECK_ARGS_IS(1);
    AG_INT(max);
    return lcl::integer(rand() % max->value());
}

BUILTIN("read-string")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, str);

    return readStr(str->value());
}

BUILTIN("read-line")
{
    if (!CHECK_ARGS_AT_LEAST(0))
    {
        QString s;
        bool fallback = true;
        if (fallback)
        {
            s = QInputDialog::getText(nullptr,
                                    "LibreCAD",
                                    QObject::tr("Enter a text:"),
                                    QLineEdit::Normal, "", nullptr, Qt::WindowFlags(), Qt::ImhNone);
            return lcl::string(s.toStdString());
        }
        else
        {
            //line = Lisp_CommandEdit->getline(str->value().c_str());
            return lcl::string(s.toStdString());
        }
    }
    ARG(lclFile, pf);

    return pf->readLine();
}

BUILTIN("read-char")
{
    if (!CHECK_ARGS_AT_LEAST(0))
    {
        return lcl::integer(int(RS_InputDialog::readChar()));
    }
    ARG(lclFile, pf);

    return pf->readChar();
}
#if 0
BUILTIN("readline")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, str);

    return readline(str->value());
}
#endif
BUILTIN("rem")
{
    CHECK_ARGS_AT_LEAST(2);
    if (ARGS_HAS_FLOAT) {
        [[maybe_unused]] double floatValue = 0;
        if (FLOAT_PTR) {
            ADD_FLOAT_VAL(*floatVal)
            floatValue = floatValue + floatVal->value();
            LCL_CHECK(floatVal->value() != 0.0, "Division by zero");
    }
    else {
        ADD_INT_VAL(*intVal)
        floatValue = floatValue + double(intVal->value());
        LCL_CHECK(intVal->value() != 0, "Division by zero");
    }
    argsBegin++;
    do {
        if (FLOAT_PTR) {
            ADD_FLOAT_VAL(*floatVal)
            floatValue = fmod(floatValue, floatVal->value());
            LCL_CHECK(floatVal->value() != 0.0, "Division by zero");
        }
        else {
            ADD_INT_VAL(*intVal)
            floatValue = fmod(floatValue, double(intVal->value()));
            LCL_CHECK(intVal->value() != 0, "Division by zero");
        }
        argsBegin++;
    } while (argsBegin != argsEnd);
    return lcl::ldouble(floatValue);
    } else {
        [[maybe_unused]] int64_t intValue = 0;
        ADD_INT_VAL(*intVal) // +
        intValue = intValue + intVal->value();
        LCL_CHECK(intVal->value() != 0, "Division by zero");
        argsBegin++;
        do {
            ADD_INT_VAL(*intVal)
            intValue = int(fmod(double(intValue), double(intVal->value())));
            LCL_CHECK(intVal->value() != 0, "Division by zero");
            argsBegin++;
        } while (argsBegin != argsEnd);
        return lcl::integer(intValue);
    }
}

BUILTIN("reset!")
{
    CHECK_ARGS_IS(2);
    ARG(lclAtom, atom);
    return atom->reset(*argsBegin);
}

BUILTIN("rest")
{
    CHECK_ARGS_IS(1);
    if (*argsBegin == lcl::nilValue()) {
        return lcl::list(new lclValueVec(0));
    }
    ARG(lclSequence, seq);
    return seq->rest();
}

BUILTIN("reverse")
{
    CHECK_ARGS_IS(1);
    if (*argsBegin == lcl::nilValue()) {
        return lcl::list(new lclValueVec(0));
    }
    ARG(lclSequence, seq);
    return seq->reverse(seq->begin(), seq->end());
}

BUILTIN("seq")
{
    CHECK_ARGS_IS(1);
    lclValuePtr arg = *argsBegin++;
    if (arg == lcl::nilValue()) {
        return lcl::nilValue();
    }
    if (const lclSequence* seq = DYNAMIC_CAST(lclSequence, arg)) {
        return seq->isEmpty() ? lcl::nilValue()
                              : lcl::list(seq->begin(), seq->end());
    }
    if (const lclString* strVal = DYNAMIC_CAST(lclString, arg)) {
        const String str = strVal->value();
        int length = str.length();
        if (length == 0)
            return lcl::nilValue();

        lclValueVec* items = new lclValueVec(length);
        for (int i = 0; i < length; i++) {
            (*items)[i] = lcl::string(str.substr(i, 1));
        }
        return lcl::list(items);
    }
    LCL_FAIL("'%s' is not a string or sequence", arg->print(true).c_str());
    return lcl::nilValue();
}

BUILTIN("set_tile")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, key);
    ARG(lclString, val);

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
            case EDIT_BOX:
            {
                const lclEdit* edit = static_cast<const lclEdit*>(tile);
                edit->edit()->setText(val->value().c_str());
            }
                break;
            case TEXT:
            case ERRTILE:
            {
                const lclLabel* l = static_cast<const lclLabel*>(tile);
                l->label()->setText(val->value().c_str());
            }
                break;
            case BUTTON:
            {
                const lclButton* b = static_cast<const lclButton*>(tile);
                b->button()->setText(val->value().c_str());
            }
                break;
            case RADIO_BUTTON:
            {
                const lclRadioButton* rb = static_cast<const lclRadioButton*>(tile);
                rb->button()->setText(val->value().c_str());
            }
                break;
            case TOGGLE:
            {
                const lclToggle* tb = static_cast<const lclToggle*>(tile);
                if(val->value() == "0")
                {
                    tb->toggle()->setChecked(false);
                }
                if(val->value() == "1")
                {
                    tb->toggle()->setChecked(true);
                }
            }
                break;
            case OK_CANCEL_HELP_ERRTILE:
            {
                const lclOkCancelHelpErrtile* err = static_cast<const lclOkCancelHelpErrtile*>(tile);
                err->errtile()->setText(val->value().c_str());
            }
                break;
            case DIAL:
            {
                const lclDial* sc = static_cast<const lclDial*>(tile);
                if (std::regex_match(val->value().c_str(), intRegex))
                {
                    sc->slider()->setValue(atoi(val->value().c_str()));
                }
            }
             break;
            case SCROLL:
            {
                const lclScrollBar* sc = static_cast<const lclScrollBar*>(tile);
                if (std::regex_match(val->value().c_str(), intRegex))
                {
                    sc->slider()->setValue(atoi(val->value().c_str()));
                }
            }
                break;
            case SLIDER:
            {
                const lclSlider* sc = static_cast<const lclSlider*>(tile);
                if (std::regex_match(val->value().c_str(), intRegex))
                {
                    sc->slider()->setValue(atoi(val->value().c_str()));
                }
            }
                break;
            default:
                return lcl::nilValue();
            }
            return lcl::trueValue();
        }
    }

    return lcl::nilValue();
}

BUILTIN("setvar")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, id);
    return shadowEnv->set(id->value(), EVAL(*argsBegin, NULL));
}

BUILTIN("sin")
{
    BUILTIN_FUNCTION(sin);
}

BUILTIN("slurp")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, filename);

    std::ios_base::openmode openmode =
        std::ios::ate | std::ios::in | std::ios::binary;
    std::ifstream file(filename->value().c_str(), openmode);
    LCL_CHECK(!file.fail(), "Cannot open %s", filename->value().c_str());

    String data;
    data.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    data.append(std::istreambuf_iterator<char>(file.rdbuf()),
                std::istreambuf_iterator<char>());

    return lcl::string(data);
}

BUILTIN("sqrt")
{
    BUILTIN_FUNCTION(sqrt);
}

BUILTIN("start_dialog")
{
    CHECK_ARGS_IS(0);
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
                if(tile->value().initial_focus != "")
                {
                    for (auto & child : dclTiles)
                    {
                        if (child->value().dialog_Id == dialogId->value() &&
                            child->value().key == tile->value().initial_focus)
                        {
                            switch (child->value().id)
                            {
                                case BUTTON:
                                {
                                    const lclButton* b = static_cast<const lclButton*>(tile);
                                    b->button()->setFocus();
                                }
                                    break;
                                case EDIT_BOX:
                                {
                                    const lclEdit* edit = static_cast<const lclEdit*>(tile);
                                    edit->edit()->setFocus();
                                }
                                    break;
                                case IMAGE_BUTTON:
                                {
                                    const lclImageButton* ib = static_cast<const lclImageButton*>(tile);
                                    ib->button()->setFocus();
                                }
                                    break;
                                case LIST_BOX:
                                {
                                    const lclListBox* l = static_cast<const lclListBox*>(tile);
                                    l->list()->setFocus();
                                }
                                    break;
                                case POPUP_LIST:
                                {
                                    const lclPopupList* pl = static_cast<const lclPopupList*>(tile);
                                    pl->list()->setFocus();
                                }
                                    break;
                                case RADIO_BUTTON:
                                {
                                    const lclRadioButton* rb = static_cast<const lclRadioButton*>(tile);
                                    rb->button()->setFocus();
                                }
                                    break;
                                case SCROLL:
                                {
                                    const lclScrollBar* sb = static_cast<const lclScrollBar*>(tile);
                                    sb->slider()->setFocus();
                                }
                                    break;
                                case SLIDER:
                                {
                                    const lclSlider* sl = static_cast<const lclSlider*>(tile);
                                    sl->slider()->setFocus();
                                }
                                    break;
                                case DIAL:
                                {
                                    const lclDial* sc = static_cast<const lclDial*>(tile);
                                    sc->slider()->setFocus();
                                }
                                    break;
                                case TOGGLE:
                                {
                                    const lclToggle* tb = static_cast<const lclToggle*>(tile);
                                    tb->toggle()->setFocus();
                                }
                                    break;
                                case TAB:
                                {
                                    const lclWidget* w = static_cast<const lclWidget*>(tile);
                                    w->widget()->setFocus();
                                }
                                break;
                                default:
                                    break;
                            }
                        }
                    }
                }
                return lcl::integer(dlg->dialog()->exec());
            }
        }
    }
    return lcl::nilValue();
}

BUILTIN("start_image")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, key);

    return dclEnv->set("start_image_key", lcl::string(key->value()));

    // FIXMI check if not in current
    //return lcl::nilValue();
}

BUILTIN("start_list")
{
    int args = CHECK_ARGS_BETWEEN(1, 3);

    ARG(lclString, key);
    if (args == 1)
    {
        dclEnv->set("start_list_operation", lcl::integer(2));
        dclEnv->set("start_list_index", lcl::nilValue());
    }
    if (args > 2)
    {
        AG_INT(operation);
        dclEnv->set("start_list_operation", lcl::integer(operation->value()));
    }
    if (args == 3)
    {
        AG_INT(index);
        dclEnv->set("start_list_index", lcl::integer(index->value()));
    }
    return dclEnv->set("start_list_key", lcl::string(key->value()));

    // FIXMI check if not in current
    //return lcl::nilValue();
}

BUILTIN("startapp")
{
    int count = CHECK_ARGS_AT_LEAST(1);
    ARG(lclString, com);
    String command = com->value();

    if (count > 1)
    {
        ARG(lclString, para);
        command += " ";
        command += para->value();
    }

    if (system(command.c_str()))
    {
        return lcl::nilValue();
    }
    return lcl::integer(count);
}

BUILTIN("str")
{
    Q_UNUSED(name);
    return lcl::string(printValues(argsBegin, argsEnd, "", false));
}

BUILTIN("strcase")
{
    int count = CHECK_ARGS_AT_LEAST(1);
    ARG(lclString, str);
    String trans = str->value();

    if (count > 1)
    {
        ARG(lclConstant, boolVal);
        if (boolVal->isTrue())
        {
            std::transform(trans.begin(), trans.end(), trans.begin(),
                   [](unsigned char c){ return std::tolower(c); });
            return lcl::string(trans);
        }
    }

    std::transform(trans.begin(), trans.end(), trans.begin(),
                   [](unsigned char c){ return std::toupper(c); });

    return lcl::string(trans);
}

BUILTIN("strlen")
{
    CHECK_ARGS_IS(0);
    return lcl::integer(countValues(argsBegin, argsEnd));
}

BUILTIN("substr")
{
    int count = CHECK_ARGS_AT_LEAST(2);
    ARG(lclString, s);
    AG_INT(start);
    int startPos = (int)start->value();

    if (s)
    {
        String bla = s->value();
        if (startPos > (int)bla.size()+1) {
            startPos = (int)bla.size()+1;
        }

        if (count > 2)
        {
            AG_INT(size);
            return lcl::string(bla.substr(startPos-1, size->value()));
        }
        else
        {
                return lcl::string(bla.substr(startPos-1, bla.size()));
        }
    }

    return lcl::string(String(""));
}

BUILTIN("subst")
{
    CHECK_ARGS_IS(3);
    lclValuePtr newSym = *argsBegin++;
    lclValuePtr oldSym = *argsBegin++;
    ARG(lclSequence, seq);

    const int length = seq->count();
    lclValueVec* items = new lclValueVec(length);
    std::copy(seq->begin(), seq->end(), items->begin());

    for (int i = 0; i < length; i++) {
        if (items->at(i)->print(true).compare(oldSym->print(true)) == 0) {
            items->at(i) = newSym;
            return lcl::list(items);
        }
    }
    return lcl::nilValue();
}

BUILTIN("swap!")
{
    CHECK_ARGS_AT_LEAST(2);
    ARG(lclAtom, atom);

    lclValuePtr op = *argsBegin++; // this gets checked in APPLY

    lclValueVec args(1 + argsEnd - argsBegin);
    args[0] = atom->deref();
    std::copy(argsBegin, argsEnd, args.begin() + 1);

    lclValuePtr value = APPLY(op, args.begin(), args.end());
    return atom->reset(value);
}

BUILTIN("symbol")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, token);
    return lcl::symbol(token->value());
}

BUILTIN("tan")
{
    BUILTIN_FUNCTION(tan);
}

BUILTIN("term_dialog")
{
    CHECK_ARGS_IS(0);
    for (int i = dclTiles.size() - 1; i >= 0; i--)
    {
        if (dclTiles.at(i)->value().id == DIALOG)
        {
            const lclDialog* dlg = static_cast<const lclDialog*>(dclTiles.at(i));
            dlg->dialog()->done(0);
            dlg->dialog()->deleteLater();
            dclEnv->set(STRF("#builtin-gui(%d)", dclTiles.at(i)->value().dialog_Id), lcl::nilValue());
        }
    }
    dclTiles.clear();
    dclEnv->set("load_dialog_id", lcl::nilValue());

    return lcl::nilValue();
}

BUILTIN("terpri")
{
    CHECK_ARGS_IS(0);
    std::cout << std::endl;
    return lcl::nilValue();
}

BUILTIN("text_image")
{
    CHECK_ARGS_IS(6);
    AG_INT(x1);
    AG_INT(y1);
    AG_INT(width);
    AG_INT(height);
    ARG(lclString, text);
    AG_INT(color);

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
                img->image()->addText(x1->value(), y1->value(),
                                    width->value(), height->value(),
                                    text->value().c_str(),
                                    color->value());

                return lcl::string(text->value());
            }
            break;
            case IMAGE_BUTTON:
            {
                const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                img->button()->addText(x1->value(), y1->value(),
                                      width->value(), height->value(),
                                      text->value().c_str(),
                                      color->value());

                return lcl::string(text->value());
            }
            break;
            default:
                return lcl::nilValue();
            }
        }
    }
    return lcl::nilValue();
}

BUILTIN("throw")
{
    CHECK_ARGS_IS(1);
    throw *argsBegin;
}

BUILTIN("time-ms")
{
    CHECK_ARGS_IS(0);

    using namespace std::chrono;
    milliseconds ms = duration_cast<milliseconds>(
        high_resolution_clock::now().time_since_epoch()
    );

    return lcl::integer(ms.count());
}

BUILTIN("timeout")
{
    CHECK_ARGS_IS(1);
    AG_INT(timeout);
    SleeperThread::msleep(timeout->value());
    return lcl::nilValue();
}

BUILTIN("type?")
{
    CHECK_ARGS_IS(1);

    if (argsBegin->ptr()->print(true).compare("nil") == 0) {
        return lcl::nilValue();
    }

    return lcl::type(argsBegin->ptr()->type());
}

BUILTIN("unload_dialog")
{
    CHECK_ARGS_IS(1);
    AG_INT(id);

    if(id)
    {
        for (auto & tile : dclTiles)
        {
            if (tile->value().dialog_Id == id->value())
            {
                const lclDialog* dlg = static_cast<const lclDialog*>(tile);
                delete dlg->dialog();

                break;
            }
        }
    }

    for (int i = 0; i < (int) dclTiles.size(); i++)
    {
        if(dclTiles.at(i)->value().dialog_Id == id->value())
        {
            dclTiles.erase(dclTiles.begin()+i);
        }
    }

    dclEnv->set(STRF("#builtin-gui(%d)", id->value()), lcl::nilValue());
    dclEnv->set("load_dialog_id", lcl::nilValue());

    return lcl::nilValue();
}


BUILTIN("vals")
{
    CHECK_ARGS_IS(1);
    ARG(lclHash, hash);
    return hash->values();
}

BUILTIN("vec")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, s);
    return lcl::vector(s->begin(), s->end());
}

BUILTIN("vector")
{
    CHECK_ARGS_AT_LEAST(0);
    return lcl::vector(argsBegin, argsEnd);
}

BUILTIN("ver")
{
    CHECK_ARGS_IS(0);
    return lcl::string(Lisp_GetVersion());
}

BUILTIN("vl-consp")
{
    CHECK_ARGS_IS(1);
    ARG(lclSequence, s);

    if(s->isDotted()) {
        return lcl::trueValue();
    }
    return lcl::nilValue();
}

BUILTIN("vl-directory-files")
{
    int count = CHECK_ARGS_AT_LEAST(0);
    int len = 0;
    String path = "./";
    lclValueVec* items;
    std::vector<std::filesystem::path> sorted_by_name;

    if (count > 0) {
        ARG(lclString, directory);
        path = directory->value();
        if (!std::filesystem::exists(path.c_str())) {
            return lcl::nilValue();
        }
        if (count > 1 && (NIL_PTR || INT_PTR) && !(count == 2 && (NIL_PTR || INT_PTR))) {
            if (NIL_PTR) {
                argsBegin++;
            }
            // pattern + dirs
            AG_INT(directories);
            switch(directories->value())
            {
                case -1:
                    for (const auto & entry : std::filesystem::directory_iterator(path.c_str())) {
                        if (std::filesystem::is_directory(entry.path()))
                        {
                            sorted_by_name.push_back(entry.path().filename());
                            len++;
                        }
                    }
                    break;
                case 0:
                    for (const auto & entry : std::filesystem::directory_iterator(path.c_str())) {
                        sorted_by_name.push_back(entry.path());
                        len++;
                    }
                    break;
                case 1:
                    for (const auto & entry : std::filesystem::directory_iterator(path.c_str())) {
                        if (!std::filesystem::is_directory(entry.path()))
                        {
                            sorted_by_name.push_back(entry.path().filename());
                            len++;
                        }
                    }
                    break;
                default: {}
            }
        }
        else if (count > 1 && !(count == 2 && (NIL_PTR || INT_PTR))) {
            ARG(lclString, pattern);
            int dir = 3;
            if (count > 2) {
                AG_INT(directories2);
                dir = directories2->value();
                if (dir > 1 || dir < -1) {
                    dir = 0;
                }
            }
            // pattern
            bool hasExt = false;
            bool hasName = false;
            String pat = pattern->value();
            int asterix = (int) pat.find_last_of("*");
            if (asterix != -1 && (int) pat.size() >= asterix) {
                hasExt = true;
            }
            if (asterix != -1 && (int) pat.size() >= asterix && pat.size() > pat.substr(asterix+1).size() ) {
                hasName = true;
            }
            for (const auto & entry : std::filesystem::directory_iterator(path.c_str())) {
                if (!std::filesystem::is_directory(entry.path()) &&
                    hasExt &&
                    hasName &&
                    (dir == 3 || dir == 1)) {
                    if ((int)entry.path().filename().string().find(pat.substr(asterix+1)) != -1 &&
                        (int)entry.path().filename().string().find(pat.substr(0, asterix)) != -1) {
                        sorted_by_name.push_back(entry.path().filename());
                        len++;
                    }
                }
                if (!std::filesystem::is_directory(entry.path()) && !hasExt &&
                    (int)entry.path().filename().string().find(pat) != -1 && (dir == 3 || dir == 1)) {
                    sorted_by_name.push_back(entry.path().filename());
                    len++;
                }
                if (std::filesystem::is_directory(entry.path()) && !hasExt &&
                    (int)entry.path().filename().string().find(pat) != -1  && dir == -1) {
                    sorted_by_name.push_back(entry.path().filename());
                    len++;
                }
                if ((int)entry.path().string().find(pat) != -1 && dir == 0) {
                    sorted_by_name.push_back(entry.path());
                    len++;
                }
            }
        }
        else {
            // directory
            for (const auto & entry : std::filesystem::directory_iterator(path.c_str())) {
                if (!std::filesystem::is_directory(entry.path()))
                {
                    sorted_by_name.push_back(entry.path().filename());
                    len++;
                }
            }
        }
    }
    else {
        // current directory
        for (const auto & entry : std::filesystem::directory_iterator(path.c_str())) {
            if (!std::filesystem::is_directory(entry.path()))
            {
                sorted_by_name.push_back(entry.path().filename());
                len++;
            }
        }
    }
    std::sort(sorted_by_name.begin(), sorted_by_name.end(), compareNat);
    items = new lclValueVec(len);
    len = 0;
    for (const auto & filename : sorted_by_name) {
        items->at(len) = lcl::string(filename);
        len++;
    }
    return items->size() ? lcl::list(items) : lcl::nilValue();
}

BUILTIN("vl-file-copy")
{
    int count = CHECK_ARGS_AT_LEAST(2);
    ARG(lclString, source);
    ARG(lclString, dest);

    if (count == 3 && argsBegin->ptr()->isTrue()) {

        std::ofstream of;
        std::ios_base::openmode openmode =
            std::ios::ate | std::ios::in | std::ios::binary;
        std::ifstream file(source->value().c_str(), openmode);

        if (file.fail()) {
            return lcl::nilValue();
        }

        String data;
        data.reserve(file.tellg());
        file.seekg(0, std::ios::beg);
        data.append(std::istreambuf_iterator<char>(file.rdbuf()), std::istreambuf_iterator<char>());

        of.open(dest->value(), std::ios::app);
        if (!of) {
            return lcl::nilValue();
        }
        else {
            of << data;
            of.close();
            return lcl::integer(sizeof source->value());
        }
    }

    std::error_code err;
    std::filesystem::copy(source->value(), dest->value(), std::filesystem::copy_options::update_existing, err);
    if (err) {
        return lcl::nilValue();
    }
    return lcl::integer(sizeof source->value());
}

BUILTIN("vl-file-delete")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);
    if (!std::filesystem::exists(path->value().c_str())) {
        return lcl::nilValue();
    }
    if (std::filesystem::remove(path->value().c_str()))
    {
        return lcl::trueValue();
    }
    return lcl::nilValue();
}

BUILTIN("vl-file-directory-p")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);
    const std::filesystem::directory_entry dir(path->value().c_str());
    if (std::filesystem::exists(path->value().c_str()) &&
        dir.is_directory()) {
        return lcl::trueValue();
    }
    return lcl::nilValue();
}

BUILTIN("vl-file-rename")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, path);
    ARG(lclString, newName);
    if (!std::filesystem::exists(path->value().c_str())) {
        return lcl::nilValue();
    }
    std::error_code err;
    std::filesystem::rename(path->value().c_str(), newName->value().c_str(), err);
    if (err) {
        return lcl::nilValue();
    }
    return lcl::trueValue();
}

BUILTIN("vl-file-size")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);
    if (!std::filesystem::exists(path->value().c_str())) {
        return lcl::nilValue();
    }
    if (!std::filesystem::is_directory(path->value().c_str())) {
        return lcl::string("0");
    }
    try {
        [[maybe_unused]] auto size = std::filesystem::file_size(path->value().c_str());
        char str[50];
        sprintf(str, "%ld", size);
        return lcl::string(str);
    }
    catch (std::filesystem::filesystem_error&) {}
    return lcl::nilValue();
}

BUILTIN("vl-file-systime")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);
    if (!std::filesystem::exists(path->value().c_str())) {
        return lcl::nilValue();
    }

    std::filesystem::file_time_type ftime = std::filesystem::last_write_time(path->value().c_str());
    std::time_t cftime = to_time_t(ftime); // assuming system_clock

    char buffer[64];
    int J,M,W,D,h,m,s;

    if (strftime(buffer, sizeof buffer, "%Y %m %w %e %I %M %S", std::localtime(&cftime))) {
        sscanf (buffer,"%d %d %d %d %d %d %d",&J,&M,&W,&D,&h,&m,&s);

        lclValueVec* items = new lclValueVec(6);
        items->at(0) = new lclInteger(J);
        items->at(1) = new lclInteger(M);
        items->at(2) = new lclInteger(W);
        items->at(3) = new lclInteger(D);
        items->at(4) = new lclInteger(m);
        items->at(5) = new lclInteger(s);
        return lcl::list(items);
    }
    return lcl::nilValue();
}

BUILTIN("vl-filename-base")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);

    const std::filesystem::path p(path->value());
    return lcl::string(p.stem());
}

BUILTIN("vl-filename-directory")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);

    const std::filesystem::path p(path->value());
    if (!p.has_extension()) {
        return lcl::string(path->value());
    }

    const auto directory = std::filesystem::path{ p }.parent_path().string();
    return lcl::string(directory);
}

BUILTIN("vl-filename-extension")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, path);

    const std::filesystem::path p(path->value());
    if (!p.has_extension()) {
        return lcl::nilValue();
    }

    return lcl::string(p.extension());
}

BUILTIN("vl-filename-mktemp")
{
    int count = CHECK_ARGS_AT_LEAST(0);
    char num[4];
    sprintf(num, "%03x", ++tmpFileCount);
    String filename = "tmpfile_";
    String path;
    std::filesystem::path p(std::filesystem::temp_directory_path());
     std::filesystem::path d("");

    filename +=  + num;
    path = p / filename;

    if (count > 0) {
        ARG(lclString, pattern);
        p = pattern->value().c_str();
        filename = p.stem();
        filename +=  + num;
        if (!p.has_root_path()) {
            path = std::filesystem::temp_directory_path() / d;
        }
        else {
            path = p.root_path() / p.relative_path().remove_filename();
        }
        if (p.has_extension()) {
            filename += p.extension();
        }
        path += filename;
    }
    if (count > 1) {
        ARG(lclString, directory);
        path = directory->value() / d;
        path += filename;
    }
    if (count == 3) {
        ARG(lclString, extension);
        path += extension->value();
    }
    return lcl::string(path);
}

BUILTIN("vl-mkdir")
{
    CHECK_ARGS_IS(1);
    ARG(lclString, dir);

    if(std::filesystem::create_directory(dir->value())) {
        return lcl::trueValue();
    }
    return lcl::nilValue();
}

BUILTIN("vl-position")
{
    CHECK_ARGS_IS(2);
    lclValuePtr op = *argsBegin++; // this gets checked in APPLY

    const lclSequence* seq = VALUE_CAST(lclSequence, *(argsBegin));
    for (int i = 0; i < seq->count(); i++) {
        if(seq->item(i)->print(true).compare(op->print(true)) == 0) {
            return lcl::integer(i);
        }
    }
    return lcl::nilValue();
}

BUILTIN("slide_image")
{
    CHECK_ARGS_IS(5);
    AG_INT(x1);
    AG_INT(y1);
    AG_INT(width);
    AG_INT(height);
    ARG(lclString, filename);

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
                img->image()->addSlide(x1->value(), y1->value(), width->value(), height->value(), tile->value().aspect_ratio, filename->value().c_str());
                return lcl::string(filename->value());
            }
            break;
            case IMAGE_BUTTON:
            {
                const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                img->button()->addSlide(x1->value(), y1->value(), width->value(), height->value(), tile->value().aspect_ratio, filename->value().c_str());
                return lcl::string(filename->value());
            }
            break;
            default:
                return lcl::nilValue();
            }
        }
    }
    return lcl::nilValue();
}

BUILTIN("symbol")
{
    CHECK_ARGS_IS(1);
    if(argsBegin->ptr()->type() == LCLTYPE::SYM) {
        return lcl::trueValue();
    }
    return lcl::nilValue();
}

BUILTIN("vector_image")
{
    CHECK_ARGS_IS(5);
    AG_INT(x1);
    AG_INT(y1);
    AG_INT(x2);
    AG_INT(y2);
    AG_INT(color);

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
                img->image()->addLine(x1->value(), y1->value(), x2->value(), y2->value(), color->value());
                return lcl::integer(color->value());
            }
            break;
            case IMAGE_BUTTON:
            {
                const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                img->button()->addLine(x1->value(), y1->value(), x2->value(), y2->value(), color->value());
                return lcl::integer(color->value());
            }
            break;
            default:
                return lcl::nilValue();
            }
        }
    }
    return lcl::nilValue();
}

BUILTIN("wcmatch")
{
    CHECK_ARGS_IS(2);
    ARG(lclString, str);
    ARG(lclString, p);
    std::vector<String> StringList;
    String del = ",";
    String pat = p->value();
    auto pos = pat.find(del);

    while (pos != String::npos) {
        StringList.push_back(pat.substr(0, pos));
        pat.erase(0, pos + del.length());
        pos = pat.find(del);
    }
    StringList.push_back(pat);
    for (auto &it : StringList) {
        String pattern = it;
        String expr = "";
        bool exclude = false;
        bool open_br = false;
        for (auto &ch : it) {
            switch (ch) {
                case '#':
                    expr += "(\\d)";
                    break;
                case '@':
                    expr += "[A-Za-zÀ-ȕ]";
                    break;
                case ' ':
                    expr += "[ ]+";
                    break;
                case '.':
                    expr += "([^(A-Za-z0-9 )]{1,})";
                    break;
                case '*':
                    expr += "(.*)";
                    break;
                case '?':
                    expr += "[A-Za-zÀ-ȕ0-9_ ]";
                    break;
                case '~': {
                    if (open_br) {
                        expr += "^";
                    } else {
                        expr += "[^";
                    exclude = true;
                    }
                    break;
                }
                case '[':
                    expr += "[";
                    open_br = true;
                    break;
                case ']': {
                    expr += "]{1}";
                    open_br = false;
                    break;
                }
                case '`':
                    expr += "//";
                    break;
                default: {
                    expr += ch;
                }
            }
        }
        if (exclude) {
            expr += "]*";
        }
        std::regex e (expr);
        if (std::regex_match (str->value(),e)) {
            return lcl::trueValue();
        }
    }
    return lcl::nilValue();
}

BUILTIN("with-meta")
{
    CHECK_ARGS_IS(2);
    lclValuePtr obj  = *argsBegin++;
    lclValuePtr meta = *argsBegin++;
    return obj->withMeta(meta);
}

BUILTIN("write-line")
{
    int count = CHECK_ARGS_AT_LEAST(1);
    //multi
    ARG(lclString, str);

    if (count == 1)
    {
        return lcl::string(str->value());
    }

    ARG(lclFile, pf);

    return pf->writeLine(str->value());
}

BUILTIN("write-char")
{
    int count = CHECK_ARGS_AT_LEAST(1);
    AG_INT(c);

    std::cout << itoa64(c->value()) << std::endl;

    if (count == 1)
    {
        return lcl::integer(c->value());
    }

    ARG(lclFile, pf);

    return pf->writeChar(itoa64(c->value()));
}

BUILTIN("zero?")
{
    CHECK_ARGS_IS(1);
    if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::REAL)
    {
        lclDouble* val = VALUE_CAST(lclDouble, EVAL(*argsBegin, NULL));
        return lcl::boolean(val->value() == 0.0);
    }
    else if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::INT)
    {
        lclInteger* val = VALUE_CAST(lclInteger, EVAL(*argsBegin, NULL));
        return lcl::boolean(val->value() == 0);
    }
    return lcl::falseValue();
}

BUILTIN("zerop")
{
    CHECK_ARGS_IS(1);
    if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::REAL)
    {
        lclDouble* val = VALUE_CAST(lclDouble, EVAL(*argsBegin, NULL));
        return val->value() == 0 ? lcl::trueValue() : lcl::nilValue();
    }
    else if (EVAL(*argsBegin, NULL)->type() == LCLTYPE::INT)
    {
        lclInteger* val = VALUE_CAST(lclInteger, EVAL(*argsBegin, NULL));
        return val->value() == 0 ? lcl::trueValue() : lcl::nilValue();
    }
    return lcl::nilValue();
}

void installCore(lclEnvPtr env) {
    for (auto it = handlers.begin(), end = handlers.end(); it != end; ++it) {
        lclBuiltIn* handler = *it;
        env->set(handler->name(), handler);
    }
}

static String printValues(lclValueIter begin, lclValueIter end,
                          const String& sep, bool readably)
{
    String out;

    if (begin != end) {
        out += (*begin)->print(readably);
        ++begin;
    }

    for ( ; begin != end; ++begin) {
        out += sep;
        out += (*begin)->print(readably);
    }

    return out;
}

static int countValues(lclValueIter begin, lclValueIter end)
{
    int result = 0;

    if (begin != end) {
        result += (*begin)->print(true).length() -2;
        ++begin;
    }

    for ( ; begin != end; ++begin) {
        result += (*begin)->print(true).length() -2;
    }

    return result;
}

#endif // DEVELOPER
