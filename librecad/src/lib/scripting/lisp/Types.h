
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
#include <QPushButton>
#include <QRadioButton>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QCheckBox>
#include <QSpacerItem>

#ifdef DEVELOPER

class lclEmptyInputException : public std::exception { };

enum class LCLTYPE { ATOM, BUILTIN, BOOLEAN, FILE, GUI, INT, LIST, MAP, REAL, STR, SYM, UNDEF, VEC, KEYW };

#define MAX_DCL_TILES 31
#define MAX_DCL_ATTR 35
#define MAX_DCL_POS 8
#define MAX_DCL_COLOR 13

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
    virtual lclValuePtr doWithMeta(lclValuePtr meta) const { \
        return new Type(*this, meta); \
    } \

class lclConstant : public lclValue {
public:
    lclConstant(String name) : m_name(name) { }
    lclConstant(const lclConstant& that, lclValuePtr meta)
        : lclValue(meta), m_name(that.m_name) { }

    virtual String print(bool) const { return m_name; }

    virtual LCLTYPE type() const {
        if ((m_name.compare("false") ||
            (m_name.compare("true")))) {
            return LCLTYPE::BOOLEAN; }
        else {
            return LCLTYPE::UNDEF; }
    }

    virtual bool doIsEqualTo(const lclValue* rhs) const {
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

    virtual String print(bool) const {
        return std::to_string(m_value);
    }

    virtual LCLTYPE type() const { return LCLTYPE::INT; }

    int64_t value() const { return m_value; }

    virtual bool doIsEqualTo(const lclValue* rhs) const;

    WITH_META(lclInteger)

private:
    const int64_t m_value;
};

class lclDouble : public lclValue {
public:
    lclDouble(double value) : m_value(value) { }
    lclDouble(const lclDouble& that, lclValuePtr meta)
        : lclValue(meta), m_value(that.m_value) { }

    virtual String print(bool) const {
        return std::to_string(m_value);
    }

    virtual bool isFloat() const { return true; }
    virtual LCLTYPE type() const { return LCLTYPE::REAL; }

    double value() const { return m_value; }

    virtual bool doIsEqualTo(const lclValue* rhs) const;

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

    virtual String print(bool) const {
        String path = "#<file \"";
        path += m_path;
        path += "\">";
        return path;
    }

    virtual LCLTYPE type() const { return LCLTYPE::FILE; }

    virtual bool doIsEqualTo(const lclValue* rhs) const {
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

    virtual String print(bool) const { return m_value; }

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

    virtual String print(bool readably) const;
    virtual LCLTYPE type() const { return LCLTYPE::STR; }

    String escapedValue() const;

    virtual bool doIsEqualTo(const lclValue* rhs) const {
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

    virtual bool doIsEqualTo(const lclValue* rhs) const {
        return value() == static_cast<const lclKeyword*>(rhs)->value();
    }

    virtual LCLTYPE type() const { return LCLTYPE::KEYW; }

    WITH_META(lclKeyword)
};

class lclSymbol : public lclStringBase {
public:
    lclSymbol(const String& token)
        : lclStringBase(token) { }
    lclSymbol(const lclSymbol& that, lclValuePtr meta)
        : lclStringBase(that, meta) { }

    virtual lclValuePtr eval(lclEnvPtr env);

    virtual bool doIsEqualTo(const lclValue* rhs) const {
        return value() == static_cast<const lclSymbol*>(rhs)->value();
    }

    virtual LCLTYPE type() const { return LCLTYPE::SYM; }

    WITH_META(lclSymbol)
};

class lclSequence : public lclValue {
public:
    lclSequence(lclValueVec* items);
    lclSequence(lclValueIter begin, lclValueIter end);
    lclSequence(const lclSequence& that, lclValuePtr meta);
    virtual ~lclSequence();

    virtual String print(bool readably) const;

    lclValueVec* evalItems(lclEnvPtr env) const;
    int count() const { return m_items->size(); }
    bool isEmpty() const { return m_items->empty(); }
    bool isDotted() const;
    lclValuePtr item(int index) const { return (*m_items)[index]; }

    lclValueIter begin() const { return m_items->begin(); }
    lclValueIter end()   const { return m_items->end(); }

    virtual bool doIsEqualTo(const lclValue* rhs) const;

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

    virtual String print(bool readably) const;
    virtual LCLTYPE type() const { return LCLTYPE::LIST; }
    virtual lclValuePtr eval(lclEnvPtr env);

    virtual lclValuePtr conj(lclValueIter argsBegin,
                             lclValueIter argsEnd) const;

    WITH_META(lclList)
};

class lclVector : public lclSequence {
public:
    lclVector(lclValueVec* items) : lclSequence(items) { }
    lclVector(lclValueIter begin, lclValueIter end)
        : lclSequence(begin, end) { }
    lclVector(const lclVector& that, lclValuePtr meta)
        : lclSequence(that, meta) { }

    virtual lclValuePtr eval(lclEnvPtr env);
    virtual String print(bool readably) const;
    virtual LCLTYPE type() const { return LCLTYPE::VEC; }

    virtual lclValuePtr conj(lclValueIter argsBegin,
                             lclValueIter argsEnd) const;

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
    lclValuePtr eval(lclEnvPtr env);
    lclValuePtr get(lclValuePtr key) const;
    lclValuePtr keys() const;
    lclValuePtr values() const;

    virtual String print(bool readably) const;

    virtual bool doIsEqualTo(const lclValue* rhs) const;

    virtual LCLTYPE type() const { return LCLTYPE::MAP; }

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
                              lclValueIter argsEnd) const;

    virtual String print(bool) const {
        return STRF("#builtin-function(%s)", m_name.c_str());
    }

    virtual bool doIsEqualTo(const lclValue* rhs) const {
        return this == rhs; // these are singletons
    }

    String name() const { return m_name; }

    virtual LCLTYPE type() const { return LCLTYPE::BUILTIN; }

    WITH_META(lclBuiltIn)

private:
    [[maybe_unused]] bool m_inEval;
    const String m_name;
    ApplyFunc* m_handler;
};

class lclLambda : public lclApplicable {
public:
    lclLambda(const StringVec& bindings, lclValuePtr body, lclEnvPtr env);
    lclLambda(const lclLambda& that, lclValuePtr meta);
    lclLambda(const lclLambda& that, bool isMacro);

    virtual lclValuePtr apply(lclValueIter argsBegin,
                              lclValueIter argsEnd) const;

    lclValuePtr getBody() const { return m_body; }
    lclEnvPtr makeEnv(lclValueIter argsBegin, lclValueIter argsEnd) const;

    virtual bool doIsEqualTo(const lclValue* rhs) const {
        return this == rhs; // do we need to do a deep inspection?
    }

    virtual String print(bool) const {
        return STRF("#user-%s(%p)", m_isMacro ? "macro" : "function", this);
    }

    bool isMacro() const { return m_isMacro; }

    virtual lclValuePtr doWithMeta(lclValuePtr meta) const;

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

    virtual bool doIsEqualTo(const lclValue* rhs) const {
        return this->m_value->isEqualTo(rhs);
    }

    virtual String print(bool readably) const {
        return "(atom " + m_value->print(readably) + ")";
    };

    virtual LCLTYPE type() const { return LCLTYPE::ATOM; }

    lclValuePtr deref() const { return m_value; }

    lclValuePtr reset(lclValuePtr value) { return m_value = value; }

    WITH_META(lclAtom)

private:
    lclValuePtr m_value;
};

enum TILE_ID {
    NONE,
    BUTTON,
    CONCATENATION,
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
    PARAGRAPH,
    POPUP_LIST,
    RADIO_BUTTON,
    RADIO_COLUMN,
    RADIO_ROW,
    SLIDER,
    SPACER,
    SPACER_0,
    SPACER_1,
    TEXT,
    TEXT_PART,
    TOGGLE,
    DIALOG,
    COLUMN = 32,
    BOXED_COLUMN = 64,
    BOXED_RADIO_COLUMN = 128,
    BOXED_RADIO_ROW = 256,
    BOXED_ROW = 512,
    ROW = 1024,
};

#define LAYOUT_TILE (COLUMN | BOXED_COLUMN | BOXED_ROW | ROW)
#define LAYOUT_ROW (BOXED_ROW | ROW)

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
   { "row", ROW },
   { "slider", SLIDER },
   { "spacer", SPACER },
   { "spacer_0", SPACER_0 },
   { "spacer_1", SPACER_1 },
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
    WHITE,
    GRAPHICS_FOREGROUND = 7
};

typedef enum COLOR color_t;
typedef struct color_prop {
    const char* name;
    color_t color;
} color_prop_t;

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
    int64_t         dialog_Id = -1;
    int64_t         big_increment = 0;
    int64_t         edit_limit = 132;
    int64_t         max_value = 10000;
    int64_t         min_value = 0;
    int64_t         small_increment = 0;
    double          aspect_ratio = 0.0;
    double          edit_width = 0.0;
    double          height = 0.0;
    double          width = 0.0;
    pos_t           alignment = LEFT;
    pos_t           children_alignment = LEFT;
    pos_t           layout = HORIZONTAL;
    color_t         color = WHITE;
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

class lclGui : public lclValue {
public:
    lclGui(const tile_t& tile) : m_value(tile) { }
    lclGui(const lclGui& that, lclValuePtr meta)
        : lclValue(meta), m_value(that.m_value) { }

    virtual ~lclGui() { delete m_value.tiles; }

    tile_t value() const { return m_value; }

    virtual String print(bool) const {
        return STRF("#builtin-gui(%s)", m_value.name.c_str());
    }

    virtual LCLTYPE type() const { return LCLTYPE::GUI; }

    virtual bool doIsEqualTo(const lclValue*) const { return false; }

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

class lclWidget : public lclGui {
public:
    lclWidget(const tile_t& tile);
    lclWidget(const lclWidget& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclWidget() { delete m_widget; }

    WITH_META(lclWidget)

    QDialog* widget() const { return m_widget; }
    virtual QVBoxLayout* vlayout() const { return m_layout; }

private:
    QDialog* m_widget;
    QVBoxLayout* m_layout;
};

class lclButton : public lclGui {

public:
    lclButton(const tile_t& tile);
    lclButton(const lclButton& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclButton() { delete m_button; }

    WITH_META(lclButton)

    QPushButton* button() const { return m_button; }

    void clicked(bool checked);

private:
    QPushButton* m_button;
//    QVBoxLayout* m_vlayout;
//    QHBoxLayout* m_hlayout;
};

class lclRadioButton : public lclGui {
public:
    lclRadioButton(const tile_t& tile);
    lclRadioButton(const lclRadioButton& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclRadioButton() { delete m_button; }

    WITH_META(lclRadioButton)

    QRadioButton* button() const { return m_button; }

    void clicked(bool checked);

private:
    QRadioButton* m_button;
//    QVBoxLayout* m_vlayout;
//    QHBoxLayout* m_hlayout;
};

class lclEdit : public lclGui {
public:
    lclEdit(const tile_t& tile);
    lclEdit(const lclEdit& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclEdit() { delete m_edit; }

    WITH_META(lclEdit)

    QLineEdit* edit() const { return m_edit; }

    void textEdited(const QString &text);

private:
    QLineEdit* m_edit;
};

class lclListBox : public lclGui {
public:
    lclListBox(const tile_t& tile);
    lclListBox(const lclListBox& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclListBox() { delete m_list; }

    WITH_META(lclListBox)

    QListWidget* list() const { return m_list; }

    void currentTextChanged(const QString &currentText);

private:
    QListWidget* m_list;
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
    QLabel* m_label;
};

class lclRow : public lclGui {
public:
    lclRow(const tile_t& tile);
    lclRow(const lclRow& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclRow() { delete m_layout; }

    WITH_META(lclRow)

    virtual QHBoxLayout* hlayout() const { return m_layout; }

private:
    QHBoxLayout* m_layout;
};

class lclBoxedRow : public lclGui {
public:
    lclBoxedRow(const tile_t& tile);
    lclBoxedRow(const lclBoxedRow& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclBoxedRow() { delete m_layout; delete m_groupbox; }

    WITH_META(lclBoxedRow)

    virtual QHBoxLayout* hlayout() const { return m_layout; }
    QGroupBox* groupbox() const { return m_groupbox; }

private:
    QHBoxLayout* m_layout;
    QGroupBox* m_groupbox;
};

class lclBoxedRadioRow : public lclGui {
public:
    lclBoxedRadioRow(const tile_t& tile);
    lclBoxedRadioRow(const lclBoxedRadioRow& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclBoxedRadioRow() { delete m_layout; delete m_groupbox; }

    WITH_META(lclBoxedRadioRow)

    virtual QHBoxLayout* hlayout() const { return m_layout; }
    QGroupBox* groupbox() const { return m_groupbox; }

private:
    QHBoxLayout* m_layout;
    QGroupBox* m_groupbox;
};

class lclColumn : public lclGui {
public:
    lclColumn(const tile_t& tile);
    lclColumn(const lclColumn& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclColumn() { delete m_layout; }

    WITH_META(lclColumn)

    virtual QVBoxLayout* vlayout() const { return m_layout; }

private:
    QVBoxLayout* m_layout;
};

class lclBoxedColumn : public lclGui {
public:
    lclBoxedColumn(const tile_t& tile);
    lclBoxedColumn(const lclBoxedColumn& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclBoxedColumn() { delete m_layout; delete m_groupbox; }

    WITH_META(lclBoxedColumn)

    virtual QVBoxLayout* vlayout() const { return m_layout; }
    QGroupBox* groupbox() const { return m_groupbox; }

private:
    QVBoxLayout* m_layout;
    QGroupBox* m_groupbox;
};

class lclBoxedRadioColumn : public lclGui {
public:
    lclBoxedRadioColumn(const tile_t& tile);
    lclBoxedRadioColumn(const lclBoxedRadioColumn& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclBoxedRadioColumn() { delete m_layout; delete m_groupbox; }

    WITH_META(lclBoxedRadioColumn)

    virtual QVBoxLayout* vlayout() const { return m_layout; }
    QGroupBox* groupbox() const { return m_groupbox; }

private:
    QVBoxLayout* m_layout;
    QGroupBox* m_groupbox;
};

class lclImage : public lclGui {
public:
    lclImage(const tile_t& tile);
    lclImage(const lclImage& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclImage() { delete m_image; }

    WITH_META(lclImage)

    QLabel* image() const { return m_image; }

private:
    QLabel* m_image;
};

class lclPopupList : public lclGui {
public:
    lclPopupList(const tile_t& tile);
    lclPopupList(const lclPopupList& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclPopupList() { delete m_list; }

    WITH_META(lclPopupList)

    QComboBox* list() const { return m_list; }

private:
    QComboBox* m_list;
};

class lclSlider : public lclGui {

public:
    lclSlider(const tile_t& tile);
    lclSlider(const lclSlider& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclSlider() { delete m_slider; }

    WITH_META(lclSlider)

    QSlider* slider() const { return m_slider; }

    void valueChanged();

private:
    QSlider* m_slider;
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
    QSpacerItem* m_spacer;
};

class lclToggle : public lclGui {

public:
    lclToggle(const tile_t& tile);
    lclToggle(const lclToggle& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclToggle() { delete m_toggle; }

    WITH_META(lclToggle)

    QCheckBox* toggle() const { return m_toggle; }

    void stateChanged(int state);

private:
    QCheckBox* m_toggle;
};

class lclOkCancel : public lclGui {

public:
    lclOkCancel(const tile_t& tile);
    lclOkCancel(const lclOkCancel& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclOkCancel() { delete m_btnCancel; delete m_btnOk; }

    WITH_META(lclOkCancel)

    QPushButton* binOK() const { return m_btnOk; }
    QPushButton* btnCancel() const { return m_btnCancel; }
    virtual QHBoxLayout* hlayout() const { return m_layout; }

    void okClicked(bool checked);
    void cancelClicked(bool checked);

private:
    QPushButton* m_btnOk;
    QPushButton* m_btnCancel;
    QHBoxLayout* m_layout;
};

class lclOkCancelHelp : public lclGui {

public:
    lclOkCancelHelp(const tile_t& tile);
    lclOkCancelHelp(const lclOkCancelHelp& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclOkCancelHelp() { delete m_btnCancel; delete m_btnOk; delete m_btnHelp; }

    WITH_META(lclOkCancelHelp)

    QPushButton* binOK() const { return m_btnOk; }
    QPushButton* btnCancel() const { return m_btnCancel; }
    QPushButton* btnHelp() const { return m_btnHelp; }
    virtual QHBoxLayout* hlayout() const { return m_layout; }

    void okClicked(bool checked);
    void cancelClicked(bool checked);
    void helpClicked(bool checked);

private:
    QPushButton* m_btnOk;
    QPushButton* m_btnCancel;
    QPushButton* m_btnHelp;
    QHBoxLayout* m_layout;
};

class lclOkCancelHelpInfo : public lclGui {

public:
    lclOkCancelHelpInfo(const tile_t& tile);
    lclOkCancelHelpInfo(const lclOkCancelHelpInfo& that, lclValuePtr meta)
        : lclGui(that, meta) { }

    virtual ~lclOkCancelHelpInfo() { delete m_btnCancel; delete m_btnOk; delete m_btnHelp; delete m_btnInfo; }

    WITH_META(lclOkCancelHelpInfo)

    QPushButton* binOK() const { return m_btnOk; }
    QPushButton* btnCancel() const { return m_btnCancel; }
    QPushButton* btnHelp() const { return m_btnHelp; }
    QPushButton* btnInfo() const { return m_btnInfo; }
    virtual QHBoxLayout* hlayout() const { return m_layout; }

    void okClicked(bool checked);
    void cancelClicked(bool checked);
    void helpClicked(bool checked);
    void infoClicked(bool checked);

private:
    QPushButton* m_btnOk;
    QPushButton* m_btnCancel;
    QPushButton* m_btnHelp;
    QPushButton* m_btnInfo;
    QHBoxLayout* m_layout;
};

extern std::vector<const lclGui*> dclTiles;
extern void openTile(const lclGui* tile);

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
    lclValuePtr widget(const tile_t& tile);
    lclValuePtr boxedcolumn(const tile_t& tile);
    lclValuePtr boxedrow(const tile_t& tile);
    lclValuePtr boxedradiocolumn(const tile_t& tile);
    lclValuePtr boxedradiorow(const tile_t& tile);
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

    lclValuePtr image(const tile_t& tile);
    lclValuePtr slider(const tile_t& tile);
    lclValuePtr spacer(const tile_t& tile);
    lclValuePtr toggle(const tile_t& tile);

}

#endif // DEVELOPER

#endif // INCLUDE_TYPES_H
