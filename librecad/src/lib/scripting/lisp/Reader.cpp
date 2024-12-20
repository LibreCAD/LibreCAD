#include "LCL.h"
#include "Types.h"

#ifdef DEVELOPER

#include <regex>
#include <fstream>
#include <iostream>
#include <string.h>

#include <QDebug>
#include <QObject>

typedef std::regex              Regex;

static const Regex intRegex("^[-+]?\\d+$");
static const Regex floatRegex("^[+-]?\\d+[.]{1}\\d+$");
static const Regex closeRegex("[\\)\\]}]");
static const Regex dclRegex("^[A-Za-z0-9_]+");

static const Regex whitespaceRegex("[\\s,]+|;.*|//.*");
static const Regex tokenRegexes[] = {
    Regex("~ "),
    Regex("~@"),
    Regex("[\\[\\]{}()'`~^@]"),
    Regex("\"(?:\\\\.|[^\\\\\"])*\""),
    Regex("[^\\s\\[\\]{}('\"`,;)]+"),
};

int dclId = -1;

std::vector<tile_t> dclProtoTile;
std::vector<LclAlias_t> LclCom;

class Tokeniser
{
public:
    Tokeniser(const String& input);

    String peek() const {
        ASSERT(!eof(), "Tokeniser reading past EOF in peek\n");
        return m_token;
    }

    String next() {
        ASSERT(!eof(), "Tokeniser reading past EOF in next\n");
        String ret = peek();
        nextToken();
        return ret;
    }

    bool eof() const {
        return m_iter == m_end;
    }

private:
    void skipWhitespace();
    void nextToken();

    bool matchRegex(const Regex& regex);

    typedef String::const_iterator StringIter;

    String      m_token;
    String      m_lastToken = "";
    StringIter  m_iter;
    StringIter  m_end;
};

Tokeniser::Tokeniser(const String& input)
:   m_iter(input.begin())
,   m_end(input.end())
{
    nextToken();
}

bool Tokeniser::matchRegex(const Regex& regex)
{
    if (eof()) {
        return false;
    }

    std::smatch match;
    auto flags = std::regex_constants::match_continuous;
    if (!std::regex_search(m_iter, m_end, match, regex, flags)) {
        return false;
    }

    ASSERT(match.size() == 1, "Should only have one submatch, not %lu\n",
                              match.size());
    ASSERT(match.position(0) == 0, "Need to match first character\n");
    ASSERT(match.length(0) > 0, "Need to match a non-empty string\n");

    // Don't advance  m_iter now, do it after we've consumed the token in
    // next().  If we do it now, we hit eof() when there's still one token left.
    m_token = match.str(0);

    return true;
}

void Tokeniser::nextToken()
{
    m_iter += m_token.size();

    skipWhitespace();
    if (eof()) {
        return;
    }

    for (auto &it : tokenRegexes) {
        if (matchRegex(it)) {
            return;
        }
    }

    String mismatch(m_iter, m_end);
    if (mismatch[0] == '"') {
        LCL_CHECK(false, "expected '\"', got EOF");
    }
    else {
        LCL_CHECK(false, "unexpected '%s'", mismatch.c_str());
    }
}

void Tokeniser::skipWhitespace()
{
    while (matchRegex(whitespaceRegex)) {
        m_iter += m_token.size();
    }
}

static lclValuePtr readAtom(Tokeniser& tokeniser);
static lclValuePtr readForm(Tokeniser& tokeniser);
static void readList(Tokeniser& tokeniser, lclValueVec* items,
                      const String& end);
static lclValuePtr processMacro(Tokeniser& tokeniser, const String& symbol);

lclValuePtr readStr(const String& input)
{
    Tokeniser tokeniser(input);
    if (tokeniser.eof()) {
        throw lclEmptyInputException();
    }
    return readForm(tokeniser);
}

static void readTile(Tokeniser& tokeniser, tile_t& tile);
static void copyTile(const tile_t &a, tile_t &b);
static bool ends_with(const std::string& str, const std::string& suffix);
static bool isLclAlias(const String& alias);
static const String lclCom(const String& alias);
static bool getDclBool(const String& str);
static bool isdclAttribute(const String& str);
#if 0
static bool isdclTile(const String& str);
#endif
static pos_t getDclPos(const String& str);
static tile_id_t getDclId(const String& str);
static int getDclColor(const String& str);
static lclValuePtr readDclFile(Tokeniser& tokeniser, bool start = false, bool parent=false);
static lclValuePtr addTile(tile_t tile);

lclValuePtr loadDcl(const String& path)
{
    std::ifstream file(path.c_str());
    String data;
    data.reserve(file.tellg());
    file.seekg(0, std::ios::beg);
    data.append(std::istreambuf_iterator<char>(file.rdbuf()),
    std::istreambuf_iterator<char>());

    data += "}"; // closing 'virtuell container'

    Tokeniser tokeniser(data);
    if (tokeniser.eof()) {
        throw lclEmptyInputException();
    }
    return readDclFile(tokeniser, true, false);
}

static lclValuePtr readForm(Tokeniser& tokeniser)
{
    LCL_CHECK(!tokeniser.eof(), "expected form, got EOF");
    String token = tokeniser.peek();

    LCL_CHECK(!std::regex_match(token, closeRegex),
            "unexpected '%s'", token.c_str());

    if (token == "(") {
        tokeniser.next();
        std::unique_ptr<lclValueVec> items(new lclValueVec);
        readList(tokeniser, items.get(), ")");
        return lcl::list(items.release());
    }
    if (token == "[") {
        tokeniser.next();
        std::unique_ptr<lclValueVec> items(new lclValueVec);
        readList(tokeniser, items.get(), "]");
        return lcl::vector(items.release());
    }
    if (token == "{") {
        tokeniser.next();
        lclValueVec items;
        readList(tokeniser, &items, "}");
        return lcl::hash(items.begin(), items.end(), false);
    }
    return readAtom(tokeniser);
}

static lclValuePtr readAtom(Tokeniser& tokeniser)
{
    struct ReaderMacro {
        const char* token;
        const char* symbol;
    };
    ReaderMacro macroTable[] = {
        { "@",   "deref" },
        { "`",   "quasiquote" },
        { "'",   "quote" },
        { "~@",  "splice-unquote" },
        { "~",   "unquote" },
    };

    struct Constant {
        const char* token;
        lclValuePtr value;
    };
    Constant constantTable[] = {
        { "false",      lcl::falseValue()  },
        { "nil",        lcl::nilValue()    },
        { "true",       lcl::trueValue()   },
        { "T",          lcl::trueValue()   },
        { "pi",         lcl::piValue()     },
        { "!false",     lcl::falseValue()  },
        { "!nil",       lcl::nilValue()    },
        { "!true",      lcl::trueValue()   },
        { "!T",         lcl::trueValue()   },
        { "!pi",        lcl::piValue()     },
        { "ATOM",       lcl::typeAtom()    },
        { "FILE",       lcl::typeFile()    },
        { "INT",        lcl::typeInteger() },
        { "LIST",       lcl::typeList()    },
        { "REAL",       lcl::typeReal()    },
        { "STR",        lcl::typeString()  },
        { "VEC",        lcl::typeVector()  },
        { "KEYW",       lcl::typeKeword()  }
    };

    String token = tokeniser.next();
    if (token[0] == '"') {
        return lcl::string(unescape(token));
    }
    if (token[0] == ':') {
        return lcl::keyword(token);
    }
    if (token == "^") {
        lclValuePtr meta = readForm(tokeniser);
        lclValuePtr value = readForm(tokeniser);
        // Note that meta and value switch places
        return lcl::list(lcl::symbol("with-meta"), value, meta);
    }

    for (Constant  &constant : constantTable) {
        if (token == constant.token) {
            return constant.value;
        }
    }
    for (auto &macro : macroTable) {
        if (token == macro.token) {
            return processMacro(tokeniser, macro.symbol);
        }
    }
    if (std::regex_match(token, intRegex)) {
        return lcl::integer(token);
    }
    if (std::regex_match(token, floatRegex)) {
        return lcl::ldouble(token);
    }
    if (token[0] == '!') {
        return lcl::symbol(token.erase(0, 1));
    }
    if (isLclAlias(token)) {
        return lcl::list(lcl::symbol(lclCom(token)));
    }
    return lcl::symbol(token);
}

static void readList(Tokeniser& tokeniser, lclValueVec* items,
                      const String& end)
{
    while (1) {
        LCL_CHECK(!tokeniser.eof(), "expected '%s', got EOF", end.c_str());
        if (tokeniser.peek() == end) {
            tokeniser.next();
            return;
        }
        items->push_back(readForm(tokeniser));
    }
}

static lclValuePtr processMacro(Tokeniser& tokeniser, const String& symbol)
{
    return lcl::list(lcl::symbol(symbol), readForm(tokeniser));
}

static bool isLclAlias(const String& alias)
{
    for (auto & com : LclCom)
    {
        if (com.alias == alias)
        {
            return true;
        }
    }
    return false;
}

static const String lclCom(const String& alias)
{
    for (auto & com : LclCom)
    {
        if (com.alias == alias)
        {
            return com.command;
        }
    }
    return String("");
}

static lclValuePtr readDclFile(Tokeniser& tokeniser, bool start, bool parent)
{
    LCL_CHECK(!tokeniser.eof(), "expected form, got EOF");
    String token = tokeniser.peek();
    int i;

    if (start) {
        dclId++;
        tile_t tile;
        tile.name = "#<dcl-tiles>";
        tile.tiles = new lclValueVec;
        readTile(tokeniser, tile);
        qDebug() << "readDclFile " << tile.name.c_str();
        return lcl::dclgui(tile); // dclId
    }

    if (token.compare(":") == 0) {
        tokeniser.next();
        token = tokeniser.peek();
        qDebug() << "readDclFile(): ':' Dcl Name: " << tokeniser.peek().c_str();
        i = 0;
        for (auto &it : dclProtoTile) {
            if (token == it.name) {
                qDebug() << "readDclFile(): got proto Dcl Tile..." << tokeniser.peek().c_str();
                tokeniser.next();
                tile_t tile;
                copyTile(dclProtoTile[i], tile);
                tile.has_parent = parent;
                tokeniser.next();
                readTile(tokeniser, tile);
                return addTile(tile);
            }
            i++;
        }
        for (auto &it : dclTile) {
            if (ends_with(token, it.name)) {
                qDebug() << "readDclFile(): known Dcl Tile..." << tokeniser.peek().c_str();
                tokeniser.next();
                tile_t tile;
                tile.name = token;
                tile.id = getDclId(token);
                tile.has_parent = parent;
                tile.tiles = new lclValueVec;
                token = tokeniser.peek();
                tokeniser.next();
                readTile(tokeniser, tile);
                //qDebug() << "Debug" << tile.label.c_str();
                return addTile(tile);
            }
            if (token == it.name) {
                qDebug() << "readDclFile(): real Dcl Tile..." << tokeniser.peek().c_str();
                tokeniser.next();
                tile_t tile;
                tile.name = token;
                tile.id = getDclId(token);
                tile.has_parent = parent;
                tile.tiles = new lclValueVec;
                token = tokeniser.peek();
                tokeniser.next();
                readTile(tokeniser, tile);
                return addTile(tile);
            }
        }
    }

    if (std::regex_match(tokeniser.peek(), dclRegex)) {
        token = tokeniser.peek();
        qDebug() << "readDclFile(): 'dclRegex' Dcl Name: " << tokeniser.peek().c_str();
        for (auto &it : dclTile) { //errortile, spacer, ...
            if (token == it.name) {
                qDebug() << "readDclFile(): 'dclTile': " << tokeniser.peek().c_str();
                tile_t tile;
                tile.name = token;
                tile.id = getDclId(token);
                tile.has_parent = parent;
                tile.tiles = new lclValueVec(0);
                token = tokeniser.peek();
                tokeniser.next();
                //std::cout << "readDclFile(): 'dclTile' next: " << tokeniser.peek() << std::endl;
                qDebug() << "readDclFile(): 'dclTile' next: " << tokeniser.peek().c_str();
                return addTile(tile);
            }
        }
        tokeniser.next();
        tokeniser.next();
        tile_t tile;
        tile.name = token;
        tile.id = getDclId(tokeniser.peek());
        tile.has_parent = parent;
        tile.tiles = new lclValueVec;
        tokeniser.next();
        readTile(tokeniser, tile);
        for (auto &it : dclTile) {
            if (ends_with(token, String("_").append(it.name))) {
                std::cout <<"readDclFile(): ("<< tile.name <<" = proto) ";
                dclProtoTile.push_back(tile);
                return NULL;
            }
        }
        qDebug() << "readDclFile(): 'dclTile' bottom" << tokeniser.peek().c_str();
        return addTile(tile);
    }
    std::cout << "X-( : " << tokeniser.peek() << std::endl;
    return NULL;
}

static void readTile(Tokeniser& tokeniser, tile_t& tile)
{
    qDebug() << "[readTile] >>>>> START FOR >>>>>" <<"("<<tile.name.c_str()<<")";
    String token;
    int parent = true;

    tile.dialog_Id = dclId;

    if (tile.id == DIALOG) {
        parent = false;
    }

    while (1) {
        qDebug() <<"[readTile] ("<<tile.name.c_str()<<") AT TOP";

        if (tokeniser.eof()) {
            return;
        }

        if (tokeniser.peek() == "}") {
            //qDebug() <<"("<<tile.name.c_str()<<") - end";
            //qDebug() << " ... } -> bye bye...";
            tokeniser.next();
            qDebug() << "[readTile] <<<<< END FOR <<<<" <<"("<<tile.name.c_str()<<")";
            return;
        }

        if (tokeniser.peek() == "{") {
            qDebug() << "[readTile] ("<<tile.name.c_str()<<") open bracet - start";
            qDebug() << " -> { ...";
            tokeniser.next();
            continue;
        }

        if (tokeniser.peek() == ":") {
            qDebug() << "[readTile] ("<<tile.name.c_str()<<") ";
            qDebug() << "[readTile] ':' [<- readDclFile(tokeniser, false) token: " << tokeniser.peek().c_str();
            tile.tiles->push_back(readDclFile(tokeniser, false, parent));
        }

        qDebug() << "[readTile] isdclAttribute for: " <<"("<<tile.name.c_str()<<") ?";
        if (isdclAttribute(tokeniser.peek())) {

            qDebug() << "got attribute...";
            token = tokeniser.peek();
            tokeniser.next();
            tokeniser.next();
            switch (getDclAttributeId(token)) {
                case ACTION:
                    tile.action = tokeniser.peek();
                    break;
                case ALIGNMENT:
                    tile.alignment = getDclPos(tokeniser.peek());
                    break;
                case ALLOW_ACCEPT:
                    tile.allow_accept = getDclBool(tokeniser.peek());
                    break;
                case ASPECT_RATIO:
                    tile.aspect_ratio = atof(tokeniser.peek().c_str());
                    break;
                case BIG_INCREMENT:
                    tile.big_increment = atoi(tokeniser.peek().c_str());
                    break;
                case CHILDREN_ALIGNMENT:
                    tile.children_alignment = getDclPos(tokeniser.peek());
                    break;
                case CHILDREN_FIXED_HEIGHT:
              tile.children_fixed_height = getDclBool(tokeniser.peek());
                    break;
                case CHILDREN_FIXED_WIDTH:
                    tile.children_fixed_width = getDclBool(tokeniser.peek());
                    break;
                case COLOR:
            {
                int color = 0;
                if (std::regex_match(tokeniser.peek(), intRegex)) {
                    if (((color < 257) && (color > -1)) ||
                        ((color < 0) && (color > -1002)))
                    {
                        color = std::stoi(tokeniser.peek());
                    }
                        tile.color = color;
                }
                else
                {
                        tile.color = getDclColor(tokeniser.peek());
                }
            }
                    break;
                case EDIT_LIMIT:
                    tile.edit_limit = atoi(tokeniser.peek().c_str());
                    break;
                case EDIT_WIDTH:
                    tile.edit_width = atof(tokeniser.peek().c_str());
                    break;
                case FIXED_HEIGHT:
                    tile.fixed_height = getDclBool(tokeniser.peek());
                    break;
                case FIXED_WIDTH:
                    tile.fixed_width = getDclBool(tokeniser.peek());
                    break;
                case FIXED_WIDTH_FONT:
                    tile.fixed_width_font = getDclBool(tokeniser.peek());
                    break;
                case HEIGHT:
                    tile.height = atof(tokeniser.peek().c_str());
                    break;
                case INITIAL_FOCUS:
                    tile.initial_focus = tokeniser.peek();
                    break;
                case IS_BOLD:
                    tile.is_bold = getDclBool(tokeniser.peek());
                    break;
                case IS_CANCEL:
                    tile.is_cancel = getDclBool(tokeniser.peek());
                    break;
                case IS_DEFAULT:
                    tile.is_default = getDclBool(tokeniser.peek());
                    break;
                case IS_ENABLED:
                    tile.is_enabled = getDclBool(tokeniser.peek());
                    break;
                case IS_TAB_STOP:
                    tile.is_tab_stop = getDclBool(tokeniser.peek());
                    break;
                case KEY:
                    tile.key = tokeniser.peek();
                    break;
                case LABEL:
                    tile.label = tokeniser.peek();
                    break;
                case LAYOUT:
                    tile.layout = getDclPos(tokeniser.peek());
                    break;
                case LIST:
                    tile.list = tokeniser.peek();
                    break;
                case MAX_VALUE:
                    tile.max_value = atoi(tokeniser.peek().c_str());
                    break;
                case MIN_VALUE:
                    tile.min_value = atoi(tokeniser.peek().c_str());
                    break;
                case MNEMONIC:
                    tile.mnemonic = tokeniser.peek();
                    break;
                case MULTIPLE_SELECT:
                    tile.multiple_select = getDclBool(tokeniser.peek());
                    break;
                case PASSWORD_CHAR:
                    tile.password_char = tokeniser.peek();
                    break;
                case SMALL_INCREMENT:
                    tile.small_increment = atoi(tokeniser.peek().c_str());
                    break;
                case TABS:
                    tile.tabs = tokeniser.peek();
                    break;
                case TAB_TRUNCATE:
                    tile.tab_truncate = getDclBool(tokeniser.peek());
                    break;
                case VALUE:
                    tile.value = tokeniser.peek();
                    break;
                case WIDTH:
                    tile.width = atof(tokeniser.peek().c_str());
                    break;
                default:
                    break;
            }
            qDebug() << token.c_str() << " = " << tokeniser.peek().c_str();
            tokeniser.next();
            continue;
        }
        else {
            qDebug() << " no attribute";
        }

        if (std::regex_match(tokeniser.peek(), dclRegex)) {
            qDebug() << "[readTile] ("<<tile.name.c_str()<<") ";
            qDebug() << "[readTile] 'dclRegex' [<- readDclFile(tokeniser, false) token: " << tokeniser.peek().c_str();
            lclValuePtr result = readDclFile(tokeniser, false, parent);
            if (result) {
                tile.tiles->push_back(result);
            }
        }
        qDebug() <<"[readTile] ("<<tile.name.c_str()<<") AT BOTTOM";
    }
}

static bool ends_with(const std::string& str, const std::string& suffix)
{
    return str.size() >= suffix.size() && str.compare(str.size()-suffix.size(), suffix.size(), suffix) == 0;
}
#if 0
static bool isdclTile(const String& str) {
    for (int i =0; i < MAX_DCL_TILES; i++) {
        if (str == dclTile[i].name) {
            return true;
        }
    }
    return false;
}
#endif
static tile_id_t getDclId(const String& str)
{
    tile_id_t id = NONE;
    for (int i = 0; i < MAX_DCL_TILES; i++) {
        if (str == dclTile[i].name) {
            return dclTile[i].id;
        }
    }
    return id;
}

static bool isdclAttribute(const String& str) {
    for (int i = 0; i < MAX_DCL_ATTR; i++) {
        if (str == dclAttribute[i].name) {
            return true;
        }
    }
    return false;
}

static pos_t getDclPos(const String& str)
{
    pos_t pos = NOPOS;
    for (int i = 0; i < MAX_DCL_POS; i++) {
        if (str == dclPosition[i].name) {
            return dclPosition[i].pos;
        }
    }
    return pos;
}

static int getDclColor(const String& str)
{
    color_t color = WHITE;
    for (int i = 0; i < MAX_DCL_COLOR; i++) {
        if (str == dclColor[i].name) {
            return dclColor[i].color;
        }
    }
    return color;
}

static bool getDclBool(const String& str)
{
    bool val = false;
    if (str == "true") {
        return true;
    }
    if (str == "false") {
        return false;
    }

    return val;
}

static void copyTile(const tile_t &a, tile_t &b)
{
    b.tiles = new lclValueVec(a.tiles->size());
    std::copy(a.tiles->begin(), a.tiles->end(), b.tiles->begin());
    b.action                 = a.action;
    b.alignment              = a.alignment;
    b.allow_accept           = a.allow_accept;
    b.aspect_ratio           = a.aspect_ratio;
    b.big_increment          = a.big_increment;
    b.children_alignment     = a.children_alignment;
    b.children_fixed_height  = a.children_fixed_height;
    b.children_fixed_width   = a.children_fixed_width;
    b.color                  = a.color;
    b.edit_limit             = a.edit_limit;
    b.edit_width             = a.edit_width;
    b.fixed_height           = a.fixed_height;
    b.fixed_width            = a.fixed_width;
    b.fixed_width_font       = a.fixed_width_font;
    b.height                 = a.height;
    b.initial_focus          = a.initial_focus;
    b.id                     = a.id;
    b.is_bold                = a.is_bold;
    b.is_cancel              = a.is_cancel;
    b.is_default             = a.is_default;
    b.is_enabled             = a.is_enabled;
    b.is_tab_stop            = a.is_tab_stop;
    b.key                    = a.key;
    b.label                  = a.label;
    b.layout                 = a.layout;
    b.list                   = a.list;
    b.max_value              = a.max_value;
    b.min_value              = a.min_value;
    b.mnemonic               = a.mnemonic;
    b.multiple_select        = a.multiple_select;
    b.name                   = a.name;
    b.password_char          = a.password_char;
    b.small_increment        = a.small_increment;
    b.tabs                   = a.tabs;
    b.tab_truncate           = a.tab_truncate;
    b.value                  = a.value;
    b.width                  = a.width;
}

static lclValuePtr addTile(tile_t tile)
{
    qDebug() << __func__ << tile.name.c_str();

    switch(tile.id) {
        case BOXED_COLUMN:
        case BOXED_RADIO_COLUMN:
            return lcl::boxedcolumn(tile);
        case BOXED_ROW:
        case BOXED_RADIO_ROW:
            return lcl::boxedrow(tile);
        case BUTTON:
            return lcl::button(tile);
        case COLUMN:
        case PARAGRAPH:
        case RADIO_COLUMN:
            return lcl::column(tile);
        case DIALOG:
            return lcl::dialog(tile);
        case EDIT_BOX:
            return lcl::edit(tile);
        case LIST_BOX:
            return lcl::listbox(tile);
        case ERRTILE:
        {
            tile.key = "\"error\"";
            return lcl::label(tile);
        }
        case IMAGE:
            return lcl::image(tile);
        case IMAGE_BUTTON:
            return lcl::imagebutton(tile);
        case OK_CANCEL:
        {
            tile.dialog_Id = dclId;
            return lcl::okcancel(tile);
        }
        case OK_CANCEL_HELP:
        {
            tile.dialog_Id = dclId;
            return lcl::okcancelhelp(tile);
        }
        case OK_CANCEL_HELP_ERRTILE:
        {
            tile.key = "\"error\"";
            tile.dialog_Id = dclId;
            return lcl::okcancelhelperrtile(tile);
        }
        case OK_CANCEL_HELP_INFO:
        {
            tile.dialog_Id = dclId;
            return lcl::okcancelhelpinfo(tile);
        }
        case OK_ONLY:
        {
            tile.key = "\"accept\"";
            tile.label = qUtf8Printable(QObject::tr("\"&Ok\""));
            tile.width = 8.0;
            tile.dialog_Id = dclId;
            return lcl::button(tile);
        }
        case POPUP_LIST:
            return lcl::popuplist(tile);
        case RADIO_BUTTON:
            return lcl::radiobutton(tile);
        case ROW:
        case RADIO_ROW:
        case CONCATENATION:
            return lcl::row(tile);
        case REGISTER:
            return lcl::tabwidget(tile);
        case SCROLL:
            return lcl::scroll(tile);
        case DIAL:
            return lcl::dial(tile);
        case SLIDER:
            return lcl::slider(tile);
        case SPACER:
            return lcl::spacer(tile);
        case SPACER_0:
        {
            tile.dialog_Id = dclId;
            return lcl::spacer(tile);
        }
        case SPACER_1:
        {
            tile.width = 1;
            tile.height = 1;
            tile.dialog_Id = dclId;
            return lcl::spacer(tile);
        }
        case TAB:
            return lcl::widget(tile);
        case TEXT:
        case TEXT_PART:
            return lcl::label(tile);
        case TOGGLE:
            return lcl::toggle(tile);
        default:
            return lcl::dclgui(tile);
    }
}

#endif // DEVELOPER
