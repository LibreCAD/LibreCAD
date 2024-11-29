#include "Debug.h"
#include "Environment.h"
#include "Types.h"
#include "lisp.h"

#ifdef DEVELOPER

#include <math.h>
#include <iostream>
#include <algorithm>
#include <memory>
#include <typeinfo>

#include <QObject>

static inline void replaceValue(std::string &com, const std::string& value);
static inline void replaceKey(std::string &com, const std::string& key);
static inline void replaceX(std::string &com, int x);
static inline void replaceY(std::string &com, int y);

namespace lcl {
    lclValuePtr atom(lclValuePtr value) {
        return lclValuePtr(new lclAtom(value));
    }

    lclValuePtr boolean(bool value) {
        return value ? trueValue() : falseValue();
    }

    lclValuePtr type(LCLTYPE type) {
        switch(type) {
            case LCLTYPE::ATOM:
                return typeAtom();
            case LCLTYPE::BUILTIN:
                return typeBuiltin();
            case LCLTYPE::FILE:
                return typeFile();
            case LCLTYPE::INT:
                return typeInteger();
            case LCLTYPE::LIST:
                return typeList();
            case LCLTYPE::MAP:
                return typeMap();
            case LCLTYPE::REAL:
                return typeReal();
            case LCLTYPE::STR:
                return typeString();
            case LCLTYPE::SYM:
                return typeSymbol();
            case LCLTYPE::VEC:
                return typeVector();
            case LCLTYPE::KEYW:
                return typeKeword();
            default:
                return typeUndef();
        }
    }

    lclValuePtr builtin(const String& name, lclBuiltIn::ApplyFunc handler) {
        return lclValuePtr(new lclBuiltIn(name, handler));
    }

    lclValuePtr builtin(bool eval, const String& name) {
        return lclValuePtr(new lclBuiltIn(eval, name));
    }

    lclValuePtr falseValue() {
        static lclValuePtr c(new lclConstant("false"));
        return lclValuePtr(c);
    }

    lclValuePtr file(const char *path, const char &mode)
    {
        return lclValuePtr(new lclFile(path, mode));
    }
#if 0
    lclValuePtr gui(const tile_t& tile) {
        return lclValuePtr(new lclGui(tile));
    }
#endif
    lclValuePtr hash(const lclHash::Map& map) {
        return lclValuePtr(new lclHash(map));
    }

    lclValuePtr hash(lclValueIter argsBegin, lclValueIter argsEnd,
                     bool isEvaluated) {
        return lclValuePtr(new lclHash(argsBegin, argsEnd, isEvaluated));
    }

    lclValuePtr integer(int64_t value) {
        return lclValuePtr(new lclInteger(value));
    };

    lclValuePtr integer(const String& token) {
        return integer(std::stoi(token));
    }

    lclValuePtr keyword(const String& token) {
        return lclValuePtr(new lclKeyword(token));
    }

    lclValuePtr lambda(const StringVec& bindings,
                       lclValuePtr body, lclEnvPtr env) {
        return lclValuePtr(new lclLambda(bindings, body, env));
    }

    lclValuePtr list(lclValueVec* items) {
        return lclValuePtr(new lclList(items));
    }

    lclValuePtr list(lclValueIter begin, lclValueIter end) {
        return lclValuePtr(new lclList(begin, end));
    }

    lclValuePtr list(lclValuePtr a) {
        lclValueVec* items = new lclValueVec(1);
        items->at(0) = a;
        return lclValuePtr(new lclList(items));
    }

    lclValuePtr list(lclValuePtr a, lclValuePtr b) {
        lclValueVec* items = new lclValueVec(2);
        items->at(0) = a;
        items->at(1) = b;
        return lclValuePtr(new lclList(items));
    }

    lclValuePtr list(lclValuePtr a, lclValuePtr b, lclValuePtr c) {
        lclValueVec* items = new lclValueVec(3);
        items->at(0) = a;
        items->at(1) = b;
        items->at(2) = c;
        return lclValuePtr(new lclList(items));
    }

    lclValuePtr macro(const lclLambda& lambda) {
        return lclValuePtr(new lclLambda(lambda, true));
    }

    lclValuePtr nilValue() {
        static lclValuePtr c(new lclConstant("nil"));
        return lclValuePtr(c);
    }

    lclValuePtr nullValue() {
        static lclValuePtr c(new lclConstant(""));
        return lclValuePtr(c);
    }

    lclValuePtr ldouble(double value)
    {
        return lclValuePtr(new lclDouble(value));
    }

    lclValuePtr ldouble(const String& token)
    {
        return ldouble(std::stof(token));
    }

    lclValuePtr piValue() {
        static lclValuePtr c(new lclDouble(M_PI));
        return lclValuePtr(c);
    }

    lclValuePtr string(const String& token) {
        return lclValuePtr(new lclString(token));
    }

    lclValuePtr symbol(const String& token) {
        return lclValuePtr(new lclSymbol(token));
    }

    lclValuePtr trueValue() {
        static lclValuePtr c(new lclConstant("true"));
        return lclValuePtr(c);
    }

    lclValuePtr typeAtom() {
        static lclValuePtr c(new lclConstant("ATOM"));
        return lclValuePtr(c);
    }

    lclValuePtr typeBuiltin() {
        static lclValuePtr c(new lclConstant("SUBR"));
        return lclValuePtr(c);
    }

    lclValuePtr typeFile() {
        static lclValuePtr c(new lclConstant("FILE"));
        return lclValuePtr(c);
    }

    lclValuePtr typeInteger() {
        static lclValuePtr c(new lclConstant("INT"));
        return lclValuePtr(c);
    }

    lclValuePtr typeList() {
        static lclValuePtr c(new lclConstant("LIST"));
        return lclValuePtr(c);
    }

    lclValuePtr typeMap() {
        static lclValuePtr c(new lclConstant("MAP"));
        return lclValuePtr(c);
    }

    lclValuePtr typeReal() {
        static lclValuePtr c(new lclConstant("REAL"));
        return lclValuePtr(c);
    };
    lclValuePtr typeString() {
        static lclValuePtr c(new lclConstant("STR"));
        return lclValuePtr(c);
    }

    lclValuePtr typeSymbol() {
        static lclValuePtr c(new lclConstant("SYM"));
        return lclValuePtr(c);
    }

    lclValuePtr typeUndef() {
        static lclValuePtr c(new lclConstant("UNDEF"));
        return lclValuePtr(c);
    }

    lclValuePtr typeVector() {
        static lclValuePtr c(new lclConstant("VEC"));
        return lclValuePtr(c);
    }

    lclValuePtr typeKeword() {
        static lclValuePtr c(new lclConstant("KEYW"));
        return lclValuePtr(c);
    }

    lclValuePtr vector(lclValueVec* items) {
        return lclValuePtr(new lclVector(items));
    }

    lclValuePtr vector(lclValueIter begin, lclValueIter end) {
        return lclValuePtr(new lclVector(begin, end));
    }

    lclValuePtr dclgui(const tile_t& tile) {
        return lclValuePtr(new lclDclGui(tile));
    }

    lclValuePtr widget(const tile_t& tile) {
        return lclValuePtr(new lclWidget(tile));
    }

    lclValuePtr boxedcolumn(const tile_t& tile) {
        return lclValuePtr(new lclBoxedColumn(tile));
    }

    lclValuePtr boxedrow(const tile_t& tile) {
        return lclValuePtr(new lclBoxedRow(tile));
    }

    lclValuePtr boxedradiocolumn(const tile_t& tile) {
        return lclValuePtr(new lclBoxedRadioColumn(tile));
    }

    lclValuePtr boxedradiorow(const tile_t& tile) {
        return lclValuePtr(new lclBoxedRadioRow(tile));
    }

    lclValuePtr button(const tile_t& tile) {
        return lclValuePtr(new lclButton(tile));
    }

    lclValuePtr column(const tile_t& tile) {
        return lclValuePtr(new lclColumn(tile));
    }

    lclValuePtr image(const tile_t& tile) {
        return lclValuePtr(new lclImage(tile));
    }

    lclValuePtr label(const tile_t& tile) {
        return lclValuePtr(new lclLabel(tile));
    }

    lclValuePtr edit(const tile_t& tile) {
        return lclValuePtr(new lclEdit(tile));
    }

    lclValuePtr listbox(const tile_t& tile) {
        return lclValuePtr(new lclListBox(tile));
    }

    lclValuePtr okcancel(const tile_t& tile) {
        return lclValuePtr(new lclOkCancel(tile));
    }

    lclValuePtr okcancelhelp(const tile_t& tile) {
        return lclValuePtr(new lclOkCancelHelp(tile));
    }

    lclValuePtr okcancelhelpinfo(const tile_t& tile) {
        return lclValuePtr(new lclOkCancelHelpInfo(tile));
    }

    lclValuePtr popuplist(const tile_t& tile) {
        return lclValuePtr(new lclPopupList(tile));
    }

    lclValuePtr radiobutton(const tile_t& tile) {
        return lclValuePtr(new lclRadioButton(tile));
    }

    lclValuePtr row(const tile_t& tile) {
        return lclValuePtr(new lclRow(tile));
    }

    lclValuePtr slider(const tile_t& tile) {
        return lclValuePtr(new lclSlider(tile));
    }

    lclValuePtr spacer(const tile_t& tile) {
        return lclValuePtr(new lclSpacer(tile));
    }

    lclValuePtr toggle(const tile_t& tile) {
        return lclValuePtr(new lclToggle(tile));
    }

}

lclValuePtr lclBuiltIn::apply(lclValueIter argsBegin,
                              lclValueIter argsEnd) const
{
    return m_handler(m_name, argsBegin, argsEnd);
}

static String makeHashKey(lclValuePtr key)
{
    if (const lclString* skey = DYNAMIC_CAST(lclString, key)) {
        return skey->print(true);
    }
    else if (const lclKeyword* kkey = DYNAMIC_CAST(lclKeyword, key)) {
        return kkey->print(true);
    }
    LCL_FAIL("'%s' is not a string or keyword", key->print(true).c_str());
}

static lclHash::Map addToMap(lclHash::Map& map,
    lclValueIter argsBegin, lclValueIter argsEnd)
{
    // This is intended to be called with pre-evaluated arguments.
    for (auto it = argsBegin; it != argsEnd; ++it) {
        String key = makeHashKey(*it++);
        map[key] = *it;
    }

    return map;
}

static lclHash::Map createMap(lclValueIter argsBegin, lclValueIter argsEnd)
{
    LCL_CHECK(std::distance(argsBegin, argsEnd) % 2 == 0,
            "hash-map requires an even-sized list");

    lclHash::Map map;
    return addToMap(map, argsBegin, argsEnd);
}

lclHash::lclHash(lclValueIter argsBegin, lclValueIter argsEnd, bool isEvaluated)
: m_map(createMap(argsBegin, argsEnd))
, m_isEvaluated(isEvaluated)
{

}

lclHash::lclHash(const lclHash::Map& map)
: m_map(map)
, m_isEvaluated(true)
{

}

lclValuePtr
lclHash::assoc(lclValueIter argsBegin, lclValueIter argsEnd) const
{
    LCL_CHECK(std::distance(argsBegin, argsEnd) % 2 == 0,
            "assoc requires an even-sized list");

    lclHash::Map map(m_map);
    return lcl::hash(addToMap(map, argsBegin, argsEnd));
}

bool lclHash::contains(lclValuePtr key) const
{
    auto it = m_map.find(makeHashKey(key));
    return it != m_map.end();
}

lclValuePtr
lclHash::dissoc(lclValueIter argsBegin, lclValueIter argsEnd) const
{
    lclHash::Map map(m_map);
    for (auto it = argsBegin; it != argsEnd; ++it) {
        String key = makeHashKey(*it);
        map.erase(key);
    }
    return lcl::hash(map);
}

lclValuePtr lclHash::eval(lclEnvPtr env)
{
    if (m_isEvaluated) {
        return lclValuePtr(this);
    }

    lclHash::Map map;
    for (auto it = m_map.begin(), end = m_map.end(); it != end; ++it) {
        map[it->first] = EVAL(it->second, env);
    }
    return lcl::hash(map);
}

lclValuePtr lclHash::get(lclValuePtr key) const
{
    auto it = m_map.find(makeHashKey(key));
    return it == m_map.end() ? lcl::nilValue() : it->second;
}

lclValuePtr lclHash::keys() const
{
    lclValueVec* keys = new lclValueVec();
    keys->reserve(m_map.size());
    for (auto it = m_map.begin(), end = m_map.end(); it != end; ++it) {
        if (it->first[0] == '"') {
            keys->push_back(lcl::string(unescape(it->first)));
        }
        else {
            keys->push_back(lcl::keyword(it->first));
        }
    }
    return lcl::list(keys);
}

lclValuePtr lclHash::values() const
{
    lclValueVec* keys = new lclValueVec();
    keys->reserve(m_map.size());
    for (auto it = m_map.begin(), end = m_map.end(); it != end; ++it) {
        keys->push_back(it->second);
    }
    return lcl::list(keys);
}

String lclHash::print(bool readably) const
{
    String s = "{";

    auto it = m_map.begin(), end = m_map.end();
    if (it != end) {
        s += it->first + " " + it->second->print(readably);
        ++it;
    }
    for ( ; it != end; ++it) {
        s += " " + it->first + " " + it->second->print(readably);
    }

    return s + "}";
}

bool lclHash::doIsEqualTo(const lclValue* rhs) const
{
    const lclHash::Map& r_map = static_cast<const lclHash*>(rhs)->m_map;
    if (m_map.size() != r_map.size()) {
        return false;
    }

    for (auto it0 = m_map.begin(), end0 = m_map.end(), it1 = r_map.begin();
         it0 != end0; ++it0, ++it1) {

        if (it0->first != it1->first) {
            return false;
        }
        if (!it0->second->isEqualTo(it1->second.ptr())) {
            return false;
        }
    }
    return true;
}

lclLambda::lclLambda(const StringVec& bindings,
                     lclValuePtr body, lclEnvPtr env)
: m_bindings(bindings)
, m_body(body)
, m_env(env)
, m_isMacro(false)
{
    //for (auto &it : m_bindings) { std::cout << "[lclLambda::lclLambda] bindings: " << it << std::endl; }
}

lclLambda::lclLambda(const lclLambda& that, lclValuePtr meta)
: lclApplicable(meta)
, m_bindings(that.m_bindings)
, m_body(that.m_body)
, m_env(that.m_env)
, m_isMacro(that.m_isMacro)
{

}

lclLambda::lclLambda(const lclLambda& that, bool isMacro)
: lclApplicable(that.m_meta)
, m_bindings(that.m_bindings)
, m_body(that.m_body)
, m_env(that.m_env)
, m_isMacro(isMacro)
{
    //std::cout << "[lclLambda::lclLambda] isMacro: " << (int)m_isMacro << std::endl;
    //std::cout << "[lclLambda::lclLambda] m_body: " << m_body->print(true) << std::endl;
    //for (auto &it : m_bindings) { std::cout << "[lclLambda::lclLambda] bindings: " << it << std::endl; }
}

lclValuePtr lclLambda::apply(lclValueIter argsBegin,
                             lclValueIter argsEnd) const
{
    //std::cout << "[lclLambda::apply] args count: " << std::distance(argsBegin, argsEnd) << std::endl;
    //std::cout << "[lclLambda::apply] EVAL body: " << m_body->print(true) << " args: ";
    //for (auto it = argsBegin; it != argsEnd; it++) { std::cout << " " << *it; }
    //std::cout << std::endl;
    return EVAL(m_body, makeEnv(argsBegin, argsEnd));
}

lclValuePtr lclLambda::doWithMeta(lclValuePtr meta) const
{
    return new lclLambda(*this, meta);
}

lclEnvPtr lclLambda::makeEnv(lclValueIter argsBegin, lclValueIter argsEnd) const
{
    //std::cout << "[lclLambda::makeEnv] args count: " << std::distance(argsBegin, argsEnd) << std::endl;
    //for (auto &it : m_bindings) { std::cout << "[lclLambda::makeEnv] bindings: " << it << std::endl; }
    return lclEnvPtr(new lclEnv(m_env, m_bindings, argsBegin, argsEnd));
}

lclValuePtr lclList::conj(lclValueIter argsBegin,
                          lclValueIter argsEnd) const
{
    int oldItemCount = std::distance(begin(), end());
    int newItemCount = std::distance(argsBegin, argsEnd);

    lclValueVec* items = new lclValueVec(oldItemCount + newItemCount);
    std::reverse_copy(argsBegin, argsEnd, items->begin());
    std::copy(begin(), end(), items->begin() + newItemCount);

    return lcl::list(items);
}

lclValuePtr lclList::eval(lclEnvPtr env)
{
    std::cout << "[lclLambda::eval]" << std::endl;
    // Note, this isn't actually called since the TCO updates, but
    // is required for the earlier steps, so don't get rid of it.
    if (count() == 0) {
        return lclValuePtr(this);
    }

    std::unique_ptr<lclValueVec> items(evalItems(env));
    auto it = items->begin();
    lclValuePtr op = *it;
    return APPLY(op, ++it, items->end());
}

String lclList::print(bool readably) const
{
    return '(' + lclSequence::print(readably) + ')';
}

lclValuePtr lclValue::eval(lclEnvPtr env)
{
    Q_UNUSED(env)
    // Default case of eval is just to return the object itself.
    return lclValuePtr(this);
}

bool lclValue::isEqualTo(const lclValue* rhs) const
{
    // Special-case. Vectors and Lists can be compared.
    bool matchingTypes = (typeid(*this) == typeid(*rhs)) ||
        (dynamic_cast<const lclSequence*>(this) &&
         dynamic_cast<const lclSequence*>(rhs))          ||
        (dynamic_cast<const lclInteger*>(this) &&
         dynamic_cast<const lclDouble*>(rhs))          ||
        (dynamic_cast<const lclDouble*>(this) &&
         dynamic_cast<const lclInteger*>(rhs));

    return matchingTypes && doIsEqualTo(rhs);
}

bool lclValue::isTrue() const
{
    return (this != lcl::falseValue().ptr())
        && (this != lcl::nilValue().ptr());
}

lclValuePtr lclValue::meta() const
{
    return m_meta.ptr() == NULL ? lcl::nilValue() : m_meta;
}

lclValuePtr lclValue::withMeta(lclValuePtr meta) const
{
    return doWithMeta(meta);
}

lclSequence::lclSequence(lclValueVec* items)
: m_items(items)
{

}

lclSequence::lclSequence(lclValueIter begin, lclValueIter end)
: m_items(new lclValueVec(begin, end))
{

}

lclSequence::lclSequence(const lclSequence& that, lclValuePtr meta)
: lclValue(meta)
, m_items(new lclValueVec(*(that.m_items)))
{

}

lclSequence::~lclSequence()
{
    delete m_items;
}

bool lclSequence::doIsEqualTo(const lclValue* rhs) const
{
    const lclSequence* rhsSeq = static_cast<const lclSequence*>(rhs);
    if (count() != rhsSeq->count()) {
        return false;
    }

    for (lclValueIter it0 = m_items->begin(),
                      it1 = rhsSeq->begin(),
                      end = m_items->end(); it0 != end; ++it0, ++it1) {

        if (! (*it0)->isEqualTo((*it1).ptr())) {
            return false;
        }
    }
    return true;
}

lclValueVec* lclSequence::evalItems(lclEnvPtr env) const
{
    lclValueVec* items = new lclValueVec;;
    items->reserve(count());
    for (auto it = m_items->begin(), end = m_items->end(); it != end; ++it) {
        items->push_back(EVAL(*it, env));
    }
    return items;
}

lclValuePtr lclSequence::first() const
{
    return count() == 0 ? lcl::nilValue() : item(0);
}

String lclSequence::print(bool readably) const
{
    String str;
    auto end = m_items->cend();
    auto it = m_items->cbegin();
    if (it != end) {
        str += (*it)->print(readably);
        ++it;
    }
    for ( ; it != end; ++it) {
        str += " ";
        str += (*it)->print(readably);
    }
    return str;
}

bool lclSequence::isDotted() const
{
    return ((count() == 3) && (m_items->at(1)->print(true).compare(".") == 0)) ? true : false;
}

lclValuePtr lclSequence::rest() const
{
    lclValueIter start = (count() > 0) ? begin() + 1 : end();
    return lcl::list(start, end());
}

lclValuePtr lclSequence::dotted() const
{
    return isDotted() == true ? item(2) : lcl::nilValue();
}

lclValuePtr lclSequence::reverse(lclValueIter argsBegin, lclValueIter argsEnd) const
{
    lclValueVec* items = new lclValueVec(std::distance(argsBegin, argsEnd));
    std::reverse_copy(argsBegin, argsEnd, items->begin());
    return lcl::list(items);
}
#if 0
lclValuePtr lclSequence::append(lclValueIter argsBegin,
                          lclValueIter argsEnd) const
#endif
lclValueVec* lclSequence::append(lclValueIter argsBegin,
                          lclValueIter argsEnd) const
{
    int oldItemCount = std::distance(begin(), end());
    int newItemCount = std::distance(argsBegin, argsEnd);

    lclValueVec* items = new lclValueVec(oldItemCount + newItemCount);
    std::copy(begin(), end(), items->begin());
    std::copy(argsBegin, argsEnd, items->begin() + oldItemCount);

    std::cout << "items: " << items << std::endl;

    //return lcl::list(items);
    return items;
}

String lclString::escapedValue() const
{
    return escape(value());
}

String lclString::print(bool readably) const
{
    return readably ? escapedValue() : value();
}

lclValuePtr lclSymbol::eval(lclEnvPtr env)
{
    return env->get(value());
}

lclValuePtr lclVector::conj(lclValueIter argsBegin,
                            lclValueIter argsEnd) const
{
    int oldItemCount = std::distance(begin(), end());
    int newItemCount = std::distance(argsBegin, argsEnd);

    lclValueVec* items = new lclValueVec(oldItemCount + newItemCount);
    std::copy(begin(), end(), items->begin());
    std::copy(argsBegin, argsEnd, items->begin() + oldItemCount);

    return lcl::vector(items);
}

lclValuePtr lclVector::eval(lclEnvPtr env)
{
    return lcl::vector(evalItems(env));
}

String lclVector::print(bool readably) const
{
    return '[' + lclSequence::print(readably) + ']';
}

bool lclInteger::doIsEqualTo(const lclValue* rhs) const
{
    return m_value == static_cast<const lclInteger*>(rhs)->m_value
    || double(m_value) == static_cast<const lclDouble*>(rhs)->value();
}

bool lclDouble::doIsEqualTo(const lclValue* rhs) const
{
    return m_value == static_cast<const lclDouble*>(rhs)->m_value
    || m_value == double(static_cast<const lclInteger*>(rhs)->value());
}

lclValuePtr lclFile::open()
{
    switch(m_mode)
    {
        case 'w':
            m_value = fopen(m_path.c_str(), "w");
            break;

        case 'r':
            m_value = fopen(m_path.c_str(), "r");
            break;

        case 'a':
            m_value = fopen(m_path.c_str(), "a");
            break;
        default:
            return lcl::nilValue();
    }

    if (m_value == NULL)
    {
        return lcl::nilValue();
    }
    return this;
}

lclValuePtr lclFile::close()
{
    LCL_CHECK(fclose(m_value) == 0,
              "i/o can not close file");
    return lcl::nilValue();
}

lclValuePtr lclFile::writeLine (const String &line)
{
    LCL_CHECK(fprintf(m_value, "%s\n", line.c_str()) > 0,
              "i/o can not write to file");

    return lcl::string(line);
}

lclValuePtr lclFile::writeChar(const char &c)
{
    LCL_CHECK(fputc(c, m_value) > -1,
              "i/o can not write to file");
    return lcl::integer(c);
}

lclValuePtr lclFile::readLine()
{
    char buf[8192];
    if (fgets(buf, sizeof(buf), m_value))
    {
         return lcl::string(buf);
    }
    return lcl::nilValue();
}

lclValuePtr lclFile::readChar()
{
    return lcl::integer(fgetc(m_value));
}

lclValuePtr lclGui::conj(lclValueIter argsBegin,
                lclValueIter argsEnd) const
{
    Q_UNUSED(argsBegin)
    Q_UNUSED(argsEnd)
    return lcl::dclgui(m_value);
}

lclWidget::lclWidget(const tile_t& tile)
    : lclGui(tile)
    , m_widget(new QWidget)
    , m_layout(new QVBoxLayout(m_widget))
{
    if(noQuotes(tile.label).empty())
    {
        m_widget->setWindowTitle(QObject::tr("LibreCAD [unnamed dialog]"));
    }
    else
    {
        m_widget->setWindowTitle(noQuotes(tile.label).c_str());
    }

    m_widget->setWindowFlags( Qt::Dialog | Qt::WindowTitleHint );
    m_widget->setLayout(m_layout);
}

lclButton::lclButton(const tile_t& tile)
    : lclGui(tile)
    , m_button(new QPushButton)
//    , m_vlayout(new QVBoxLayout)
//    , m_hlayout(new QHBoxLayout)
{
    m_button->setText(noQuotes(tile.label).c_str());
    m_button->setDefault(tile.is_default);

    if(int(tile.width))
    {
        m_button->setMinimumWidth(int(tile.width * 10));
    }

    if(int(tile.height))
    {
        m_button->setMinimumHeight(int(tile.height * 10));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_button->setFixedWidth(int(tile.width * 10));
        }
        else
        {
            m_button->setFixedWidth(80);
        }

    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_button->setMinimumHeight(int(tile.height * 10));
        }
        else
        {
            m_button->setFixedWidth(32);
        }
    }

    QObject::connect(m_button, QOverload<bool>::of(&QPushButton::clicked), [&](bool checked) { clicked(checked); });
}

void lclButton::clicked(bool checked)
{
    Q_UNUSED(checked)
    qDebug() << "lclButton::clicked key:" << this->value().key.c_str();
    qDebug() << "lclButton::clicked key:" << noQuotes(this->value().key).c_str();

    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_" + noQuotes(this->value().key).c_str());
    qDebug() << "lclButton::clicked val:" << val->print(true).c_str();
    if (val->print(true).compare("nil") != 0) {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        lclValuePtr action_expr = lcl::string(action);
        qDebug() << "lclButton::clicked action_expr:" << action_expr->print(true).c_str();
        LispRun_SimpleString(action.c_str());
    }
}

lclRadioButton::lclRadioButton(const tile_t& tile)
    : lclGui(tile)
    , m_button(new QRadioButton)
//    , m_vlayout(new QVBoxLayout)
//    , m_hlayout(new QHBoxLayout)
{
    m_button->setText(noQuotes(tile.label).c_str());

    if(int(tile.width))
    {
        m_button->setMinimumWidth(int(tile.width * 10));
    }

    if(int(tile.height))
    {
        m_button->setMinimumHeight(int(tile.height * 10));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_button->setFixedWidth(int(tile.width * 10));
        }
        else
        {
            m_button->setFixedWidth(80);
        }

    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_button->setMinimumHeight(int(tile.height * 10));
        }
        else
        {
            m_button->setFixedWidth(32);
        }
    }

    QObject::connect(m_button, QOverload<bool>::of(&QPushButton::clicked), [&](bool checked) { clicked(checked); });
}

void lclRadioButton::clicked(bool checked)
{
    qDebug() << "lclRadioButton::clicked checked:" << checked;
    qDebug() << "lclRadioButton::clicked key:" << noQuotes(this->value().key).c_str();
    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_" + noQuotes(this->value().key).c_str());
    if (val->print(true).compare("nil") != 0) {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        lclValuePtr action_expr = lcl::string(action);
        qDebug() << "lclButton::clicked action_expr:" << action_expr->print(true).c_str();
        LispRun_SimpleString(action.c_str());
    }
}

lclRow::lclRow(const tile_t& tile)
    : lclGui(tile)
    , m_layout(new QHBoxLayout)
{
#if 0
    if(int(tile.width))
    {
        m_layout->setMinimumWidth(int(tile.width * 10));
    }

    if(int(tile.height))
    {
        m_layout->setMinimumHeight(int(tile.height * 10));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_layout->setFixedWidth(int(tile.width * 10));
        }
        else
        {
            m_layout->setFixedWidth(80);
        }

    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_layout->setMinimumHeight(int(tile.height * 10));
        }
        else
        {
            m_layout->setFixedWidth(32);
        }
    }
#endif
}

lclBoxedColumn::lclBoxedColumn(const tile_t& tile)
    : lclGui(tile)
    , m_layout(new QVBoxLayout)
    , m_groupbox(new QGroupBox)
{
    m_groupbox->setTitle(noQuotes(tile.label).c_str());
    m_groupbox->setStyleSheet("QGroupBox { border: 1px solid silver; border-radius: 5px;margin-top: 5px; }"
                                " QGroupBox::title { subcontrol-origin: margin;left: 5px;padding: -10px 2px 0px 2px;}" );
    //m_layout->addStretch(1);
    m_groupbox->setLayout(m_layout);
#if 0
    if(int(tile.width))
    {
        m_groupbox->setMinimumWidth(int(tile.width * 10));
    }

    if(int(tile.height))
    {
        m_groupbox->setMinimumHeight(int(tile.height * 10));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_groupbox->setFixedWidth(int(tile.width * 10));
        }
        else
        {
            m_groupbox->setFixedWidth(80);
        }
    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_groupbox->setMinimumHeight(int(tile.height * 10));
        }
        else
        {
            m_groupbox->setFixedWidth(32);
        }
    }
#endif
}

lclBoxedRow::lclBoxedRow(const tile_t& tile)
    : lclGui(tile)
    , m_layout(new QHBoxLayout)
    , m_groupbox(new QGroupBox)
{
    m_groupbox->setTitle(noQuotes(tile.label).c_str());
    m_groupbox->setStyleSheet("QGroupBox { border: 1px solid silver; border-radius: 5px;margin-top: 5px; }"
                              " QGroupBox::title { subcontrol-origin: margin;left: 5px;padding: -10px 2px 0px 2px;}" );
    //m_layout->addStretch(1);
    m_groupbox->setLayout(m_layout);
#if 0
    if(int(tile.width))
    {
        m_groupbox->setMinimumWidth(int(tile.width * 10));
    }

    if(int(tile.height))
    {
        m_groupbox->setMinimumHeight(int(tile.height * 10));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_groupbox->setFixedWidth(int(tile.width * 10));
        }
        else
        {
            m_groupbox->setFixedWidth(80);
        }
    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_groupbox->setMinimumHeight(int(tile.height * 10));
        }
        else
        {
            m_groupbox->setFixedWidth(32);
        }
    }
#endif
}

lclBoxedRadioColumn::lclBoxedRadioColumn(const tile_t& tile)
    : lclGui(tile)
    , m_layout(new QVBoxLayout)
    , m_groupbox(new QGroupBox)
{
    m_groupbox->setTitle(noQuotes(tile.label).c_str());
    m_groupbox->setStyleSheet("QGroupBox { border: 1px solid silver; border-radius: 5px;margin-top: 5px; }"
                              " QGroupBox::title { subcontrol-origin: margin;left: 5px;padding: -10px 2px 0px 2px;}" );
    //m_layout->addStretch(1);
    m_groupbox->setLayout(m_layout);
#if 0
    if(int(tile.width))
    {
        m_groupbox->setMinimumWidth(int(tile.width * 10));
    }

    if(int(tile.height))
    {
        m_groupbox->setMinimumHeight(int(tile.height * 10));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_groupbox->setFixedWidth(int(tile.width * 10));
        }
        else
        {
            m_groupbox->setFixedWidth(80);
        }
    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_groupbox->setMinimumHeight(int(tile.height * 10));
        }
        else
        {
            m_groupbox->setFixedWidth(32);
        }
    }
#endif
}

lclBoxedRadioRow::lclBoxedRadioRow(const tile_t& tile)
    : lclGui(tile)
    , m_layout(new QHBoxLayout)
    , m_groupbox(new QGroupBox)
{
    m_groupbox->setTitle(noQuotes(tile.label).c_str());
    m_groupbox->setStyleSheet("QGroupBox { border: 1px solid silver; border-radius: 5px;margin-top: 5px; }"
                              " QGroupBox::title { subcontrol-origin: margin;left: 5px;padding: -10px 2px 0px 2px;}" );
    //m_layout->addStretch(1);
    m_groupbox->setLayout(m_layout);
#if 0
    if(int(tile.width))
    {
        m_groupbox->setMinimumWidth(int(tile.width * 10));
    }

    if(int(tile.height))
    {
        m_groupbox->setMinimumHeight(int(tile.height * 10));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_groupbox->setFixedWidth(int(tile.width * 10));
        }
        else
        {
            m_groupbox->setFixedWidth(80);
        }
    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_groupbox->setMinimumHeight(int(tile.height * 10));
        }
        else
        {
            m_groupbox->setFixedWidth(32);
        }
    }
#endif
}

lclColumn::lclColumn(const tile_t& tile)
    : lclGui(tile)
    , m_layout(new QVBoxLayout)
{
#if 0
    if(int(tile.width))
    {
        m_layout->setMinimumWidth(int(tile.width * 10));
    }

    if(int(tile.height))
    {
        m_layout->setMinimumHeight(int(tile.height * 10));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_layout->setFixedWidth(int(tile.width * 10));
        }
        else
        {
            m_layout->setFixedWidth(80);
        }

    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_layout->setMinimumHeight(int(tile.height * 10));
        }
        else
        {
            m_layout->setFixedWidth(32);
        }
    }
#endif
}

lclLabel::lclLabel(const tile_t& tile)
    : lclGui(tile)
    , m_label(new QLabel)
{
    m_label->setText(noQuotes(tile.label).c_str());

    if (tile.is_bold)
    {
        m_label->setStyleSheet("font-weight: bold");
    }

    if(int(tile.width))
    {
        m_label->setMinimumWidth(int(tile.width));
    }

    if(int(tile.height))
    {
        m_label->setMinimumHeight(int(tile.height));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_label->setFixedWidth(int(tile.width));
        }
        else
        {
            m_label->setFixedWidth(m_label->width());
        }

    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_label->setMinimumHeight(int(tile.height));
        }
        else
        {
            m_label->setFixedWidth(m_label->height());
        }
    }

    switch (tile.alignment)
    {
        case LEFT:
            m_label->setAlignment(Qt::AlignLeft);
            break;
        case RIGHT:
            m_label->setAlignment(Qt::AlignRight);
            break;
        case TOP:
            m_label->setAlignment(Qt::AlignTop);
            break;
        case BOTTOM:
            m_label->setAlignment(Qt::AlignBottom);
            break;
        case CENTERED:
            m_label->setAlignment(Qt::AlignCenter);
            break;
        default: {}
            break;
    }
}

lclPopupList::lclPopupList(const tile_t& tile)
    : lclGui(tile)
    , m_list(new QComboBox)
{
    if(int(tile.width))
    {
        m_list->setMinimumWidth(int(tile.width));
    }

    if(int(tile.height))
    {
        m_list->setMinimumHeight(int(tile.height));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_list->setFixedWidth(int(tile.width));
        }
        else
        {
            m_list->setFixedWidth(m_list->width());
        }
    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_list->setMinimumHeight(int(tile.height));
        }
        else
        {
            m_list->setFixedWidth(m_list->height());
        }
    }
}

lclEdit::lclEdit(const tile_t& tile)
    : lclGui(tile)
    , m_edit(new QLineEdit)
{
    //[signal] void QLineEdit::textEdited(const QString &text)
    m_edit->setText(noQuotes(tile.label).c_str());
    if(int(tile.width))
    {
        m_edit->setMinimumWidth(int(tile.width));
    }

    if(int(tile.height))
    {
        m_edit->setMinimumHeight(int(tile.height));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_edit->setFixedWidth(int(tile.width));
        }
        else
        {
            m_edit->setFixedWidth(m_edit->width());
        }
    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_edit->setMinimumHeight(int(tile.height));
        }
        else
        {
            m_edit->setFixedWidth(m_edit->height());
        }
    }
    QObject::connect(m_edit, QOverload<const QString&>::of(&QLineEdit::textEdited), [&](const QString& text) { textEdited(text); });
}

void lclEdit::textEdited(const QString &text)
{
    qDebug() << "lclEdit::textEdited text:" << text;
    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_" + noQuotes(this->value().key).c_str());
    qDebug() << "lclEdit::textEdited val:" << val->print(true).c_str();
    if (val->print(true).compare("nil") != 0) {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        replaceValue(action, text.toStdString());
        qDebug() << "lclEdit::textEdited action:" << action.c_str();
        LispRun_SimpleString(action.c_str());
    }
}

lclListBox::lclListBox(const tile_t& tile)
    : lclGui(tile)
    , m_list(new QListWidget)
{
    //[signal] void QListWidget::currentTextChanged(const QString &currentText)
    if(int(tile.width))
    {
        m_list->setMinimumWidth(int(tile.width));
    }

    if(int(tile.height))
    {
        m_list->setMinimumHeight(int(tile.height));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_list->setFixedWidth(int(tile.width));
        }
        else
        {
            m_list->setFixedWidth(m_list->width());
        }
    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_list->setMinimumHeight(int(tile.height));
        }
        else
        {
            m_list->setFixedWidth(m_list->height());
        }
    }
    QObject::connect(m_list, QOverload<const QString&>::of(&QListWidget::currentTextChanged), [&](const QString& currentText) { currentTextChanged(currentText); });
}

void lclListBox::currentTextChanged(const QString &currentText)
{
    qDebug() << "lclListBox::currentTextChanged currentText:" << currentText;
    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_" + noQuotes(this->value().key).c_str());
    if (val->print(true).compare("nil") != 0)
    {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        replaceValue(action, currentText.toStdString());
        qDebug() << "lclListBox::currentTextChanged action:" << action.c_str();
        LispRun_SimpleString(action.c_str());
    }
}

lclSlider::lclSlider(const tile_t& tile)
    : lclGui(tile)
    , m_slider(new QSlider)
{
    if(int(tile.width))
    {
        m_slider->setMinimumWidth(int(tile.width * 10));
    }

    if(int(tile.height))
    {
        m_slider->setMinimumHeight(int(tile.height * 10));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_slider->setFixedWidth(int(tile.width * 10));
        }
        else
        {
            m_slider->setFixedWidth(80);
        }

    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_slider->setMinimumHeight(int(tile.height * 10));
        }
        else
        {
            m_slider->setFixedWidth(32);
        }
    }

    QObject::connect(m_slider, &QSlider::valueChanged, [&] { valueChanged(); });
}

void lclSlider::valueChanged()
{
    qDebug() << "lclSlider::valueChanged key:" << this->value().key.c_str();
    qDebug() << "lclSlider::valueChanged key:" << noQuotes(this->value().key).c_str();

    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_" + noQuotes(this->value().key).c_str());
    qDebug() << "lclSlider::valueChanged val:" << val->print(true).c_str();
    if (val->print(true).compare("nil") != 0) {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        replaceValue(action, std::to_string(m_slider->tickInterval()).c_str());
        LispRun_SimpleString(action.c_str());
    }
}

lclSpacer::lclSpacer(const tile_t& tile)
    : lclGui(tile)
    , m_spacer(new QSpacerItem(int(tile.width * 10), int(tile.height * 10)))
{
    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_spacer->widget()->setFixedWidth(int(tile.width * 10));
        }
        else
        {
            m_spacer->widget()->setFixedWidth(80);
        }

    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_spacer->widget()->setFixedHeight(int(tile.height * 10));
        }
        else
        {
            m_spacer->widget()->setFixedHeight(32);
        }
    }

    switch (tile.alignment)
    {
    case LEFT:
        m_spacer->setAlignment(Qt::AlignLeft);
        break;
    case RIGHT:
        m_spacer->setAlignment(Qt::AlignRight);
        break;
    case TOP:
        m_spacer->setAlignment(Qt::AlignTop);
        break;
    case BOTTOM:
        m_spacer->setAlignment(Qt::AlignBottom);
        break;
    case CENTERED:
        m_spacer->setAlignment(Qt::AlignCenter);
        break;
    case HORIZONTAL:
        m_spacer->setAlignment(Qt::AlignHCenter);
        break;
    case VERTICAL:
        m_spacer->setAlignment(Qt::AlignVCenter);
        break;
    default:
        break;
    }
}

lclToggle::lclToggle(const tile_t& tile)
    : lclGui(tile)
    , m_toggle(new QCheckBox)
{
    m_toggle->setText(noQuotes(tile.label).c_str());

    if(int(tile.width))
    {
        m_toggle->setMinimumWidth(int(tile.width * 10));
    }

    if(int(tile.height))
    {
        m_toggle->setMinimumHeight(int(tile.height * 10));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_toggle->setFixedWidth(int(tile.width * 10));
        }
        else
        {
            m_toggle->setFixedWidth(80);
        }

    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_toggle->setMinimumHeight(int(tile.height * 10));
        }
        else
        {
            m_toggle->setFixedWidth(32);
        }
    }

    QObject::connect(m_toggle, QOverload<int>::of(&QCheckBox::stateChanged), [&](int state) { stateChanged(state); });
}

void lclToggle::stateChanged(int state)
{
    Q_UNUSED(state)
    qDebug() << "lclToggle::stateChanged key:" << this->value().key.c_str();
    qDebug() << "lclToggle::stateChanged key:" << noQuotes(this->value().key).c_str();

    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_" + noQuotes(this->value().key).c_str());
    qDebug() << "lclToggle::stateChanged val:" << val->print(true).c_str();
    if (val->print(true).compare("nil") != 0) {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        replaceValue(action, std::to_string(state).c_str());
        LispRun_SimpleString(action.c_str());
    }
}

lclImage::lclImage(const tile_t& tile)
    : lclGui(tile)
    , m_image(new QLabel)
{
    if (tile.is_bold)
    {
        m_image->setStyleSheet("font-weight: bold");
    }

    if(int(tile.width))
    {
        m_image->setMinimumWidth(int(tile.width));
    }

    if(int(tile.height))
    {
        m_image->setMinimumHeight(int(tile.height));
    }

    if (tile.fixed_width)
    {
        if(int(tile.width)) {
            m_image->setFixedWidth(int(tile.width));
        }
        else
        {
            m_image->setFixedWidth(m_image->width());
        }

    }

    if (tile.fixed_height)
    {
        if(int(tile.height))
        {
            m_image->setMinimumHeight(int(tile.height));
        }
        else
        {
            m_image->setFixedWidth(m_image->height());
        }
    }

    switch (tile.alignment)
    {
    case LEFT:
        m_image->setAlignment(Qt::AlignLeft);
        break;
    case RIGHT:
        m_image->setAlignment(Qt::AlignRight);
        break;
    case TOP:
        m_image->setAlignment(Qt::AlignTop);
        break;
    case BOTTOM:
        m_image->setAlignment(Qt::AlignBottom);
        break;
    case CENTERED:
        m_image->setAlignment(Qt::AlignCenter);
        break;
    default: {}
    break;
    }
}

lclOkCancel::lclOkCancel(const tile_t& tile)
    : lclGui(tile)
    , m_btnOk(new QPushButton)
    , m_btnCancel(new QPushButton)
    , m_layout(new QHBoxLayout)
{
    m_btnOk->setText(QObject::tr("OK"));
    m_btnCancel->setText(QObject::tr("Cancel"));
    m_btnOk->setDefault(true);

    m_btnOk->setMinimumWidth(80);
    m_btnCancel->setMinimumWidth(80);

    m_layout->addWidget(m_btnOk);
    m_layout->addWidget(m_btnCancel);

    QObject::connect(m_btnOk, QOverload<bool>::of(&QPushButton::clicked), [&](bool checked) { okClicked(checked); });
    QObject::connect(m_btnCancel, QOverload<bool>::of(&QPushButton::clicked), [&](bool checked) { cancelClicked(checked); });
}

void lclOkCancel::okClicked(bool checked)
{
    Q_UNUSED(checked)
    qDebug() << "lclOkCancel::clicked key:" << this->value().key.c_str();
    qDebug() << "lclOkCancel::clicked key:" << noQuotes(this->value().key).c_str();

    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_accept");
    qDebug() << "lclOkCancel::clicked val:" << val->print(true).c_str();
    if (val->print(true).compare("nil") != 0)
    {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        qDebug() << "lclOkCancel::okClicked action:" << action.c_str();
        LispRun_SimpleString(action.c_str());
    }
}

void lclOkCancel::cancelClicked(bool checked)
{
    Q_UNUSED(checked)
    qDebug() << "lclOkCancel::cancelClicked key:" << this->value().key.c_str();
    qDebug() << "lclOkCancel::cancelClicked key:" << noQuotes(this->value().key).c_str();

    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_cancel");
    qDebug() << "lclOkCancel::cancelClicked val:" << val->print(true).c_str();
    if (val->print(true).compare("nil") != 0)
    {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        qDebug() << "lclOkCancel::okClicked action:" << action.c_str();
        LispRun_SimpleString(action.c_str());
    }
}

lclOkCancelHelp::lclOkCancelHelp(const tile_t& tile)
    : lclGui(tile)
    , m_btnOk(new QPushButton)
    , m_btnCancel(new QPushButton)
    , m_btnHelp(new QPushButton)
    , m_layout(new QHBoxLayout)
{
    m_btnOk->setText(QObject::tr("OK"));
    m_btnCancel->setText(QObject::tr("Cancel"));
    m_btnHelp->setText(QObject::tr("Help"));
    m_btnOk->setDefault(tile.is_default);

    m_btnOk->setMinimumWidth(80);
    m_btnCancel->setMinimumWidth(80);
    m_btnHelp->setMinimumWidth(80);

    m_layout->addWidget(m_btnOk);
    m_layout->addWidget(m_btnCancel);
    m_layout->addWidget(m_btnHelp);

    QObject::connect(m_btnOk, QOverload<bool>::of(&QPushButton::clicked), [&](bool checked) { okClicked(checked); });
    QObject::connect(m_btnCancel, QOverload<bool>::of(&QPushButton::clicked), [&](bool checked) { cancelClicked(checked); });
    QObject::connect(m_btnHelp, QOverload<bool>::of(&QPushButton::clicked), [&](bool checked) { helpClicked(checked); });
}

void lclOkCancelHelp::okClicked(bool checked)
{
    Q_UNUSED(checked)
    qDebug() << "lclOkCancelHelp::okClicked key:" << this->value().key.c_str();
    qDebug() << "lclOkCancelHelp::okClicked key:" << noQuotes(this->value().key).c_str();

    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_accept");
    qDebug() << "lclOkCancelHelp::okClicked val:" << val->print(true).c_str();
    if (val->print(true).compare("nil") != 0)
    {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        qDebug() << "lclOkCancel::okClicked action:" << action.c_str();
        LispRun_SimpleString(action.c_str());
    }
}

void lclOkCancelHelp::cancelClicked(bool checked)
{
    Q_UNUSED(checked)
    qDebug() << "lclOkCancelHelp::cancelClicked key:" << this->value().key.c_str();
    qDebug() << "lclOkCancelHelp::cancelClicked key:" << noQuotes(this->value().key).c_str();

    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_cancel");
    qDebug() << "lclOkCancelHelp::cancelClicked val:" << val->print(true).c_str();
    if (val->print(true).compare("nil") != 0)
    {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        qDebug() << "lclOkCancel::cancelClicked action:" << action.c_str();
        LispRun_SimpleString(action.c_str());
    }
}

void lclOkCancelHelp::helpClicked(bool checked)
{
    Q_UNUSED(checked)
    qDebug() << "lclOkCancelHelp::helpClicked key:" << this->value().key.c_str();
    qDebug() << "lclOkCancelHelp::helpClicked key:" << noQuotes(this->value().key).c_str();

    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_help");
    qDebug() << "lclOkCancelHelp::helpClicked val:" << val->print(true).c_str();
    if (val->print(true).compare("nil") != 0)
    {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        qDebug() << "lclOkCancel::helpClicked action:" << action.c_str();
        LispRun_SimpleString(action.c_str());
    }
}

lclOkCancelHelpInfo::lclOkCancelHelpInfo(const tile_t& tile)
    : lclGui(tile)
    , m_btnOk(new QPushButton)
    , m_btnCancel(new QPushButton)
    , m_btnHelp(new QPushButton)
    , m_btnInfo(new QPushButton)
    , m_layout(new QHBoxLayout)
{
    m_btnOk->setText(QObject::tr("OK"));
    m_btnCancel->setText(QObject::tr("Cancel"));
    m_btnHelp->setText(QObject::tr("Help"));
    m_btnInfo->setText(QObject::tr("Info"));
    m_btnOk->setDefault(tile.is_default);

    m_btnOk->setMinimumWidth(80);
    m_btnCancel->setMinimumWidth(80);
    m_btnHelp->setMinimumWidth(80);
    m_btnInfo->setMinimumWidth(80);

    m_layout->addWidget(m_btnOk);
    m_layout->addWidget(m_btnCancel);
    m_layout->addWidget(m_btnHelp);
    m_layout->addWidget(m_btnInfo);

    QObject::connect(m_btnOk, QOverload<bool>::of(&QPushButton::clicked), [&](bool checked) { okClicked(checked); });
    QObject::connect(m_btnCancel, QOverload<bool>::of(&QPushButton::clicked), [&](bool checked) { cancelClicked(checked); });
    QObject::connect(m_btnHelp, QOverload<bool>::of(&QPushButton::clicked), [&](bool checked) { helpClicked(checked); });
    QObject::connect(m_btnInfo, QOverload<bool>::of(&QPushButton::clicked), [&](bool checked) { helpClicked(checked); });
}

void lclOkCancelHelpInfo::okClicked(bool checked)
{
    Q_UNUSED(checked)
    qDebug() << "lclOkCancelHelpInfo::okClicked key:" << this->value().key.c_str();
    qDebug() << "lclOkCancelHelpInfo::okClicked key:" << noQuotes(this->value().key).c_str();

    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_accept");
    qDebug() << "lclOkCancelHelpInfo::okClicked val:" << val->print(true).c_str();
    if (val->print(true).compare("nil") != 0)
    {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        qDebug() << "lclOkCancel::okClicked action:" << action.c_str();
        LispRun_SimpleString(action.c_str());
    }
}

void lclOkCancelHelpInfo::cancelClicked(bool checked)
{
    Q_UNUSED(checked)
    qDebug() << "lclOkCancelHelpInfo::cancelClicked key:" << this->value().key.c_str();
    qDebug() << "lclOkCancelHelpInfo::cancelClicked key:" << noQuotes(this->value().key).c_str();

    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_cancel");
    qDebug() << "lclOkCancelHelpInfo::cancelClicked val:" << val->print(true).c_str();
    if (val->print(true).compare("nil") != 0)
    {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        qDebug() << "lclOkCancel::cancelClicked action:" << action.c_str();
        LispRun_SimpleString(action.c_str());
    }
}

void lclOkCancelHelpInfo::helpClicked(bool checked)
{
    Q_UNUSED(checked)
    qDebug() << "lclOkCancelHelpInfo::helpClicked key:" << this->value().key.c_str();
    qDebug() << "lclOkCancelHelpInfo::helpClicked key:" << noQuotes(this->value().key).c_str();

    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_help");
    qDebug() << "lclOkCancelHelpInfo::helpClicked val:" << val->print(true).c_str();
    if (val->print(true).compare("nil") != 0)
    {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        qDebug() << "lclOkCancel::helpClicked action:" << action.c_str();
        LispRun_SimpleString(action.c_str());
    }
}

void lclOkCancelHelpInfo::infoClicked(bool checked)
{
    Q_UNUSED(checked)
    qDebug() << "lclOkCancelHelpInfo::infoClicked key:" << this->value().key.c_str();
    qDebug() << "lclOkCancelHelpInfo::infoClicked key:" << noQuotes(this->value().key).c_str();

    lclValuePtr val = dclEnv->get(std::to_string(this->value().dialog_Id) + "_info");
    qDebug() << "lclOkCancelHelpInfo::infoClicked val:" << val->print(true).c_str();
    if (val->print(true).compare("nil") != 0)
    {
        const lclString *str = VALUE_CAST(lclString, val);
        String action = "(do";
        action += str->value();
        action += ")";
        qDebug() << "lclOkCancel::helpClicked action:" << action.c_str();
        LispRun_SimpleString(action.c_str());
    }
}


static inline void replaceValue(std::string &com, const std::string& value)
{
    while(com.find("$value") != std::string::npos) {
        com.replace(com.find("$value"),6, "\"" + value + "\"");
    }
}

static inline void replaceKey(std::string &com, const std::string& key)
{
    while(com.find("$key") != std::string::npos) {
        com.replace(com.find("$key"),4, "\"" + key + "\"");
    }
}

static inline void replaceX(std::string &com, int x)
{
    while(com.find("$x") != std::string::npos) {
        com.replace(com.find("$x"),2, std::to_string(x));
    }
}

static inline void replaceY(std::string &com, int y)
{
    while(com.find("$y") != std::string::npos) {
        com.replace(com.find("$y"),2, std::to_string(y));
    }
}

#endif // DEVELOPER
