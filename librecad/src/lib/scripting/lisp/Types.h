
#ifndef INCLUDE_TYPES_H
#define INCLUDE_TYPES_H

#include "LCL.h"

#include <exception>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <iostream>

#include <QWidget>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QAbstractButton>
#include <QRadioButton>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QScrollBar>
#include <QDial>
#include <QCheckBox>
#include <QSpacerItem>
#include <QColor>
#include <QPainter>
#include <QProxyStyle>

#ifdef DEVELOPER

class lclEmptyInputException : public std::exception { };

enum class LCLTYPE { ATOM, BUILTIN, BOOLEAN, FILE, GUI, INT, LIST, MAP, REAL, STR, SYM, UNDEF, VEC, KEYW };

#define MAX_DCL_TILES 35
#define MAX_DCL_ATTR 35
#define MAX_DCL_POS 8
#define MAX_DCL_COLOR 257
#define MAX_DCL_ICONS 18

inline const char * boolToString(bool b)
{
    return b ? "true" : "false";
}

class lclValue : public RefCounted {
public:
    lclValue() {
        TRACE_OBJECT("Creating lclValue %p\n", this);
    }
    lclValue(lclValuePtr meta) : m_meta(meta) {
        TRACE_OBJECT("Creating lclValue %p\n", this);
    }
    virtual ~lclValue() {
        TRACE_OBJECT("Destroying lclValue %p\n", this);
    }

    lclValuePtr withMeta(lclValuePtr meta) const;
    virtual lclValuePtr doWithMeta(lclValuePtr meta) const = 0;
    lclValuePtr meta() const;

    bool isTrue() const;

    bool isEqualTo(const lclValue* rhs) const;

    virtual lclValuePtr eval(lclEnvPtr env);

    virtual String print(bool readably) const = 0;

    virtual LCLTYPE type() const { return LCLTYPE::UNDEF; }

protected:
    virtual bool doIsEqualTo(const lclValue* rhs) const = 0;

    lclValuePtr m_meta;
};

template<class T>
T* value_cast(lclValuePtr obj, const char* typeName) {
    T* dest = dynamic_cast<T*>(obj.ptr());
    LCL_CHECK(dest != NULL, "'%s' is not a %s",
              obj->print(true).c_str(), typeName);
    return dest;
}

#define VALUE_CAST(Type, Value)    value_cast<Type>(Value, #Type)
#define DYNAMIC_CAST(Type, Value)  (dynamic_cast<Type*>((Value).ptr()))
#define STATIC_CAST(Type, Value)   (static_cast<Type*>((Value).ptr()))

#define WITH_META(Type) \
    virtual lclValuePtr doWithMeta(lclValuePtr meta) const override { \
        return new Type(*this, meta); \
    } \

class lclConstant : public lclValue {
public:
    lclConstant(String name) : m_name(name) { }
    lclConstant(const lclConstant& that, lclValuePtr meta)
        : lclValue(meta), m_name(that.m_name) { }

    virtual String print(bool) const override { return m_name; }

    virtual LCLTYPE type() const override {
        if ((m_name.compare("false") ||
            (m_name.compare("true")))) {
            return LCLTYPE::BOOLEAN; }
        else {
            return LCLTYPE::UNDEF; }
    }

    virtual bool doIsEqualTo(const lclValue* rhs) const override {
        return this == rhs; // these are singletons
    }

    WITH_META(lclConstant)

private:
    const String m_name;
};

class lclInteger : public lclValue {
public:
    lclInteger(int64_t value) : m_value(value) { }
    lclInteger(const lclInteger& that, lclValuePtr meta)
        : lclValue(meta), m_value(that.m_value) { }

    virtual String print(bool) const override {
        return std::to_string(m_value);
    }

    virtual LCLTYPE type() const override { return LCLTYPE::INT; }

    int64_t value() const { return m_value; }

    virtual bool doIsEqualTo(const lclValue* rhs) const override;

    WITH_META(lclInteger)

private:
    const int64_t m_value;
};

class lclDouble : public lclValue {
public:
    lclDouble(double value) : m_value(value) { }
    lclDouble(const lclDouble& that, lclValuePtr meta)
        : lclValue(meta), m_value(that.m_value) { }

    virtual String print(bool) const override {
        return std::to_string(m_value);
    }

    virtual bool isFloat() const { return true; }
    virtual LCLTYPE type() const override { return LCLTYPE::REAL; }

    double value() const { return m_value; }

    virtual bool doIsEqualTo(const lclValue* rhs) const override;

    WITH_META(lclDouble)

private:
    const double m_value;
};

class lclFile : public lclValue {
public:
    lclFile(const char *path, const char &mode)
        : m_path(path)
        , m_mode(mode)
    {
    }
    lclFile(const lclFile& that, lclValuePtr meta)
        : lclValue(meta), m_value(that.m_value) { }

    virtual String print(bool) const override {
        String path = "#<file \"";
        path += m_path;
        path += "\">";
        return path;
    }

    virtual LCLTYPE type() const override { return LCLTYPE::FILE; }

    virtual bool doIsEqualTo(const lclValue* rhs) const override {
        return value() == static_cast<const lclFile*>(rhs)->value();
    }

    ::FILE *value() const { return m_value; }

    WITH_META(lclFile)

    lclValuePtr close();
    lclValuePtr open();
    lclValuePtr readLine();
    lclValuePtr readChar();
    lclValuePtr writeChar(const char &c);
    lclValuePtr writeLine(const String &line);

private:
    String m_path;
    ::FILE *m_value;
    char m_mode;
};

class lclStringBase : public lclValue {
public:
    lclStringBase(const String& token)
        : m_value(token) { }
    lclStringBase(const lclStringBase& that, lclValuePtr meta)
        : lclValue(meta), m_value(that.value()) { }

    virtual String print(bool) const override { return m_value; }

    String value() const { return m_value; }

private:
    const String m_value;
};

class lclString : public lclStringBase {
public:
    lclString(const String& token)
        : lclStringBase(token) { }
    lclString(const lclString& that, lclValuePtr meta)
        : lclStringBase(that, meta) { }

    virtual String print(bool readably) const override;
    virtual LCLTYPE type() const override { return LCLTYPE::STR; }

    String escapedValue() const;

    virtual bool doIsEqualTo(const lclValue* rhs) const override {
        return value() == static_cast<const lclString*>(rhs)->value();
    }

    WITH_META(lclString)
};

class lclKeyword : public lclStringBase {
public:
    lclKeyword(const String& token)
        : lclStringBase(token) { }
    lclKeyword(const lclKeyword& that, lclValuePtr meta)
        : lclStringBase(that, meta) { }

    virtual bool doIsEqualTo(const lclValue* rhs) const override {
        return value() == static_cast<const lclKeyword*>(rhs)->value();
    }

    virtual LCLTYPE type() const override { return LCLTYPE::KEYW; }

    WITH_META(lclKeyword)
};

class lclSymbol : public lclStringBase {
public:
    lclSymbol(const String& token)
        : lclStringBase(token) { }
    lclSymbol(const lclSymbol& that, lclValuePtr meta)
        : lclStringBase(that, meta) { }

    virtual lclValuePtr eval(lclEnvPtr env) override;

    virtual bool doIsEqualTo(const lclValue* rhs) const override {
        return value() == static_cast<const lclSymbol*>(rhs)->value();
    }

    virtual LCLTYPE type() const override { return LCLTYPE::SYM; }

    WITH_META(lclSymbol)
};

class lclSequence : public lclValue {
public:
    lclSequence(lclValueVec* items);
    lclSequence(lclValueIter begin, lclValueIter end);
    lclSequence(const lclSequence& that, lclValuePtr meta);
    virtual ~lclSequence();

    virtual String print(bool readably) const override;

    lclValueVec* evalItems(lclEnvPtr env) const;
    int count() const { return m_items->size(); }
    bool isEmpty() const { return m_items->empty(); }
    bool isDotted() const;
    lclValuePtr item(int index) const { return (*m_items)[index]; }

    lclValueIter begin() const { return m_items->begin(); }
    lclValueIter end()   const { return m_items->end(); }

    virtual bool doIsEqualTo(const lclValue* rhs) const override;

    virtual lclValuePtr conj(lclValueIter argsBegin,
                              lclValueIter argsEnd) const = 0;
#if 0
    virtual lclValuePtr append(lclValueIter argsBegin,
                             lclValueIter argsEnd) const;
#endif
    virtual lclValueVec* append(lclValueIter argsBegin,
                             lclValueIter argsEnd) const;

    lclValuePtr first() const;
    lclValuePtr reverse(lclValueIter argsBegin, lclValueIter argsEnd) const;
    virtual lclValuePtr rest() const;
    virtual lclValuePtr dotted() const;

private:
    lclValueVec* const m_items;
};

class lclList : public lclSequence {
public:
    lclList(lclValueVec* items) : lclSequence(items) { }
    lclList(lclValueIter begin, lclValueIter end)
        : lclSequence(begin, end) { }
    lclList(const lclList& that, lclValuePtr meta)
        : lclSequence(that, meta) { }

    virtual String print(bool readably) const override;
    virtual LCLTYPE type() const override { return LCLTYPE::LIST; }
    virtual lclValuePtr eval(lclEnvPtr env) override;

    virtual lclValuePtr conj(lclValueIter argsBegin,
                             lclValueIter argsEnd) const override;

    WITH_META(lclList)
};

class lclVector : public lclSequence {
public:
    lclVector(lclValueVec* items) : lclSequence(items) { }
    lclVector(lclValueIter begin, lclValueIter end)
        : lclSequence(begin, end) { }
    lclVector(const lclVector& that, lclValuePtr meta)
        : lclSequence(that, meta) { }

    virtual lclValuePtr eval(lclEnvPtr env) override;
    virtual String print(bool readably) const override;
    virtual LCLTYPE type() const override { return LCLTYPE::VEC; }

    virtual lclValuePtr conj(lclValueIter argsBegin,
                             lclValueIter argsEnd) const override;

    WITH_META(lclVector)
};

class lclApplicable : public lclValue {
public:
    lclApplicable() { }
    lclApplicable(lclValuePtr meta) : lclValue(meta) { }

    virtual lclValuePtr apply(lclValueIter argsBegin,
                               lclValueIter argsEnd) const = 0;
};

class lclHash : public lclValue {
public:
    typedef std::map<String, lclValuePtr> Map;

    lclHash(lclValueIter argsBegin, lclValueIter argsEnd, bool isEvaluated);
    lclHash(const lclHash::Map& map);
    lclHash(const lclHash& that, lclValuePtr meta)
    : lclValue(meta), m_map(that.m_map), m_isEvaluated(that.m_isEvaluated) { }

    lclValuePtr assoc(lclValueIter argsBegin, lclValueIter argsEnd) const;
    lclValuePtr dissoc(lclValueIter argsBegin, lclValueIter argsEnd) const;
    bool contains(lclValuePtr key) const;
    lclValuePtr eval(lclEnvPtr env) override;
    lclValuePtr get(lclValuePtr key) const;
    lclValuePtr keys() const;
    lclValuePtr values() const;

    virtual String print(bool readably) const override;

    virtual bool doIsEqualTo(const lclValue* rhs) const override;

    virtual LCLTYPE type() const override { return LCLTYPE::MAP; }

    WITH_META(lclHash)

private:
    const Map m_map;
    const bool m_isEvaluated;
};

class lclBuiltIn : public lclApplicable {
public:
    typedef lclValuePtr (ApplyFunc)(const String& name,
                                    lclValueIter argsBegin,
                                    lclValueIter argsEnd);

    lclBuiltIn(const String& name, ApplyFunc* handler)
    : m_name(name), m_handler(handler) { }

    lclBuiltIn(bool eval, const String& name)
    : m_inEval(eval), m_name(name) { }

    lclBuiltIn(const lclBuiltIn& that, lclValuePtr meta)
    : lclApplicable(meta), m_name(that.m_name), m_handler(that.m_handler) { }

    virtual lclValuePtr apply(lclValueIter argsBegin,
                              lclValueIter argsEnd) const override;

    virtual String print(bool) const override {
        return STRF("#builtin-function(%s)", m_name.c_str());
    }

    virtual bool doIsEqualTo(const lclValue* rhs) const override {
        return this == rhs; // these are singletons
    }

    String name() const { return m_name; }

    virtual LCLTYPE type() const override { return LCLTYPE::BUILTIN; }

    WITH_META(lclBuiltIn)

private:
    [[maybe_unused]] bool m_inEval;
    const String m_name;
    ApplyFunc *m_handler;
};

class lclLambda : public lclApplicable {
public:
    lclLambda(const StringVec& bindings, lclValuePtr body, lclEnvPtr env);
    lclLambda(const lclLambda& that, lclValuePtr meta);
    lclLambda(const lclLambda& that, bool isMacro);

    virtual lclValuePtr apply(lclValueIter argsBegin,
                              lclValueIter argsEnd) const override;

    lclValuePtr getBody() const { return m_body; }
    lclEnvPtr makeEnv(lclValueIter argsBegin, lclValueIter argsEnd) const;

    virtual bool doIsEqualTo(const lclValue* rhs) const override {
        return this == rhs; // do we need to do a deep inspection?
    }

    virtual String print(bool) const override {
        return STRF("#user-%s(%p)", m_isMacro ? "macro" : "function", this);
    }

    bool isMacro() const { return m_isMacro; }

    virtual lclValuePtr doWithMeta(lclValuePtr meta) const override;

private:
    const StringVec   m_bindings;
    const lclValuePtr m_body;
    const lclEnvPtr   m_env;
    const bool        m_isMacro;
};

class lclAtom : public lclValue {
public:
    lclAtom(lclValuePtr value) : m_value(value) { }
    lclAtom(const lclAtom& that, lclValuePtr meta)
        : lclValue(meta), m_value(that.m_value) { }

    virtual bool doIsEqualTo(const lclValue* rhs) const override {
        return this->m_value->isEqualTo(rhs);
    }

    virtual String print(bool readably) const override {
        return "(atom " + m_value->print(readably) + ")";
    };

    virtual LCLTYPE type() const override { return LCLTYPE::ATOM; }

    lclValuePtr deref() const { return m_value; }

    lclValuePtr reset(lclValuePtr value) { return m_value = value; }

    WITH_META(lclAtom)

private:
    lclValuePtr m_value;
};

enum TILE_ID {
    NONE,
    BUTTON,
    DIAL,
    DIALOG,
    EDIT_BOX,
    ERRTILE,
    IMAGE,
    IMAGE_BUTTON,
    LIST_BOX,
    OK_CANCEL,
    OK_CANCEL_HELP,
    OK_CANCEL_HELP_ERRTILE,
    OK_CANCEL_HELP_INFO,
    OK_ONLY,
    POPUP_LIST,
    RADIO_BUTTON,
    REGISTER,
    SCROLL,
    SLIDER,
    SPACER,
    SPACER_0,
    SPACER_1,
    TEXT,
    TEXT_PART,
    TOGGLE,
    RADIO_COLUMN = 32,
    RADIO_ROW = 64,
    BOXED_COLUMN = 128,
    BOXED_RADIO_COLUMN = 256,
    BOXED_RADIO_ROW = 512,
    BOXED_ROW = 1024,
    COLUMN = 2048,
    CONCATENATION = 4096,
    PARAGRAPH = 8192,
    ROW = 16384,
    TAB = 32768
};

#define LAYOUT_TILE (COLUMN | RADIO_COLUMN | BOXED_COLUMN | BOXED_RADIO_COLUMN | ROW | RADIO_ROW | BOXED_ROW | BOXED_RADIO_ROW | CONCATENATION | PARAGRAPH | TAB)
#define LAYOUT_PARENT_TILE (COLUMN | RADIO_COLUMN | BOXED_COLUMN | BOXED_RADIO_COLUMN | ROW | RADIO_ROW | BOXED_ROW | BOXED_RADIO_ROW)
#define LAYOUT_ROW (ROW | RADIO_ROW | BOXED_ROW | BOXED_RADIO_ROW | CONCATENATION)

typedef enum TILE_ID tile_id_t;

typedef struct tile_prop {
    const char* name;
    tile_id_t id;
} tile_prop_t;

static const tile_prop_t dclTile[MAX_DCL_TILES] = {
   { "boxed_column", BOXED_COLUMN },
   { "boxed_radio_column", BOXED_RADIO_COLUMN },
   { "boxed_radio_row", BOXED_RADIO_ROW },
   { "boxed_row",BOXED_ROW },
   { "button", BUTTON },
   { "column", COLUMN },
   { "concatenation", CONCATENATION },
   { "dial", DIAL },
   { "dialog", DIALOG },
   { "edit_box", EDIT_BOX },
   { "errtile", ERRTILE },
   { "image", IMAGE },
   { "image_button", IMAGE_BUTTON },
   { "list_box", LIST_BOX },
   { "ok_cancel", OK_CANCEL },
   { "ok_cancel_help", OK_CANCEL_HELP },
   { "ok_cancel_help_errtile", OK_CANCEL_HELP_ERRTILE },
   { "ok_cancel_help_info", OK_CANCEL_HELP_INFO },
   { "ok_only", OK_ONLY },
   { "paragraph", PARAGRAPH },
   { "popup_list", POPUP_LIST },
   { "radio_button", RADIO_BUTTON },
   { "radio_column", RADIO_COLUMN },
   { "radio_row", RADIO_ROW },
   { "register", REGISTER },
   { "row", ROW },
   { "scroll", SCROLL },
   { "slider", SLIDER },
   { "spacer", SPACER },
   { "spacer_0", SPACER_0 },
   { "spacer_1", SPACER_1 },
   { "tab", TAB },
   { "text", TEXT },
   { "text_part", TEXT_PART },
   { "toggle", TOGGLE }
};

enum ATTRIBUTE_ID {
    NOATTR,
    ACTION,
    ALIGNMENT,
    ALLOW_ACCEPT,
    ASPECT_RATIO,
    BIG_INCREMENT,
    CHILDREN_ALIGNMENT,
    CHILDREN_FIXED_HEIGHT,
    CHILDREN_FIXED_WIDTH,
    COLOR,
    EDIT_LIMIT,
    EDIT_WIDTH,
    FIXED_HEIGHT,
    FIXED_WIDTH,
    FIXED_WIDTH_FONT,
    HEIGHT,
    INITIAL_FOCUS,
    IS_BOLD,
    IS_CANCEL,
    IS_DEFAULT,
    IS_ENABLED,
    IS_TAB_STOP,
    KEY,
    LABEL,
    LAYOUT,
    LIST,
    MAX_VALUE,
    MIN_VALUE,
    MNEMONIC,
    MULTIPLE_SELECT,
    PASSWORD_CHAR,
    SMALL_INCREMENT,
    TABS,
    TAB_TRUNCATE,
    VALUE,
    WIDTH
};

typedef enum ATTRIBUTE_ID attribute_id_t;

typedef struct attribute_prop {
    const char* name;
    attribute_id_t id;
} attribute_prop_t;

enum POS {
    NOPOS,
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    CENTERED,
    HORIZONTAL,
    VERTICAL
};

typedef enum POS pos_t;
typedef struct position_prop {
    const char* name;
    pos_t pos;
} position_prop_t;

static position_prop_t dclPosition[MAX_DCL_POS] = {
    { "nopos", NOPOS },
    { "left", LEFT },
    { "right", RIGHT },
    { "top", TOP },
    { "bottom", BOTTOM },
    { "centered", CENTERED },
    { "horizontal", HORIZONTAL },
    { "vertical", VERTICAL }
};

enum COLOR {
    DIALOG_LINE,
    DIALOG_FOREGROUND,
    DIALOG_BACKGROUND,
    GRAPHICS_BACKGROUND = 0,
    BLACK = 0,
    RED,
    YELLOW,
    GREEN,
    CYAN,
    BLUE,
    MAGENTA,
    WHITE = 7,
    GRAPHICS_FOREGROUND = 7
};

typedef enum COLOR color_t;
typedef struct color_prop {
    const char* name;
    int color;
} color_prop_t;

static color_prop_t dclColor[MAX_DCL_COLOR] = {
    { "dialog_line", -1002 },
    { "dialog_background", -1001},
    { "dialog_foreground", -1000 },
    { "graphics_background", 0 },
    { "black", 0 },
    { "red", 1 },
    { "yellow", 2 },
    { "green", 3 },
    { "cyan", 4 },
    { "blue", 5 },
    { "magenta", 6 },
    { "white", 7 },
    { "graphics_foreground", 7 }
};

typedef struct qcolor {
    int color;
    QColor qcolor;
} qcolor_t;

static QColor dclQColor[MAX_DCL_COLOR] = {
    Qt::black,
    Qt::red,
    Qt::yellow,
    Qt::green,
    Qt::cyan,
    Qt::blue,
    Qt::magenta,
    Qt::white,
    QColor(65, 65, 65),
    QColor(128, 128, 128),
    QColor(255, 0, 0),
    QColor(255, 170, 170),
    QColor(189, 0, 0),
    QColor(189, 126, 126),
    QColor(129, 0, 0),
    QColor(129, 86, 86),
    QColor(104, 0, 0),
    QColor(104, 69, 69),
    QColor(79, 0, 0),
    QColor(79, 53, 53),
    QColor(255, 63, 0),
    QColor(255, 191, 170),
    QColor(189, 46, 0),
    QColor(189, 141, 126),
    QColor(129, 31, 0),
    QColor(129, 96, 86),
    QColor(104, 25, 0),
    QColor(104, 78, 69),
    QColor(79, 19, 0),
    QColor(79, 59, 53),
    QColor(255, 127, 0),
    QColor(255, 212, 170),
    QColor(189, 94, 0),
    QColor(189, 157, 126),
    QColor(129, 64, 0),
    QColor(129, 107, 86),
    QColor(104, 52, 0),
    QColor(104, 86, 69),
    QColor(79, 39, 0),
    QColor(79, 66, 53),
    QColor(255, 191, 0),
    QColor(255, 234, 170),
    QColor(189, 141, 0),
    QColor(189, 173, 126),
    QColor(129, 96, 0),
    QColor(129, 118, 86),
    QColor(104, 78, 0),
    QColor(104, 95, 69),
    QColor(79, 59, 0),
    QColor(79, 73, 53),
    QColor(255, 255, 0),
    QColor(255, 255, 170),
    QColor(189, 189, 0),
    QColor(189, 189, 126),
    QColor(129, 129, 0),
    QColor(129, 129, 86),
    QColor(104, 104, 0),
    QColor(104, 104, 69),
    QColor(79, 79, 0),
    QColor(79, 79, 53),
    QColor(191, 255, 0),
    QColor(234, 255, 170),
    QColor(141, 189, 0),
    QColor(173, 189, 126),
    QColor(96, 129, 0),
    QColor(118, 129, 86),
    QColor(78, 104, 0),
    QColor(95, 104, 69),
    QColor(59, 79, 0),
    QColor(73, 79, 53),
    QColor(127, 255, 0),
    QColor(212, 255, 170),
    QColor(94, 189, 0),
    QColor(157, 189, 126),
    QColor(64, 129, 0),
    QColor(107, 129, 86),
    QColor(52, 104, 0),
    QColor(86, 104, 69),
    QColor(39, 79, 0),
    QColor(66, 79, 53),
    QColor(63, 255, 0),
    QColor(191, 255, 170),
    QColor(46, 189, 0),
    QColor(141, 189, 126),
    QColor(31, 129, 0),
    QColor(96, 129, 86),
    QColor(25, 104, 0),
    QColor(78, 104, 69),
    QColor(19, 79, 0),
    QColor(59, 79, 53),
    QColor(0, 255, 0),
    QColor(170, 255, 170),
    QColor(0, 189, 0),
    QColor(126, 189, 126),
    QColor(0, 129, 0),
    QColor(86, 129, 86),
    QColor(0, 104, 0),
    QColor(69, 104, 69),
    QColor(0, 79, 0),
    QColor(53, 79, 53),
    QColor(0, 255, 63),
    QColor(170, 255, 191),
    QColor(0, 189, 46),
    QColor(126, 189, 141),
    QColor(0, 129, 31),
    QColor(86, 129, 96),
    QColor(0, 104, 25),
    QColor(69, 104, 78),
    QColor(0, 79, 19),
    QColor(53, 79, 59),
    QColor(0, 255, 127),
    QColor(170, 255, 212),
    QColor(0, 189, 94),
    QColor(126, 189, 157),
    QColor(0, 129, 64),
    QColor(86, 129, 107),
    QColor(0, 104, 52),
    QColor(69, 104, 86),
    QColor(0, 79, 39),
    QColor(53, 79, 66),
    QColor(0, 255, 191),
    QColor(170, 255, 234),
    QColor(0, 189, 141),
    QColor(126, 189, 173),
    QColor(0, 129, 96),
    QColor(86, 129, 118),
    QColor(0, 104, 78),
    QColor(69, 104, 95),
    QColor(0, 79, 59),
    QColor(53, 79, 73),
    QColor(0, 255, 255),
    QColor(170, 255, 255),
    QColor(0, 189, 189),
    QColor(126, 189, 189),
    QColor(0, 129, 129),
    QColor(86, 129, 129),
    QColor(0, 104, 104),
    QColor(69, 104, 104),
    QColor(0, 79, 79),
    QColor(53, 79, 79),
    QColor(0, 191, 255),
    QColor(170, 234, 255),
    QColor(0, 141, 189),
    QColor(126, 173, 189),
    QColor(0, 96, 129),
    QColor(86, 118, 129),
    QColor(0, 78, 104),
    QColor(69, 95, 104),
    QColor(0, 59, 79),
    QColor(53, 73, 79),
    QColor(0, 127, 255),
    QColor(170, 212, 255),
    QColor(0, 94, 189),
    QColor(126, 157, 189),
    QColor(0, 64, 129),
    QColor(86, 107, 129),
    QColor(0, 52, 104),
    QColor(69, 86, 104),
    QColor(0, 39, 79),
    QColor(53, 66, 79),
    QColor(0, 63, 255),
    QColor(170, 191, 255),
    QColor(0, 46, 189),
    QColor(126, 141, 189),
    QColor(0, 31, 129),
    QColor(86, 96, 129),
    QColor(0, 25, 104),
    QColor(69, 78, 104),
    QColor(0, 19, 79),
    QColor(53, 59, 79),
    QColor(0, 0, 255),
    QColor(170, 170, 255),
    QColor(0, 0, 189),
    QColor(126, 126, 189),
    QColor(0, 0, 129),
    QColor(86, 86, 129),
    QColor(0, 0, 104),
    QColor(69, 69, 104),
    QColor(0, 0, 79),
    QColor(53, 53, 79),
    QColor(63, 0, 255),
    QColor(191, 170, 255),
    QColor(46, 0, 189),
    QColor(141, 126, 189),
    QColor(31, 0, 129),
    QColor(96, 86, 129),
    QColor(25, 0, 104),
    QColor(78, 69, 104),
    QColor(19, 0, 79),
    QColor(59, 53, 79),
    QColor(127, 0, 255),
    QColor(212, 170, 255),
    QColor(94, 0, 189),
    QColor(157, 126, 189),
    QColor(64, 0, 129),
    QColor(107, 86, 129),
    QColor(52, 0, 104),
    QColor(86, 69, 104),
    QColor(39, 0, 79),
    QColor(66, 53, 79),
    QColor(191, 0, 255),
    QColor(234, 170, 255),
    QColor(141, 0, 189),
    QColor(173, 126, 189),
    QColor(96, 0, 129),
    QColor(118, 86, 129),
    QColor(78, 0, 104),
    QColor(95, 69, 104),
    QColor(59, 0, 79),
    QColor(73, 53, 79),
    QColor(255, 0, 255),
    QColor(255, 170, 255),
    QColor(189, 0, 189),
    QColor(189, 126, 189),
    QColor(129, 0, 129),
    QColor(129, 86, 129),
    QColor(104, 0, 104),
    QColor(104, 69, 104),
    QColor(79, 0, 79),
    QColor(79, 53, 79),
    QColor(255, 0, 191),
    QColor(255, 170, 234),
    QColor(189, 0, 141),
    QColor(189, 126, 173),
    QColor(129, 0, 96),
    QColor(129, 86, 118),
    QColor(104, 0, 78),
    QColor(104, 69, 95),
    QColor(79, 0, 59),
    QColor(79, 53, 73),
    QColor(255, 0, 127),
    QColor(255, 170, 212),
    QColor(189, 0, 94),
    QColor(189, 126, 157),
    QColor(129, 0, 64),
    QColor(129, 86, 107),
    QColor(104, 0, 52),
    QColor(104, 69, 86),
    QColor(79, 0, 39),
    QColor(79, 53, 66),
    QColor(255, 0, 63),
    QColor(255, 170, 191),
    QColor(189, 0, 46),
    QColor(189, 126, 141),
    QColor(129, 0, 31),
    QColor(129, 86, 96),
    QColor(104, 0, 25),
    QColor(104, 69, 78),
    QColor(79, 0, 19),
    QColor(79, 53, 59),
    QColor(51, 51, 51),
    QColor(80, 80, 80),
    QColor(105, 105, 105),
    QColor(130, 130, 130),
    QColor(190, 190, 190),
    QColor(255, 255, 255)
};

typedef struct button_icon {
    const char* name;
    int id;
} button_icon_t;

static const button_icon_t dclIcon[MAX_DCL_ICONS] = {
    { "ok", QStyle::SP_DialogOkButton },
    { "save", QStyle::SP_DialogSaveButton },
    { "open", QStyle::SP_DialogOpenButton },
    { "cancel", QStyle::SP_DialogCancelButton },
    { "close", QStyle::SP_DialogCloseButton },
    { "apply", QStyle::SP_DialogApplyButton },
    { "reset", QStyle::SP_DialogResetButton },
    { "help", QStyle::SP_DialogHelpButton },
    { "discard", QStyle::SP_DialogDiscardButton },
    { "yes", QStyle::SP_DialogYesButton },
    { "no", QStyle::SP_DialogNoButton },
    { "yestoall", QStyle::SP_DialogYesToAllButton },
    { "notoall", QStyle::SP_DialogNoToAllButton },
    { "saveall", QStyle::SP_DialogSaveAllButton },
    { "abort", QStyle::SP_DialogAbortButton },
    { "retry", QStyle::SP_DialogRetryButton },
    { "ignore", QStyle::SP_DialogIgnoreButton },
    { "defaults", QStyle::SP_RestoreDefaultsButton }
};

typedef struct guitile {
    bool            allow_accept = false;
    bool            children_fixed_height = false;
    bool            children_fixed_width = false;
    bool            fixed_height = false;
    bool            fixed_width = false;
    bool            fixed_width_font = false;
    bool            is_bold = false;
    bool            is_cancel = false;
    bool            is_default = false;
    bool            is_enabled = true;
    bool            is_tab_stop = true;
    bool            multiple_select = false;
    bool            tab_truncate = false;
    bool            has_parent = false;
    int64_t         big_increment = 0;
    int64_t         color = 0;
    int64_t         dialog_Id = -1;
    int64_t         edit_limit = 132;
    int64_t         max_value = 10000;
    int64_t         min_value = 0;
    int64_t         small_increment = 0;
    double          aspect_ratio = 0.0;
    double          edit_width = 0.0;
    double          height = 0.0;
    double          width = 0.0;
    pos_t           alignment = LEFT;
    pos_t           children_alignment = NOPOS;
    pos_t           layout = HORIZONTAL;
    tile_id_t       id = NONE;
    String          action = "";
    String          initial_focus = "";
    String          key = "";
    String          label = "";
    String          list = "";
    String          mnemonic = "";
    String          name = "";
    String          password_char = "";
    String          tabs = "";
    String          value = "";
    lclValueVec*    tiles;
} tile_t;

typedef struct child_config {
    bool            children_fixed_height = false;
    bool            children_fixed_width = false;
    pos_t           children_alignment = NOPOS;
} child_config_t;

static attribute_prop_t dclAttribute[MAX_DCL_ATTR] = {
    { "action", ACTION },
    { "alignment", ALIGNMENT },
    { "allow_accept", ALLOW_ACCEPT },
    { "aspect_ratio", ASPECT_RATIO },
    { "big_increment", BIG_INCREMENT },
    { "children_alignment", CHILDREN_ALIGNMENT },
    { "children_fixed_height", CHILDREN_FIXED_HEIGHT },
    { "children_fixed_width", CHILDREN_FIXED_WIDTH },
    { "color", COLOR },
    { "edit_limit", EDIT_LIMIT },
    { "edit_width", EDIT_WIDTH },
    { "fixed_height", FIXED_HEIGHT },
    { "fixed_width", FIXED_WIDTH },
    { "fixed_width_font", FIXED_WIDTH_FONT },
    { "height", HEIGHT },
    { "initial_focus", INITIAL_FOCUS },
    { "is_bold", IS_BOLD },
    { "is_cancel", IS_CANCEL },
    { "is_default", IS_DEFAULT },
    { "is_enabled", IS_ENABLED },
    { "is_tab_stop", IS_TAB_STOP },
    { "key", KEY },
    { "label", LABEL },
    { "layout", LAYOUT },
    { "list", LIST },
    { "max_value", MAX_VALUE },
    { "min_value", MIN_VALUE },
    { "mnemonic", MNEMONIC },
    { "multiple_select", MULTIPLE_SELECT },
    { "password_char", PASSWORD_CHAR },
    { "small_increment", SMALL_INCREMENT },
    { "tabs", TABS },
    { "tab_truncate", TAB_TRUNCATE },
    { "value", VALUE },
    { "width", WIDTH }
};


enum PaintAction { DCL_NON, DCL_LINE, DCL_RECT, DCL_TXT, DCL_PIX, DCL_SLD};

typedef struct dclVector
{
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    int color = 0;
    QString str = "";
    PaintAction action = DCL_NON;
} dclVector_t;

typedef std::vector<dclVector_t> dclVectors;

QColor getDclQColor(int c);
extern attribute_id_t getDclAttributeId(const String& str);

class QDclLabel : public QLabel
{
public:

    QDclLabel(QWidget *parent=nullptr);
    ~QDclLabel() { delete m_pixs; }

    void addLine(int x1, int y1, int x2, int y2, int color);
    void addRect(int x1, int y1, int width, int height, int color);
    void addPicture(int x1, int y1, int width, int height, double aspect_ratio, const QString &name);
    void addSlide(int x1,int y1,int width, int height, double aspect_ratio, const QString &name);
    void addText(int x1, int y1, int width, int height, const QString &text, int color);

protected:
    virtual void paintEvent(QPaintEvent *event) override;

private:
    dclVectors *m_pixs;
};

class QDclButton : public QAbstractButton
{
public:

    QDclButton(QWidget *parent=nullptr);
    ~QDclButton() { delete m_pixs; }

    void addLine(int x1, int y1, int x2, int y2, int color);
    void addRect(int x1, int y1, int width, int height, int color);
    void addPicture(int x1, int y1, int width, int height, double aspect_ratio, const QString &name);
    void addSlide(int x1,int y1,int width, int height, double aspect_ratio, const QString &name);
    void addText(int x1, int y1, int width, int height, const QString &text, int color);

protected:
    virtual void paintEvent(QPaintEvent *event);

private:
    dclVectors *m_pixs;
};

class DclEditStyle : public QProxyStyle
{
public:
    DclEditStyle(QStyle *style = 0) : QProxyStyle(style) { }

    int styleHint(StyleHint hint, const QStyleOption * option = 0,
                  const QWidget * widget = 0, QStyleHintReturn * returnData = 0 ) const
    {
        if (hint==QStyle::SH_LineEdit_PasswordCharacter)
            return m_char;
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }

    void setPasswordCharacter(const char pwchar) { m_char = pwchar; }
private:
    char m_char;
};

class ListEvent: public QObject
{
    Q_OBJECT
public:
    bool eventFilter(QObject *o, QEvent *e);
};

class lclGui : public lclValue {
public:
    lclGui(const tile_t& tile) : m_value(tile) { }
    lclGui(const lclGui& that, lclValuePtr meta)
        : lclValue(meta), m_value(that.m_value) { }

    virtual ~lclGui() { delete m_value.tiles; }

    tile_t value() const { return m_value; }

    virtual String print(bool) const override {
        return STRF("#builtin-gui(%s)", m_value.name.c_str());
    }

    virtual LCLTYPE type() const override { return LCLTYPE::GUI; }

    virtual bool doIsEqualTo(const lclValue*) const override { return false; }

    virtual lclValuePtr conj(lclValueIter argsBegin,
                             lclValueIter argsEnd) const;

    virtual QVBoxLayout* vlayout() const { return nullptr; }
    virtual QHBoxLayout* hlayout() const { return nullptr; }

private:
    const tile_t m_value;
};

class lclDclGui : public lclGui {
public:
    lclDclGui(const tile_t& tile) : lclGui(tile) { }
    lclDclGui(const lclDclGui& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    WITH_META(lclDclGui)
};

class lclDialog : public lclGui {
public:
    lclDialog(const tile_t& tile);
    lclDialog(const lclDialog& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclDialog() { delete m_dialog; delete m_layout; }

    WITH_META(lclDialog)

    QDialog* dialog() const { return m_dialog; }
    virtual QVBoxLayout* vlayout() const override { return m_layout; }

    void addShortcut(const QString & key, QWidget *w);

private:
    QDialog *m_dialog;
    QVBoxLayout *m_layout;
};

class lclTabWidget : public lclGui {
public:
    lclTabWidget(const tile_t& tile);
    lclTabWidget(const lclTabWidget& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclTabWidget() { delete m_widget; }

    WITH_META(lclTabWidget)

    QTabWidget* widget() const { return m_widget; }

private:
    QTabWidget *m_widget;
};

class lclWidget : public lclGui {
public:
    lclWidget(const tile_t& tile);
    lclWidget(const lclWidget& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclWidget() { delete m_widget; delete m_layout; }

    WITH_META(lclWidget)

    QWidget* widget() const { return m_widget; }
    virtual QVBoxLayout* vlayout() const override { return m_layout; }

private:
    QWidget *m_widget;
    QVBoxLayout *m_layout;
};

class lclButton : public lclGui {
public:
    lclButton(const tile_t& tile);
    lclButton(const lclButton& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclButton() { delete m_button; delete m_vlayout; delete m_hlayout; }

    WITH_META(lclButton)

    QPushButton* button() const { return m_button; }
    virtual QVBoxLayout* vlayout() const override { return m_vlayout; }
    virtual QHBoxLayout* hlayout() const override { return m_hlayout; }

    void clicked(bool checked);

private:
    QPushButton *m_button;
    QVBoxLayout *m_vlayout;
    QHBoxLayout *m_hlayout;
};

class lclRadioButton : public lclGui {
public:
    lclRadioButton(const tile_t& tile);
    lclRadioButton(const lclRadioButton& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclRadioButton() { delete m_button; delete m_vlayout; delete m_hlayout; }

    WITH_META(lclRadioButton)

    QRadioButton* button() const { return m_button; }
    virtual QVBoxLayout* vlayout() const override { return m_vlayout; }
    virtual QHBoxLayout* hlayout() const override { return m_hlayout; }

    void clicked(bool checked);

private:
    QRadioButton *m_button;
    QVBoxLayout *m_vlayout;
    QHBoxLayout *m_hlayout;
};

class lclImageButton : public lclGui {
public:
    lclImageButton(const tile_t& tile);
    lclImageButton(const lclImageButton& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclImageButton() { delete m_button; delete m_vlayout; delete m_hlayout; }

    WITH_META(lclImageButton)

    QDclButton* button() const { return m_button; }
    virtual QVBoxLayout* vlayout() const override { return m_vlayout; }
    virtual QHBoxLayout* hlayout() const override { return m_hlayout; }

    void clicked(bool checked);

private:
    QDclButton *m_button;
    QVBoxLayout *m_vlayout;
    QHBoxLayout *m_hlayout;
};

class lclEdit : public lclGui {
public:
    lclEdit(const tile_t& tile);
    lclEdit(const lclEdit& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclEdit() { delete m_edit; delete m_label; delete m_layout; }

    WITH_META(lclEdit)

    QLineEdit* edit() const { return m_edit; }
    QLabel* label() const { return m_label; }

    virtual QHBoxLayout* hlayout() const override { return m_layout; }

    void textEdited(const QString &text);

private:
    QLineEdit *m_edit;
    QLabel *m_label;
    QHBoxLayout *m_layout;
};

class lclListBox : public lclGui {
public:
    lclListBox(const tile_t& tile);
    lclListBox(const lclListBox& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclListBox() { delete m_list; delete m_vlayout; delete m_hlayout; }

    WITH_META(lclListBox)

    QListWidget* list() const { return m_list; }
    QLabel* label() const { return m_label; }
    virtual QVBoxLayout* vlayout() const override { return m_vlayout; }
    virtual QHBoxLayout* hlayout() const override { return m_hlayout; }

    void currentTextChanged(const QString &currentText);

private:
    QListWidget *m_list;
    QLabel *m_label;
    QVBoxLayout *m_vlayout;
    QHBoxLayout *m_hlayout;
};

class lclLabel : public lclGui {
public:
    lclLabel(const tile_t& tile);
    lclLabel(const lclLabel& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclLabel() { delete m_label; }

    WITH_META(lclLabel)

    QLabel* label() const { return m_label; }

private:
    QLabel *m_label;
};

class lclRow : public lclGui {
public:
    lclRow(const tile_t& tile);
    lclRow(const lclRow& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclRow() { delete m_widget; delete m_layout; }

    WITH_META(lclRow)

    QWidget* widget() const { return m_widget; }
    virtual QHBoxLayout* hlayout() const override { return m_layout; }

private:
    QWidget *m_widget;
    QHBoxLayout *m_layout;
};

class lclBoxedRow : public lclGui {
public:
    lclBoxedRow(const tile_t& tile);
    lclBoxedRow(const lclBoxedRow& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclBoxedRow() { delete m_layout; delete m_groupbox; }

    WITH_META(lclBoxedRow)

    virtual QHBoxLayout* hlayout() const override { return m_layout; }
    QGroupBox* groupbox() const { return m_groupbox; }

private:
    QHBoxLayout *m_layout;
    QGroupBox *m_groupbox;
};

class lclColumn : public lclGui {
public:
    lclColumn(const tile_t& tile);
    lclColumn(const lclColumn& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclColumn() { delete m_widget; delete m_layout; }

    WITH_META(lclColumn)

    QWidget* widget() const { return m_widget; }
    virtual QVBoxLayout* vlayout() const override { return m_layout; }

private:
    QWidget *m_widget;
    QVBoxLayout *m_layout;
};

class lclBoxedColumn : public lclGui {
public:
    lclBoxedColumn(const tile_t& tile);
    lclBoxedColumn(const lclBoxedColumn& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclBoxedColumn() { delete m_layout; delete m_groupbox; }

    WITH_META(lclBoxedColumn)

    virtual QVBoxLayout* vlayout() const override { return m_layout; }
    QGroupBox* groupbox() const { return m_groupbox; }

private:
    QVBoxLayout *m_layout;
    QGroupBox *m_groupbox;
};

class lclImage : public lclGui {
public:
    lclImage(const tile_t& tile);
    lclImage(const lclImage& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclImage() { delete m_image; }

    WITH_META(lclImage)

    QDclLabel* image() const { return m_image; }

private:
    QDclLabel *m_image;
};

class lclPopupList : public lclGui {
public:
    lclPopupList(const tile_t& tile);
    lclPopupList(const lclPopupList& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclPopupList() { delete m_list; delete m_vlayout; delete m_hlayout; }

    WITH_META(lclPopupList)

    QComboBox* list() const { return m_list; }
    virtual QVBoxLayout* vlayout() const override { return m_vlayout; }
    virtual QHBoxLayout* hlayout() const override { return m_hlayout; }

    void currentTextChanged(const QString &currentText);

private:
    QComboBox *m_list;
    QVBoxLayout *m_vlayout;
    QHBoxLayout *m_hlayout;
};

class lclScrollBar : public lclGui {
public:
    lclScrollBar(const tile_t& tile);
    lclScrollBar(const lclScrollBar& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclScrollBar() { delete m_slider; delete m_vlayout; delete m_hlayout; }

    WITH_META(lclScrollBar)

    QScrollBar* slider() const { return m_slider; }
    virtual QVBoxLayout* vlayout() const override { return m_vlayout; }
    virtual QHBoxLayout* hlayout() const override { return m_hlayout; }

    void valueChanged(int value);
    void sliderReleased();

private:
    QScrollBar *m_slider;
    QVBoxLayout *m_vlayout;
    QHBoxLayout *m_hlayout;
};

class lclSlider : public lclGui {
public:
    lclSlider(const tile_t& tile);
    lclSlider(const lclSlider& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclSlider() { delete m_slider; delete m_vlayout; delete m_hlayout; }

    WITH_META(lclSlider)

    QSlider* slider() const { return m_slider; }
    virtual QVBoxLayout* vlayout() const override { return m_vlayout; }
    virtual QHBoxLayout* hlayout() const override { return m_hlayout; }

    void valueChanged(int value);
    void sliderReleased();

private:
    QSlider *m_slider;
    QVBoxLayout *m_vlayout;
    QHBoxLayout *m_hlayout;
};

class lclDial : public lclGui {
public:
    lclDial(const tile_t& tile);
    lclDial(const lclDial& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclDial() { delete m_slider; delete m_vlayout; delete m_hlayout; }

    WITH_META(lclDial)

    QDial* slider() const { return m_slider; }
    virtual QVBoxLayout* vlayout() const override { return m_vlayout; }
    virtual QHBoxLayout* hlayout() const override { return m_hlayout; }

    void valueChanged(int value);
    void sliderReleased();

private:
    QDial *m_slider;
    QVBoxLayout *m_vlayout;
    QHBoxLayout *m_hlayout;
};

class lclSpacer : public lclGui {
public:
    lclSpacer(const tile_t& tile);
    lclSpacer(const lclSpacer& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclSpacer() { delete m_spacer; }

    WITH_META(lclSpacer)

    QSpacerItem* spacer() const { return m_spacer; }

private:
    QSpacerItem *m_spacer;
};

class lclToggle : public lclGui {
public:
    lclToggle(const tile_t& tile);
    lclToggle(const lclToggle& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclToggle() { delete m_toggle; }

    WITH_META(lclToggle)

    QCheckBox* toggle() const { return m_toggle; delete m_vlayout; delete m_hlayout; }
    virtual QVBoxLayout* vlayout() const override { return m_vlayout; }
    virtual QHBoxLayout* hlayout() const override { return m_hlayout; }

    void stateChanged(int state);

private:
    QCheckBox *m_toggle;
    QVBoxLayout *m_vlayout;
    QHBoxLayout *m_hlayout;
};

class lclOkCancel : public lclGui {
public:
    lclOkCancel(const tile_t& tile);
    lclOkCancel(const lclOkCancel& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclOkCancel() { delete m_btnCancel; delete m_btnOk; delete m_layout; }

    WITH_META(lclOkCancel)

    QPushButton* binOK() const { return m_btnOk; }
    QPushButton* btnCancel() const { return m_btnCancel; }
    virtual QHBoxLayout* hlayout() const override { return m_layout; }

    void okClicked(bool checked);
    void cancelClicked(bool checked);

private:
    QPushButton *m_btnOk;
    QPushButton *m_btnCancel;
    QHBoxLayout *m_layout;
};

class lclOkCancelHelp : public lclGui {
public:
    lclOkCancelHelp(const tile_t& tile);
    lclOkCancelHelp(const lclOkCancelHelp& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclOkCancelHelp() { delete m_btnCancel; delete m_btnOk; delete m_btnHelp; delete m_layout; }

    WITH_META(lclOkCancelHelp)

    QPushButton* binOK() const { return m_btnOk; }
    QPushButton* btnCancel() const { return m_btnCancel; }
    QPushButton* btnHelp() const { return m_btnHelp; }
    virtual QHBoxLayout* hlayout() const override { return m_layout; }

    void okClicked(bool checked);
    void cancelClicked(bool checked);
    void helpClicked(bool checked);

private:
    QPushButton *m_btnOk;
    QPushButton *m_btnCancel;
    QPushButton *m_btnHelp;
    QHBoxLayout *m_layout;
};

class lclOkCancelHelpInfo : public lclGui {
public:
    lclOkCancelHelpInfo(const tile_t& tile);
    lclOkCancelHelpInfo(const lclOkCancelHelpInfo& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclOkCancelHelpInfo() { delete m_btnCancel; delete m_btnOk; delete m_btnHelp; delete m_btnInfo; delete m_layout; }

    WITH_META(lclOkCancelHelpInfo)

    QPushButton* binOK() const { return m_btnOk; }
    QPushButton* btnCancel() const { return m_btnCancel; }
    QPushButton* btnHelp() const { return m_btnHelp; }
    QPushButton* btnInfo() const { return m_btnInfo; }
    virtual QHBoxLayout* hlayout() const override { return m_layout; }

    void okClicked(bool checked);
    void cancelClicked(bool checked);
    void helpClicked(bool checked);
    void infoClicked(bool checked);

private:
    QPushButton *m_btnOk;
    QPushButton *m_btnCancel;
    QPushButton *m_btnHelp;
    QPushButton *m_btnInfo;
    QHBoxLayout *m_layout;
};

class lclOkCancelHelpErrtile : public lclGui {
public:
    lclOkCancelHelpErrtile(const tile_t& tile);
    lclOkCancelHelpErrtile(const lclOkCancelHelpErrtile& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclOkCancelHelpErrtile() { delete m_btnCancel; delete m_btnOk; delete m_btnHelp; delete m_errTile; delete m_layout; delete m_hlayout;}

    WITH_META(lclOkCancelHelpErrtile)

    QPushButton* binOK() const { return m_btnOk; }
    QPushButton* btnCancel() const { return m_btnCancel; }
    QPushButton* btnHelp() const { return m_btnHelp; }
    QLabel* errtile() const { return m_errTile; }
    virtual QVBoxLayout* vlayout() const override { return m_layout; }

    void okClicked(bool checked);
    void cancelClicked(bool checked);
    void helpClicked(bool checked);

private:
    QPushButton *m_btnOk;
    QPushButton *m_btnCancel;
    QPushButton *m_btnHelp;
    QLabel *m_errTile;
    QHBoxLayout *m_hlayout;
    QVBoxLayout *m_layout;
};

extern std::vector<const lclGui*> dclTiles;
extern void openTile(const lclGui* tile, const child_config_t child_cfg = { false, false, NOPOS });

namespace lcl {
    lclValuePtr atom(lclValuePtr value);
    lclValuePtr boolean(bool value);
    lclValuePtr builtin(const String& name, lclBuiltIn::ApplyFunc handler);
    lclValuePtr builtin(bool eval, const String&);
    lclValuePtr falseValue();
    lclValuePtr file(const char *path, const char &mode);
    lclValuePtr gui(const tile_t& tile);
    lclValuePtr hash(lclValueIter argsBegin, lclValueIter argsEnd,
                     bool isEvaluated);
    lclValuePtr hash(const lclHash::Map& map);
    lclValuePtr integer(int64_t value);
    lclValuePtr integer(const String& token);
    lclValuePtr keyword(const String& token);
    lclValuePtr lambda(const StringVec&, lclValuePtr, lclEnvPtr);
    lclValuePtr list(lclValueVec* items);
    lclValuePtr list(lclValueIter begin, lclValueIter end);
    lclValuePtr list(lclValuePtr a);
    lclValuePtr list(lclValuePtr a, lclValuePtr b);
    lclValuePtr list(lclValuePtr a, lclValuePtr b, lclValuePtr c);
    lclValuePtr macro(const lclLambda& lambda);
    lclValuePtr ldouble(double value);
    lclValuePtr ldouble(const String& token);
    lclValuePtr nilValue();
    lclValuePtr nullValue();
    lclValuePtr string(const String& token);
    lclValuePtr symbol(const String& token);
    lclValuePtr trueValue();
    lclValuePtr type(LCLTYPE type);
    lclValuePtr typeAtom();
    lclValuePtr typeBuiltin();
    lclValuePtr typeFile();
    lclValuePtr typeInteger();
    lclValuePtr typeList();
    lclValuePtr typeMap();
    lclValuePtr typeReal();
    lclValuePtr typeString();
    lclValuePtr typeSymbol();
    lclValuePtr typeUndef();
    lclValuePtr typeVector();
    lclValuePtr typeKeword();
    lclValuePtr piValue();
    lclValuePtr vector(lclValueVec* items);
    lclValuePtr vector(lclValueIter begin, lclValueIter end);

    lclValuePtr dclgui(const tile_t& tile);
    lclValuePtr dialog(const tile_t& tile);
    lclValuePtr tabwidget(const tile_t& tile);
    lclValuePtr widget(const tile_t& tile);
    lclValuePtr boxedcolumn(const tile_t& tile);
    lclValuePtr boxedrow(const tile_t& tile);
    lclValuePtr button(const tile_t& tile);
    lclValuePtr column(const tile_t& tile);
    lclValuePtr label(const tile_t& tile);
    lclValuePtr popuplist(const tile_t& tile);
    lclValuePtr row(const tile_t& tile);
    lclValuePtr radiobutton(const tile_t& tile);

    lclValuePtr edit(const tile_t& tile);
    lclValuePtr listbox(const tile_t& tile);
    lclValuePtr okcancel(const tile_t& tile);
    lclValuePtr okcancelhelp(const tile_t& tile);
    lclValuePtr okcancelhelpinfo(const tile_t& tile);
    lclValuePtr okcancelhelperrtile(const tile_t& tile);

    lclValuePtr image(const tile_t& tile);
    lclValuePtr imagebutton(const tile_t& tile);
    lclValuePtr scroll(const tile_t& tile);
    lclValuePtr slider(const tile_t& tile);
    lclValuePtr dial(const tile_t& tile);
    lclValuePtr spacer(const tile_t& tile);
    lclValuePtr toggle(const tile_t& tile);

}

#endif // DEVELOPER

#endif // INCLUDE_TYPES_H
