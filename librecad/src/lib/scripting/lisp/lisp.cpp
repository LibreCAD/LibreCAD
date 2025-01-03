#include "lisp.h"
#include "LCL.h"

#include "Environment.h"
//#include "ReadLine.h"
#include "Types.h"

#ifdef DEVELOPER

#include <iostream>
#include <filesystem>
#include <algorithm>
#include <QDebug>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QKeySequence>
#include <QShortcut>

#define MAX_FUNC 23

static const char* lclEvalFunctionTable[MAX_FUNC] = {
//    "bla",
    "and",
    "def!",
    "defun",
    "defmacro!",
    "do",
    "fn*",
    "foreach",
    "if",
    "lambda",
    "let*",
    "new_dialog",
    "not",
    "or",
    "progn",
    "quasiquote",
    "quote",
    "repeat",
    "set",
    "setq",
    "trace",
    "try*",
    "untrace",
    "while",
};

bool traceDebug = false;

QG_Lsp_CommandEdit *Lisp_CommandEdit = nullptr;

std::vector<const lclGui*> dclTiles(0);

lclValuePtr READ(const String& input);
String PRINT(lclValuePtr ast);
String strToUpper(String s);

static void installFunctions(lclEnvPtr env);
//  Installs functions, macros and constants implemented in LCL.
static void installEvalCore(lclEnvPtr env);
//  Installs functions from EVAL, implemented in LCL.
static void checkForAlias(const String& com);

static void makeArgv(lclEnvPtr env, int argc, char* argv[]);
static String safeRep(const String& input, lclEnvPtr env);
static lclValuePtr quasiquote(lclValuePtr obj);

const char *Lisp_GetVersion()
{
    return LISP_VERSION_STR;
}

int LispRun_SimpleString(const char *command)
{
    String out = safeRep(command, replEnv);
    if (out.length() > 0) {
        std::cout << out << std::endl;
        fflush(stdout);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int LispRun_SimpleFile(const char *filename)
{
    String out = safeRep(STRF("(load-file \"%s\")", filename), replEnv);
    if (out.length() > 0) {
        std::cout << out << std::endl;
        fflush(stdout);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

std::string Lisp_EvalFile(const char *filename)
{
    return safeRep(STRF("(load-file \"%s\")", filename), replEnv);
}

std::string Lisp_EvalString(const String& input)
{
#if 0
    String out = lispOut.str();
    qDebug() << "out: " << out.c_str();
    out += safeRep(input, replEnv);
    qDebug() << "out: " << out.c_str();
    lispOut.str("");
    return out;
#else
    return safeRep(input.c_str(), replEnv);
#endif
}

void initAlias()
{
    LclCom.push_back({ { "copyright" },   { "copyright" }   });
    LclCom.push_back({ { "credits" },     { "credits" }     });
    LclCom.push_back({ { "help" },        { "help" }        });
    LclCom.push_back({ { "license" },     { "license" }     });
}

int Lisp_Initialize(int argc, char* argv[])
{
    installCore(replEnv);
    installEvalCore(replEnv);
    installFunctions(replEnv);
    initAlias();
    makeArgv(replEnv, argc, argv);
    if (argc > 1) {
        String filename = escape(argv[1]);
        safeRep(STRF("(load-file %s)", filename.c_str()), replEnv);
        return 0;
    }
    rep("(println (str \"LibreLisp [\" *host-language* \"]\"))", replEnv);
    return 0;
}

static String safeRep(const String& input, lclEnvPtr env)
{
    try {
        return rep(input, env);
    }
    catch (lclEmptyInputException&) {
        return String();
    }
    catch (lclValuePtr& mv) {
        return "Error: " + mv->print(true);
    }
    catch (String& s) {
        return "Error: " + s;
    };
}

static void makeArgv(lclEnvPtr env, int argc, char* argv[])
{
    lclValueVec* args = new lclValueVec();
    for (int i = 0; i < argc; i++) {
        args->push_back(lcl::string(argv[i]));
    }
    env->set("*ARGV*", lcl::list(args));
}

String rep(const String& input, lclEnvPtr env)
{
    return PRINT(EVAL(READ(input), env));
}

lclValuePtr READ(const String& input)
{
    return readStr(input);
}

lclValuePtr EVAL(lclValuePtr ast, lclEnvPtr env)
{
    if (!env) {
        env = replEnv;
    }

    try {
        while (1)
        {
            const lclEnvPtr dbgenv = env->find("DEBUG-EVAL");
            if (dbgenv && dbgenv->get("DEBUG-EVAL")->isTrue()) {
                std::cout << "EVAL: " << PRINT(ast) << "\n";
            }

            if (traceDebug) {
                std::cout << "TRACE: " << PRINT(ast) << std::endl;
            }

            const lclList* list = DYNAMIC_CAST(lclList, ast);
            if (!list || (list->count() == 0)) {
                return ast->eval(env);
            }

            // From here on down we are evaluating a non-empty list.
            // First handle the special forms.
            if (const lclSymbol* symbol = DYNAMIC_CAST(lclSymbol, list->item(0))) {
                String special = symbol->value();

                const lclEnvPtr traceEnv = shadowEnv->find(strToUpper(special));
                if (traceEnv && traceEnv->get(strToUpper(special))->print(true) != "nil") {
                    traceDebug = true;
                    std::cout << "TRACE: " << PRINT(ast) << std::endl;
                }
                int argCount = list->count() - 1;

                if (special == "and") {
                    checkArgsAtLeast("and", 2, argCount);
                    int value = 0;
                    for (int i = 1; i < argCount+1; i++) {
                        if (EVAL(list->item(i), env)->isTrue()) {
                            value |= 1;
                        }
                        else {
                            value |= 2;
                        }
                    }
                    return value == 3 ? lcl::falseValue() : lcl::trueValue();
                }
#if 0
                if (special == "bla") {
                    checkArgsAtLeast("bla", 2, argCount);
                    return lcl::nilValue();
                }
#endif
                if (special == "debug-eval") {
                    checkArgsIs("debug-eval", 1, argCount);
                    if (list->item(1) == lcl::trueValue()) {
                        env->set("DEBUG-EVAL", lcl::trueValue());
                        return lcl::trueValue();
                    }
                    else {
                        env->set("DEBUG-EVAL", lcl::falseValue());
                        return lcl::falseValue();
                    }
                }

                if (special == "def!") {
                    checkArgsIs("def!", 2, argCount);
                    const lclSymbol* id = VALUE_CAST(lclSymbol, list->item(1));
                    return env->set(id->value(), EVAL(list->item(2), env));
                }

                if (special == "defmacro!") {
                    checkArgsIs("defmacro!", 2, argCount);

                    const lclSymbol* id = VALUE_CAST(lclSymbol, list->item(1));
                    lclValuePtr body = EVAL(list->item(2), env);
                    const lclLambda* lambda = VALUE_CAST(lclLambda, body);
                    return env->set(id->value(), lcl::macro(*lambda));
                }

                if (special == "defun") {
                    checkArgsAtLeast("defun", 3, argCount);

                    String macro = "(do";
                    const lclSymbol* id = VALUE_CAST(lclSymbol, list->item(1));
                    const lclSequence* bindings =
                        VALUE_CAST(lclSequence, list->item(2));
                    StringVec params;
                    for (int i = 0; i < bindings->count(); i++) {
                        const lclSymbol* sym =
                                VALUE_CAST(lclSymbol, bindings->item(i));
                        params.push_back(sym->value());
                    }

                    for (int i = 3; i <= argCount; i++) {
                        macro += " ";
                        macro += list->item(i)->print(true);
#if 0
                        for (auto it = params.begin(); it != params.end(); it++) {
                            if (list->item(i)->print(true).find(*it) != std::string::npos) {
                                std::cout << "parameter '" << *it << "' in: " << list->item(i)->print(true) << std::endl;
                            }
                        }
#endif
                    }
                    macro += ")";
                    lclValuePtr body = READ(macro);
                    const lclLambda* lambda = new lclLambda(params, body, env);
                    checkForAlias(id->value());
                    return env->set(id->value(), new lclLambda(*lambda, true));
                }

                if (special == "do" || special == "progn") {
                    checkArgsAtLeast(special.c_str(), 1, argCount);

                    for (int i = 1; i < argCount; i++) {
                        EVAL(list->item(i), env);
                    }
                    ast = list->item(argCount);
                    continue; // TCO
                }

                if (special == "fn*" || special == "lambda") {
                    checkArgsIs(special.c_str(), 2, argCount);

                    const lclSequence* bindings =
                        VALUE_CAST(lclSequence, list->item(1));
                    StringVec params;
                    for (int i = 0; i < bindings->count(); i++) {
                        const lclSymbol* sym =
                            VALUE_CAST(lclSymbol, bindings->item(i));
                        params.push_back(sym->value());
                    }
                    return lcl::lambda(params, list->item(2), env);
                }

                if (special == "foreach") {
                    checkArgsIs("foreach", 3, argCount);
                    const lclSymbol* sym =
                            VALUE_CAST(lclSymbol, list->item(1));
                    lclSequence* each =
                        VALUE_CAST(lclSequence, EVAL(list->item(2), env));

                    lclEnvPtr inner(new lclEnv(env));
                    inner->set(sym->value(), lcl::nilValue());
                    int count = each->count();
                    lclValuePtr result = NULL;
                    for (int i=0; i < count; i++) {
                        inner->set(sym->value(), each->item(i));
                        result = EVAL(list->item(3), inner);
                    }
                    if (result) {
                        return result;
                    }
                    return lcl::nilValue();
                }

                if (special == "if") {
                    checkArgsBetween("if", 2, 3, argCount);

                    bool isTrue = EVAL(list->item(1), env)->isTrue();
                    if (!isTrue && (argCount == 2)) {
                        return lcl::nilValue();
                    }
                    ast = list->item(isTrue ? 2 : 3);
                    continue; // TCO
                }

                if (special == "let*") {
                    checkArgsIs("let*", 2, argCount);
                    const lclSequence* bindings =
                        VALUE_CAST(lclSequence, list->item(1));
                    int count = checkArgsEven("let*", bindings->count());
                    lclEnvPtr inner(new lclEnv(env));
                    for (int i = 0; i < count; i += 2) {
                        const lclSymbol* var =
                            VALUE_CAST(lclSymbol, bindings->item(i));
                        inner->set(var->value(), EVAL(bindings->item(i+1), inner));
                    }
                    ast = list->item(2);
                    env = inner;
                    continue; // TCO
                }

                if (special == "new_dialog") {
                    checkArgsAtLeast("new_dialog", 2, argCount);
                    const lclString*  dlgName = DYNAMIC_CAST(lclString, list->item(1));
                    const lclInteger* id      = DYNAMIC_CAST(lclInteger, EVAL(list->item(2), env));
                    const lclGui*     gui     = DYNAMIC_CAST(lclGui, dclEnv->get(STRF("#builtin-gui(%d)", id->value())));
                    lclValueVec*      items   = new lclValueVec(gui->value().tiles->size());
                    std::copy(gui->value().tiles->begin(), gui->value().tiles->end(), items->begin());

                    for (auto it = items->begin(), end = items->end(); it != end; it++) {
                        const lclGui* dlg = DYNAMIC_CAST(lclGui, *it);
                        qDebug() << "[EVAL] Dialog name: " << dlg->value().name.c_str();
                        if (dlg->value().name == dlgName->value()) {
                            openTile(dlg);
                            return lcl::trueValue();
                        }
                    }
                    return lcl::nilValue();
                }

                if (special == "not") {
                    checkArgsIs("not", 1, argCount);

                    lclValuePtr cond = list->item(1);

                    if(cond->print(true) == "nil" ||
                        cond->print(true) == "false" )
                    {
                        return lcl::trueValue();
                    }

                    if (lcl::nilValue() == cond ||
                        lcl::falseValue() == cond)
                    {
                        return lcl::trueValue();
                    }

                    cond = EVAL(cond, env);

                    if (cond->print(true) == "nil" ||
                        cond->print(true) == "false" ||
                        !cond->isTrue())
                    {
                        return lcl::trueValue();
                    }
                    return lcl::falseValue();
                }

                if (special == "or") {
                    checkArgsAtLeast("or", 2, argCount);
                    int value = 0;
                    for (int i = 1; i < argCount+1; i++) {
                        if (EVAL(list->item(i), env)->isTrue()) {
                            value |= 1;
                        }
                        else {
                            value |= 2;
                        }
                    }
                    return (value & 1) ? lcl::trueValue() : lcl::falseValue();
                }

                if (special == "quasiquote") {
                    checkArgsIs("quasiquote", 1, argCount);
                    ast = quasiquote(list->item(1));
                    continue; // TCO
                }

                if (special == "quote") {
                    checkArgsIs("quote", 1, argCount);
                    return list->item(1);
                }

                if (special == "repeat") {
                    checkArgsAtLeast("repeat", 2, argCount);
                    const lclInteger* loop = VALUE_CAST(lclInteger, list->item(1));
                    lclValuePtr loopBody;

                    for (int i = 0; i < loop->value(); i++) {
                        for (int j = 1; j < argCount; j++)
                        {
                            loopBody = EVAL(list->item(j+1), env);
                        }
                    }
                    ast = loopBody;
                    continue; // TCO
                }

                if (special == "set") {
                    checkArgsIs("set", 2, argCount);
                    const lclSymbol* id = new lclSymbol(list->item(1)->print(true));
                    return env->set(id->value(), EVAL(list->item(2), env));
                }

                if (special == "setq") {
                    LCL_CHECK(checkArgsAtLeast(special.c_str(), 2, argCount) % 2 == 0, "setq: missing odd number");
                    int i;
                    for (i = 1; i < argCount - 2; i += 2) {
                        const lclSymbol* id = VALUE_CAST(lclSymbol, list->item(i));
                        env->set(id->value(), EVAL(list->item(i+1), env));
                    }
                    const lclSymbol* id = VALUE_CAST(lclSymbol, list->item(i));
                    return env->set(id->value(), EVAL(list->item(i+1), env));
                }
                if (special == "trace") {
                    checkArgsIs("trace", 1, argCount);
                    shadowEnv->set(strToUpper(list->item(1)->print(true)), lcl::trueValue());
                    return lcl::symbol(list->item(1)->print(true));
                }

                if (special == "untrace") {
                    checkArgsIs("untrace", 1, argCount);
                    shadowEnv->set(strToUpper(list->item(1)->print(true)), lcl::nilValue());
                    return lcl::symbol(strToUpper(list->item(1)->print(true)));
                }

                if (special == "try*") {
                    lclValuePtr tryBody = list->item(1);

                    if (argCount == 1) {
                        ast = tryBody;
                        continue; // TCO
                    }
                    checkArgsIs("try*", 2, argCount);
                    const lclList* catchBlock = VALUE_CAST(lclList, list->item(2));

                    checkArgsIs("catch*", 2, catchBlock->count() - 1);
                    LCL_CHECK(VALUE_CAST(lclSymbol,
                        catchBlock->item(0))->value() == "catch*",
                        "catch block must begin with catch*");

                    // We don't need excSym at this scope, but we want to check
                    // that the catch block is valid always, not just in case of
                    // an exception.
                    const lclSymbol* excSym =
                        VALUE_CAST(lclSymbol, catchBlock->item(1));

                    lclValuePtr excVal;

                    try {
                        return EVAL(tryBody, env);
                    }
                    catch(String& s) {
                        excVal = lcl::string(s);
                    }
                    catch (lclEmptyInputException&) {
                        // Not an error, continue as if we got nil
                        ast = lcl::nilValue();
                    }
                    catch(lclValuePtr& o) {
                        excVal = o;
                    };

                    if (excVal) {
                        // we got some exception
                        env = lclEnvPtr(new lclEnv(env));
                        env->set(excSym->value(), excVal);
                        ast = catchBlock->item(2);
                    }
                    continue; // TCO
                }

                if (special == "while") {
                    checkArgsAtLeast("while", 2, argCount);
                    lclValuePtr loop = list->item(1);
                    lclValuePtr loopBody;

                    while (1) {
                        for (int i = 1; i < argCount; i++)
                        {
                            loopBody = EVAL(list->item(i+1), env);
                            loop = EVAL(list->item(1), env);

                            if (!loop->isTrue()) {
                                break;
                            }
                        }

                        if (!loop->isTrue()) {
                            ast = loopBody;
                            break;
                        }
                    }
                    continue; // TCO
                }
            }
            // Now we're left with the case of a regular list to be evaluated.
            lclValuePtr op = EVAL(list->item(0), env);
            if (const lclLambda* lambda = DYNAMIC_CAST(lclLambda, op)) {
                if (lambda->isMacro()) {
                    ast = lambda->apply(list->begin()+1, list->end());
                    traceDebug = false;
                    continue; // TCO
                }
                lclValueVec* items = STATIC_CAST(lclList, list->rest())->evalItems(env);
                ast = lambda->getBody();
                env = lambda->makeEnv(items->begin(), items->end());
                traceDebug = false;
                continue; // TCO
            }
            else {
                lclValueVec* items = STATIC_CAST(lclList, list->rest())->evalItems(env);
                return APPLY(op, items->begin(), items->end());
            }
        }
    }
    catch(int)
    {
        qDebug() << "[EVAL] exit by user";
        return lcl::nilValue();
    }
}

String PRINT(lclValuePtr ast)
{
    return ast->print(true);
}

lclValuePtr APPLY(lclValuePtr op, lclValueIter argsBegin, lclValueIter argsEnd)
{
    const lclApplicable* handler = DYNAMIC_CAST(lclApplicable, op);
    LCL_CHECK(handler != NULL,
              "'%s' is not applicable", op->print(true).c_str());

    return handler->apply(argsBegin, argsEnd);
}

static bool isSymbol(lclValuePtr obj, const String& text)
{
    const lclSymbol* sym = DYNAMIC_CAST(lclSymbol, obj);
    return sym && (sym->value() == text);
}

//  Return arg when ast matches ('sym, arg), else NULL.
static lclValuePtr starts_with(const lclValuePtr ast, const char* sym)
{
    const lclList* list = DYNAMIC_CAST(lclList, ast);
    if (!list || list->isEmpty() || !isSymbol(list->item(0), sym))
        return NULL;
    checkArgsIs(sym, 1, list->count() - 1);
    return list->item(1);
}

static lclValuePtr quasiquote(lclValuePtr obj)
{
    if (DYNAMIC_CAST(lclSymbol, obj) || DYNAMIC_CAST(lclHash, obj))
        return lcl::list(lcl::symbol("quote"), obj);

    const lclSequence* seq = DYNAMIC_CAST(lclSequence, obj);
    if (!seq)
        return obj;

    const lclValuePtr unquoted = starts_with(obj, "unquote");
    if (unquoted)
        return unquoted;

    lclValuePtr res = lcl::list(new lclValueVec(0));
    for (int i=seq->count()-1; 0<=i; i--) {
        const lclValuePtr elt     = seq->item(i);
        const lclValuePtr spl_unq = starts_with(elt, "splice-unquote");
        if (spl_unq)
            res = lcl::list(lcl::symbol("concat"), spl_unq, res);
         else
            res = lcl::list(lcl::symbol("cons"), quasiquote(elt), res);
    }
    if (DYNAMIC_CAST(lclVector, obj))
        res = lcl::list(lcl::symbol("vec"), res);
    return res;
}

static const char* lclFunctionTable[] = {
    "(defmacro! cond (fn* (& xs) (if (> (count xs) 0) (list 'if (first xs) (if (> (count xs) 1) (nth xs 1) (throw \"odd number of forms to cond\")) (cons 'cond (rest (rest xs)))))))",
    "(defmacro! 2+ (fn* (zahl)(+ zahl 2)))",
    "(def! load-file (fn* (filename) \
        (eval (read-string (str \"(do \" (slurp filename) \"\nnil)\")))))",
    "(def! *host-language* \"C++\")",
    "(def! append concat)",
    "(def! length count)",
    "(def! load load-file)",
    "(def! strcat str)",
    "(def! type type?)",
    "(def! EOF -1)"
};

static void installFunctions(lclEnvPtr env) {
    for (auto &function : lclFunctionTable) {
        rep(function, env);
    }
}

static void installEvalCore(lclEnvPtr env) {
    for (auto &function : lclEvalFunctionTable) {
        env->set(function, lcl::builtin(true, function));
    }
}

void openTile(const lclGui* tile, const child_config_t child_cfg)
{
    qDebug() << "[openTile] start Name: " << tile->value().name.c_str();
    //qDebug() << "[openTile] start Id: " << (int)tile->value().id;

    int dlgId = tile->value().dialog_Id;

    switch (tile->value().id) {
        case DIALOG:
        {
            /* FIXME initial_focus */
            //const lclDialog* dlg = static_cast<const lclDialog*>(tile);
            qDebug() << "[openTile] DIALOG init";
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_dcl_result", lcl::integer(0));
            dclTiles.push_back(tile);
        }
            break;
        case EDIT_BOX:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclEdit* edit = static_cast<const lclEdit*>(tile);
            dclTiles.push_back(tile);
            if (tile->value().key != "")
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::nilValue());
            }
            if (tile->value().action != "")
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::string(tile->value().action));
            }
            if (!edit->value().has_parent)
            {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addLayout(edit->hlayout());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size()-1; i >= 0 ; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addLayout(edit->hlayout());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addLayout(edit->hlayout());
                        }
                        break;
                    }
                }
            }

            if (child_cfg.children_alignment != NOPOS)
            {
                switch (child_cfg.children_alignment)
                {
                case LEFT:
                    edit->edit()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    edit->edit()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    edit->edit()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    edit->edit()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    edit->edit()->setAlignment(Qt::AlignCenter);
                    break;
                case HORIZONTAL:
                    edit->edit()->setAlignment(Qt::AlignHCenter);
                    break;
                case VERTICAL:
                    edit->edit()->setAlignment(Qt::AlignVCenter);
                    break;
                default:
                    break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(edit->value().width)) {
                    edit->edit()->setMaximumWidth(int(edit->value().width));
                }
                else
                {
                    edit->edit()->setMaximumWidth(edit->edit()->width());
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(edit->value().height))
                {
                    edit->edit()->setMaximumHeight(int(edit->value().height));
                }
                else
                {
                    edit->edit()->setMaximumHeight(edit->edit()->height());
                }
            }

            if(noQuotes(edit->value().mnemonic).size())
            {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        qDebug() << "[openTile] (mnemonic) edit:" << noQuotes(edit->value().key).c_str();

                        const lclDialog* d = static_cast<const lclDialog*>(dlg);
                        QShortcut *shortcut = new QShortcut(QKeySequence(noQuotes(edit->value().mnemonic).c_str()), d->dialog());
                        QObject::connect(shortcut, &QShortcut::activated, edit->edit(), [=]() { edit->edit()->setFocus(); });
                        break;
                    }
                }
            }

            if(edit->value().allow_accept)
            {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
#if 0
                        const lclDialog* d = static_cast<const lclDialog*>(dlg);
                        QShortcut *shortcut = new QShortcut(Qt::Key_Return|Qt::Key_Enter, d->dialog());
                        QObject::connect(shortcut, &QShortcut::activated, edit->value(), [&] { edit->value()->clicked(false); });
#endif
                        break;
                    }
                }
            }
        }
            break;
        case LIST_BOX:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclListBox* list = static_cast<const lclListBox*>(tile);
            dclTiles.push_back(tile);
            if (tile->value().key != "")
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::nilValue());
            }
            if (tile->value().action != "")
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::string(tile->value().action));
            }
            if (!list->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addLayout(list->hlayout());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addLayout(list->hlayout());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addLayout(list->hlayout());
                        }
                        break;
                    }
                }
            }
            if (child_cfg.children_alignment != NOPOS)
            {
                switch (child_cfg.children_alignment)
                {
                case LEFT:
                    list->hlayout()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    list->hlayout()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    list->vlayout()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    list->vlayout()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    list->vlayout()->setAlignment(Qt::AlignVCenter);
                    list->hlayout()->setAlignment(Qt::AlignHCenter);
                case HORIZONTAL:
                    list->hlayout()->setAlignment(Qt::AlignHCenter);
                    break;
                case VERTICAL:
                    list->vlayout()->setAlignment(Qt::AlignVCenter);
                default: {}
                    break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(list->value().width)) {
                    list->list()->setMaximumWidth(int(list->value().width * 10));
                }
                else
                {
                    list->list()->setMaximumWidth(list->list()->width());
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(list->value().height))
                {
                    list->list()->setMaximumHeight(int(list->value().height * 10));
                }
                else
                {
                    list->list()->setMaximumHeight(list->list()->height());
                }
            }

            if(noQuotes(list->value().mnemonic).size())
            {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        qDebug() << "[openTile] (mnemonic) edit:" << noQuotes(list->value().key).c_str();

                        const lclDialog* d = static_cast<const lclDialog*>(dlg);
                        QShortcut *shortcut = new QShortcut(QKeySequence(noQuotes(list->value().mnemonic).c_str()), d->dialog());
                        QObject::connect(shortcut, &QShortcut::activated, list->list(), [=]() { list->list()->setFocus(); });
                        break;
                    }
                }
            }

            if(list->value().allow_accept)
            {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
#if 0
                        const lclDialog* d = static_cast<const lclDialog*>(dlg);
                        QShortcut *shortcut = new QShortcut(Qt::Key_Return|Qt::Key_Enter, d->dialog());
                        QObject::connect(shortcut, &QShortcut::activated, list->value(), [&] {list->value()->clicked(false); });
#endif
                        break;
                    }
                }
            }
        }
            break;
        case ROW:
        case RADIO_ROW:
        case CONCATENATION:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclRow* r = static_cast<const lclRow*>(tile);
            dclTiles.push_back(tile);
            if (!r->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addWidget(r->widget());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addWidget(r->widget());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addWidget(r->widget());
                        }
                        break;
                    }
                }
            }
        }
            break;
        case BOXED_ROW:
        case BOXED_RADIO_ROW:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclBoxedRow* br = static_cast<const lclBoxedRow*>(tile);
            dclTiles.push_back(tile);
            if (!br->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addWidget(br->groupbox());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addWidget(br->groupbox());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addWidget(br->groupbox());
                        }
                        break;
                    }
                }
            }
            if (child_cfg.children_alignment != NOPOS)
            {
                switch(child_cfg.children_alignment)
                {
                case LEFT:
                    br->groupbox()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    br->groupbox()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    br->groupbox()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    br->groupbox()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    br->groupbox()->setAlignment(Qt::AlignCenter);
                    break;
                case HORIZONTAL:
                    br->groupbox()->setAlignment(Qt::AlignHCenter);
                    break;
                case VERTICAL:
                    br->groupbox()->setAlignment(Qt::AlignVCenter);
                    break;
                default: {}
                break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(br->value().width)) {
                    br->groupbox()->setMaximumWidth(int(br->value().width));
                }
                else
                {
                    br->groupbox()->setMaximumWidth(br->groupbox()->width());
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(br->value().height))
                {
                    br->groupbox()->setMaximumHeight(int(br->value().height));
                }
                else
                {
                    br->groupbox()->setMaximumHeight(br->groupbox()->height());
                }
            }
        }
            break;
        case COLUMN:
        case PARAGRAPH:
        case RADIO_COLUMN:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclColumn* c = static_cast<const lclColumn*>(tile);
            dclTiles.push_back(tile);
            if (!c->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addWidget(c->widget());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addWidget(c->widget());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addWidget(c->widget());
                        }
                        break;
                    }
                }
            }
        }
            break;
        case BOXED_COLUMN:
        case BOXED_RADIO_COLUMN:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclBoxedColumn* bc = static_cast<const lclBoxedColumn*>(tile);
            dclTiles.push_back(tile);
            if (!bc->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addWidget(bc->groupbox());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addWidget(bc->groupbox());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addWidget(bc->groupbox());
                        }
                        break;
                    }
                }
            }
            if (child_cfg.children_alignment != NOPOS)
            {
                switch(child_cfg.children_alignment)
                {
                case LEFT:
                    bc->groupbox()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    bc->groupbox()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    bc->groupbox()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    bc->groupbox()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    bc->groupbox()->setAlignment(Qt::AlignCenter);
                    break;
                case HORIZONTAL:
                    bc->groupbox()->setAlignment(Qt::AlignHCenter);
                    break;
                case VERTICAL:
                    bc->groupbox()->setAlignment(Qt::AlignVCenter);
                    break;
                default: {}
                    break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(bc->value().width)) {
                    bc->groupbox()->setMaximumWidth(int(bc->value().width));
                }
                else
                {
                    bc->groupbox()->setMaximumWidth(bc->groupbox()->width());
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(bc->value().height))
                {
                    bc->groupbox()->setMaximumHeight(int(bc->value().height));
                }
                else
                {
                    bc->groupbox()->setMaximumHeight(bc->groupbox()->height());
                }
            }
        }
            break;
        case TEXT:
        case TEXT_PART:
        case ERRTILE:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclLabel* l = static_cast<const lclLabel*>(tile);
            dclTiles.push_back(tile);
            if (!l->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addWidget(l->label());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addWidget(l->label());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addWidget(l->label());
                        }
                        break;
                    }
                }
            }
            if(tile->value().id == ERRTILE)
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_error", lcl::nilValue());
            }

            if (child_cfg.children_alignment != NOPOS)
            {
                switch(child_cfg.children_alignment)
                {
                case LEFT:
                    l->label()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    l->label()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    l->label()->setMinimumHeight(int(l->value().height * 1.5));
                    l->label()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    l->label()->setMinimumHeight(int(l->value().height * 1.5));
                    l->label()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    l->label()->setAlignment(Qt::AlignCenter);
                case HORIZONTAL:
                    l->label()->setAlignment(Qt::AlignHCenter);
                    break;
                case VERTICAL:
                    l->label()->setAlignment(Qt::AlignVCenter);
                    break;
                default: {}
                    break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(l->value().width)) {
                    l->label()->setMaximumWidth(int(l->value().width));
                }
                else
                {
                    l->label()->setMaximumWidth(l->label()->width());
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(l->value().height))
                {
                    l->label()->setMaximumHeight(int(l->value().height));
                }
                else
                {
                    l->label()->setMaximumHeight(l->label()->height());
                }
            }
        }
            break;
        case BUTTON:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclButton* b = static_cast<const lclButton*>(tile);
            dclTiles.push_back(tile);
            if (tile->value().key != "")
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::nilValue());
            }
            if (tile->value().action != "")
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::string(tile->value().action));
            }

            if (!b->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addLayout(b->hlayout());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addLayout(b->hlayout());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addLayout(b->hlayout());
                        }
                        break;
                    }
                }
            }

            if (child_cfg.children_alignment != NOPOS)
            {
                switch (child_cfg.children_alignment)
                {
                case LEFT:
                    b->hlayout()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    b->hlayout()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    b->vlayout()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    b->vlayout()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    b->vlayout()->setAlignment(Qt::AlignVCenter);
                    b->hlayout()->setAlignment(Qt::AlignHCenter);
                case HORIZONTAL:
                    b->hlayout()->setAlignment(Qt::AlignHCenter);
                    break;
                case VERTICAL:
                    b->vlayout()->setAlignment(Qt::AlignVCenter);
                    break;
                default: {}
                break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(b->value().width)) {
                    b->button()->setMaximumWidth(int(b->value().width * 10));
                }
                else
                {
                    b->button()->setMaximumWidth(80);
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(b->value().height))
                {
                    b->button()->setMaximumHeight(int(b->value().height * 10));
                }
                else
                {
                    b->button()->setMaximumHeight(32);
                }
            }
        }
            break;
        case OK_ONLY:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclButton* b = static_cast<const lclButton*>(tile);
            dclTiles.push_back(tile);
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::nilValue());

            if (!b->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addWidget(b->button());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addWidget(b->button());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addWidget(b->button());
                        }
                        break;
                    }
                }
            }

            if (child_cfg.children_alignment != NOPOS)
            {
                switch (child_cfg.children_alignment)
                {
                case LEFT:
                    b->hlayout()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    b->hlayout()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    b->vlayout()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    b->vlayout()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    b->vlayout()->setAlignment(Qt::AlignVCenter);
                    b->hlayout()->setAlignment(Qt::AlignHCenter);
                case HORIZONTAL:
                    b->hlayout()->setAlignment(Qt::AlignHCenter);
                    break;
                case VERTICAL:
                    b->vlayout()->setAlignment(Qt::AlignVCenter);
                default: {}
                    break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(b->value().width)) {
                    b->button()->setMaximumWidth(int(b->value().width * 10));
                }
                else
                {
                    b->button()->setMaximumWidth(80);
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(b->value().height))
                {
                    b->button()->setMaximumHeight(int(b->value().height * 10));
                }
                else
                {
                    b->button()->setMaximumHeight(32);
                }
            }
        }
            break;
        case POPUP_LIST:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclPopupList* lp = static_cast<const lclPopupList*>(tile);
            dclTiles.push_back(tile);
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::nilValue());

            if (!lp->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addLayout(lp->hlayout());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addLayout(lp->hlayout());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addLayout(lp->hlayout());
                        }
                        break;
                    }
                }
            }
            if (child_cfg.children_alignment != NOPOS)
            {
                switch (child_cfg.children_alignment)
                {
                case LEFT:
                    lp->hlayout()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    lp->hlayout()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    lp->vlayout()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    lp->vlayout()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    lp->vlayout()->setAlignment(Qt::AlignVCenter);
                    lp->hlayout()->setAlignment(Qt::AlignHCenter);
                case HORIZONTAL:
                    lp->hlayout()->setAlignment(Qt::AlignHCenter);
                    break;
                case VERTICAL:
                    lp->vlayout()->setAlignment(Qt::AlignVCenter);
                    break;
                default: {}
                break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(lp->value().width)) {
                    lp->list()->setMaximumWidth(int(lp->value().width * 10));
                }
                else
                {
                    lp->list()->setMaximumWidth(lp->list()->width());
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(lp->value().height))
                {
                    lp->list()->setMaximumHeight(int(lp->value().height * 10));
                }
                else
                {
                    lp->list()->setMaximumHeight(lp->list()->height());
                }
            }

            if(noQuotes(lp->value().mnemonic).size())
            {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        qDebug() << "[openTile] (mnemonic) edit:" << noQuotes(lp->value().key).c_str();

                        const lclDialog* d = static_cast<const lclDialog*>(dlg);
                        QShortcut *shortcut = new QShortcut(QKeySequence(noQuotes(lp->value().mnemonic).c_str()), d->dialog());
                        QObject::connect(shortcut, &QShortcut::activated, lp->list(), [=]() { lp->list()->setFocus(); });
                        break;
                    }
                }
            }
        }
            break;
        case RADIO_BUTTON:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclRadioButton* rb = static_cast<const lclRadioButton*>(tile);
            dclTiles.push_back(tile);
            if (tile->value().key != "")
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::nilValue());
            }

            if (!rb->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addLayout(rb->hlayout());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {

                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addLayout(rb->hlayout());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addLayout(rb->hlayout());
                        }
                        break;
                    }
                }
            }

            if (child_cfg.children_alignment != NOPOS)
            {
                switch (child_cfg.children_alignment)
                {
                case LEFT:
                    rb->hlayout()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    rb->hlayout()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    rb->vlayout()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    rb->vlayout()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    rb->vlayout()->setAlignment(Qt::AlignVCenter);
                    rb->hlayout()->setAlignment(Qt::AlignHCenter);
                case HORIZONTAL:
                    rb->hlayout()->setAlignment(Qt::AlignHCenter);
                    break;
                case VERTICAL:
                    rb->vlayout()->setAlignment(Qt::AlignVCenter);
                    break;
                default: {}
                break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(rb->value().width)) {
                    rb->button()->setMaximumWidth(int(rb->value().width * 10));
                }
                else
                {
                    rb->button()->setMaximumWidth(80);
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(rb->value().height))
                {
                    rb->button()->setMaximumHeight(int(rb->value().height * 10));
                }
                else
                {
                    rb->button()->setMaximumHeight(32);
                }
            }
        }
            break;
        case IMAGE:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclImage* img = static_cast<const lclImage*>(tile);
            dclTiles.push_back(tile);
            if (!img->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addWidget(img->image());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addWidget(img->image());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addWidget(img->image());
                        }
                        break;
                    }
                }
            }
            if (child_cfg.children_alignment != NOPOS)
            {
                switch(child_cfg.children_alignment)
                {
                case LEFT:
                    img->image()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    img->image()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    img->image()->setMinimumHeight(int(img->value().height * 1.5));
                    img->image()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    img->image()->setMinimumHeight(int(img->value().height * 1.5));
                    img->image()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    img->image()->setAlignment(Qt::AlignCenter);
                    break;
                default: {}
                    break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(img->value().width)) {
                    img->image()->setMaximumWidth(int(img->value().width));
                }
                else
                {
                    img->image()->setMaximumWidth(img->image()->width());
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(img->value().height))
                {
                    img->image()->setMaximumHeight(int(img->value().height));
                }
                else
                {
                    img->image()->setMaximumHeight(img->image()->height());
                }
            }
        }
            break;
        case IMAGE_BUTTON:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclImageButton* b = static_cast<const lclImageButton*>(tile);
            dclTiles.push_back(tile);
            if (tile->value().key != "")
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::nilValue());
            }
            if (tile->value().action != "")
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::string(tile->value().action));
            }

            if (!b->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addLayout(b->hlayout());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addLayout(b->hlayout());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addLayout(b->hlayout());
                        }
                        break;
                    }
                }
            }
            if (child_cfg.children_alignment != NOPOS)
            {
                switch (child_cfg.children_alignment)
                {
                case LEFT:
                    b->hlayout()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    b->hlayout()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    b->vlayout()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    b->vlayout()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    b->vlayout()->setAlignment(Qt::AlignVCenter);
                    b->hlayout()->setAlignment(Qt::AlignHCenter);
                case HORIZONTAL:
                    b->hlayout()->setAlignment(Qt::AlignHCenter);
                    break;
                case VERTICAL:
                    b->vlayout()->setAlignment(Qt::AlignVCenter);
                    break;
                default: {}
                break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(b->value().width)) {
                    b->button()->setMaximumWidth(int(b->value().width * 10));
                }
                else
                {
                    b->button()->setMaximumWidth(80);
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(b->value().height))
                {
                    b->button()->setMaximumHeight(int(b->value().height * 10));
                }
                else
                {
                    b->button()->setMaximumHeight(32);
                }
            }

            if(b->value().allow_accept && !b->value().is_default)
            {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        const lclDialog* d = static_cast<const lclDialog*>(dlg);
                        QShortcut *shortcut = new QShortcut(Qt::Key_Return|Qt::Key_Enter, d->dialog());
                        QObject::connect(shortcut, &QShortcut::activated, b->button(), [&] { b->button()->clicked(false); });
                        break;
                    }
                }
            }
        }
            break;
        case REGISTER:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclTabWidget* r = static_cast<const lclTabWidget*>(tile);
            dclTiles.push_back(tile);

            lclValueVec* tiles = new lclValueVec(tile->value().tiles->size());
            std::copy(tile->value().tiles->begin(), tile->value().tiles->end(), tiles->begin());

            for (auto it = tiles->begin(), end = tiles->end(); it != end; it++) {
                const lclGui* child = DYNAMIC_CAST(lclGui, *it);
                if (child->value().id == TAB)
                {
                    const lclWidget* tab = static_cast<const lclWidget*>(child);
                    dclTiles.push_back(child);
                    r->widget()->addTab(tab->widget(), noQuotes(child->value().label).c_str());
                    if (noQuotes(child->value().value) == "1")
                    {
                        r->widget()->setCurrentWidget(tab->widget());
                    }
                    openTile(child);
                }
            }

            //dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::nilValue());

            for (auto & dlg : dclTiles)
            {
                if (dlg->value().dialog_Id == dlgId)
                {
                    dlg->vlayout()->addWidget(r->widget());
                    break;
                }
            }
            return;
        }
            break;
        case TAB:
        {
        }
            break;
        case SLIDER:
        case SCROLL:
        case DIAL:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclSlider* sl = static_cast<const lclSlider*>(tile);
            dclTiles.push_back(tile);
            if (tile->value().key != "")
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::nilValue());
            }
            if (tile->value().action != "")
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::string(tile->value().action));
            }
            if (!sl->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addLayout(sl->hlayout());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addLayout(sl->hlayout());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addLayout(sl->hlayout());
                        }
                        break;
                    }
                }
            }
            if (child_cfg.children_alignment != NOPOS)
            {
                switch (child_cfg.children_alignment)
                {
                case LEFT:
                    sl->hlayout()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    sl->hlayout()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    sl->vlayout()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    sl->vlayout()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    sl->vlayout()->setAlignment(Qt::AlignVCenter);
                    sl->hlayout()->setAlignment(Qt::AlignHCenter);
                case HORIZONTAL:
                    sl->hlayout()->setAlignment(Qt::AlignHCenter);
                    break;
                case VERTICAL:
                    sl->vlayout()->setAlignment(Qt::AlignVCenter);
                    break;
                default: {}
                    break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(sl->value().width)) {
                    sl->slider()->setMaximumWidth(int(sl->value().width * 10));
                }
                else
                {
                    sl->slider()->setMaximumWidth(sl->slider()->width());
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(sl->value().height))
                {
                    sl->slider()->setMaximumHeight(int(sl->value().height * 10));
                }
                else
                {
                    sl->slider()->setMaximumHeight(sl->slider()->height());
                }
            }

            if(noQuotes(sl->value().mnemonic).size())
            {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        qDebug() << "[openTile] (mnemonic) edit:" << noQuotes(sl->value().key).c_str();

                        const lclDialog* d = static_cast<const lclDialog*>(dlg);
                        QShortcut *shortcut = new QShortcut(QKeySequence(noQuotes(sl->value().mnemonic).c_str()), d->dialog());
                        QObject::connect(shortcut, &QShortcut::activated, sl->slider(), [=]() { sl->slider()->setFocus(); });
                        break;
                    }
                }
            }
        }
            break;
        case SPACER:
        case SPACER_0:
        case SPACER_1:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclSpacer* s = static_cast<const lclSpacer*>(tile);
            dclTiles.push_back(tile);
            if (!s->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addSpacerItem(s->spacer());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addSpacerItem(s->spacer());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addSpacerItem(s->spacer());
                        }
                        break;
                    }
                }
            }
            if (child_cfg.children_alignment != NOPOS)
            {
                switch (child_cfg.children_alignment)
                {
                case LEFT:
                    s->spacer()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    s->spacer()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    s->spacer()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    s->spacer()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    s->spacer()->setAlignment(Qt::AlignCenter);
                    break;
                case HORIZONTAL:
                    s->spacer()->setAlignment(Qt::AlignHCenter);
                    break;
                case VERTICAL:
                    s->spacer()->setAlignment(Qt::AlignVCenter);
                    break;
                default:
                    break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(s->value().width)) {
                    s->spacer()->widget()->setMaximumWidth(int(s->value().width * 10));
                }
                else
                {
                    s->spacer()->widget()->setMaximumWidth(80);
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(s->value().height))
                {
                    s->spacer()->widget()->setMaximumHeight(int(s->value().height * 10));
                }
                else
                {
                    s->spacer()->widget()->setMaximumHeight(32);
                }
            }
        }
            break;
        case TOGGLE:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclToggle* tb = static_cast<const lclToggle*>(tile);
            dclTiles.push_back(tile);
            if (tile->value().key != "")
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::nilValue());
            }
            if (tile->value().action != "")
            {
                dclEnv->set(std::to_string(tile->value().dialog_Id) + "_" + noQuotes(tile->value().key).c_str(), lcl::string(tile->value().action));
            }

            if (!tb->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addLayout(tb->hlayout());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addLayout(tb->hlayout());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addLayout(tb->hlayout());
                        }
                        break;
                    }
                }
            }
            if (child_cfg.children_alignment != NOPOS)
            {
                switch (child_cfg.children_alignment)
                {
                case LEFT:
                    tb->hlayout()->setAlignment(Qt::AlignLeft);
                    break;
                case RIGHT:
                    tb->hlayout()->setAlignment(Qt::AlignRight);
                    break;
                case TOP:
                    tb->vlayout()->setAlignment(Qt::AlignTop);
                    break;
                case BOTTOM:
                    tb->vlayout()->setAlignment(Qt::AlignBottom);
                    break;
                case CENTERED:
                    tb->vlayout()->setAlignment(Qt::AlignVCenter);
                    tb->hlayout()->setAlignment(Qt::AlignHCenter);
                case HORIZONTAL:
                    tb->hlayout()->setAlignment(Qt::AlignHCenter);
                    break;
                case VERTICAL:
                    tb->vlayout()->setAlignment(Qt::AlignVCenter);
                default: {}
                    break;
                }
            }

            if (child_cfg.children_fixed_width)
            {
                if(int(tb->value().width)) {
                    tb->toggle()->setMaximumWidth(int(tb->value().width * 10));
                }
                else
                {
                    tb->toggle()->setMaximumWidth(80);
                }
            }

            if (child_cfg.children_fixed_height)
            {
                if(int(tb->value().height))
                {
                    tb->toggle()->setMaximumHeight(int(tb->value().height * 10));
                }
                else
                {
                    tb->toggle()->setMaximumHeight(32);
                }
            }
        }
            break;
        case OK_CANCEL:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclOkCancel* okc = static_cast<const lclOkCancel*>(tile);
            dclTiles.push_back(tile);
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_accept", lcl::nilValue());
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_cancel", lcl::nilValue());
            if (!okc->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addLayout(okc->hlayout());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addLayout(okc->hlayout());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addLayout(okc->hlayout());
                        }
                        break;
                    }
                }
            }
        }
            break;
        case OK_CANCEL_HELP:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclOkCancelHelp* okch = static_cast<const lclOkCancelHelp*>(tile);
            dclTiles.push_back(tile);
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_accept", lcl::nilValue());
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_cancel", lcl::nilValue());
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_help", lcl::nilValue());
            if (!okch->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addLayout(okch->hlayout());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                             dclTiles.at(i)->hlayout()->addLayout(okch->hlayout());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addLayout(okch->hlayout());
                        }
                        break;
                    }
                }
            }
        }
            break;
        case OK_CANCEL_HELP_INFO:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclOkCancelHelpInfo* okchi = static_cast<const lclOkCancelHelpInfo*>(tile);
            dclTiles.push_back(tile);
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_accept", lcl::nilValue());
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_cancel", lcl::nilValue());
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_help", lcl::nilValue());
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_info", lcl::nilValue());
            if (!okchi->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addLayout(okchi->hlayout());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addLayout(okchi->hlayout());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addLayout(okchi->hlayout());
                        }
                        break;
                    }
                }
            }
        }
            break;
        case OK_CANCEL_HELP_ERRTILE:
        {
            if (!dclTiles.size())
            {
                break;
            }
            const lclOkCancelHelpErrtile* okche = static_cast<const lclOkCancelHelpErrtile*>(tile);
            dclTiles.push_back(tile);
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_accept", lcl::nilValue());
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_cancel", lcl::nilValue());
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_help", lcl::nilValue());
            dclEnv->set(std::to_string(tile->value().dialog_Id) + "_error", lcl::nilValue());

            if (!okche->value().has_parent) {
                for (auto & dlg : dclTiles)
                {
                    if (dlg->value().dialog_Id == dlgId)
                    {
                        dlg->vlayout()->addLayout(okche->vlayout());
                        break;
                    }
                }
            }
            else
            {
                for (int i = dclTiles.size() - 2; i >= 0; i--)
                {
                    if(LAYOUT_TILE & dclTiles.at(i)->value().id)
                    {
                        if (LAYOUT_ROW & dclTiles.at(i)->value().id)
                        {
                            dclTiles.at(i)->hlayout()->addLayout(okche->vlayout());
                        }
                        else
                        {
                            dclTiles.at(i)->vlayout()->addLayout(okche->vlayout());
                        }
                        break;
                    }
                }
            }
        }
            break;
        default:
            break;
    }

    child_config_t cfg = { false, false, NOPOS };

    if(tile->value().id & LAYOUT_PARENT_TILE)
    {
        cfg.children_fixed_height = tile->value().children_fixed_height;
        cfg.children_fixed_width = tile->value().children_fixed_width;
        cfg.children_alignment = tile->value().children_alignment;
    }

    lclValueVec* tiles = new lclValueVec(tile->value().tiles->size());
    std::copy(tile->value().tiles->begin(), tile->value().tiles->end(), tiles->begin());

    for (auto it = tiles->begin(), end = tiles->end(); it != end; it++)
    {
        const lclGui* tile = DYNAMIC_CAST(lclGui, *it);
        qDebug() << "[openTile] end: found: " << tile->value().name.c_str();
        openTile(tile, cfg);
    }
}

String strToUpper(String s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::toupper(c); } // correct
                  );
    return s;
}

String noQuotes(const String& s)
{
    return s.size() >= 2 ? s.substr(1, s.size() - 2) : "";
}

static void checkForAlias(const String& com)
{
    if (com.size() < 3 ||
        std::toupper(com[0]) != 'C' ||  // correct
        com[1] != ':')
    {
        return;
    }

    String alias = com.substr(2);

    for (std::vector<LclAlias_t>::iterator it = LclCom.begin(); it != LclCom.end();)
    {
        if (it->alias == alias)
            it = LclCom.erase(it);
        else
            ++it;
    }

    LclCom.push_back({ { alias }, { com } });
}

bool isAlias(const String& alias)
{
    for (std::vector<LclAlias_t>::iterator it = LclCom.begin(); it != LclCom.end();)
    {
        if (it->alias == alias)
        {
            return true;
        }
        ++it;
    }
    return false;
}

#endif // DEVELOPER
