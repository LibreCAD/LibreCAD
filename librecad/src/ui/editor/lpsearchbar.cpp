#include "lpsearchbar.h"
#include "ui_searchbarpower.h"
#include "ui_searchbarincremental.h"

#include "texteditor.h"

#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QStringListModel>
#include <QVBoxLayout>

#include <vector>
#include <QStack>

#include <QCompleter>
#include <QElapsedTimer>
#include <QMenu>

#include <QShortcut>
#include <QStringView>
#include <QClipboard>

#ifdef DEVELOPER

namespace
{
class AddMenuManager
{
private:
    QList<QString> m_insertBefore;
    QList<QString> m_insertAfter;
    QSet<QAction *> m_actionPointers;
    uint m_indexWalker;
    QMenu *m_menu;

public:
    AddMenuManager(QMenu *parent, int expectedItemCount)
        : m_insertBefore(QList<QString>(expectedItemCount))
        , m_insertAfter(QList<QString>(expectedItemCount))
        , m_indexWalker(0)
        , m_menu(nullptr)
    {
        Q_ASSERT(parent != nullptr);
        m_menu = parent->addMenu(QObject::tr("Add..."));
        if (m_menu == nullptr) {
            return;
        }
        m_menu->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    }

    void enableMenu(bool enabled)
    {
        if (m_menu == nullptr) {
            return;
        }
        m_menu->setEnabled(enabled);
    }

    void addEntry(const QString &before,
                  const QString &after,
                  const QString &description,
                  const QString &realBefore = QString(),
                  const QString &realAfter = QString())
    {
        if (m_menu == nullptr) {
            return;
        }
        QAction *const action = m_menu->addAction(before + after + QLatin1Char('\t') + description);
        m_insertBefore[m_indexWalker] = QString(realBefore.isEmpty() ? before : realBefore);
        m_insertAfter[m_indexWalker] = QString(realAfter.isEmpty() ? after : realAfter);
        action->setData(QVariant(m_indexWalker++));
        m_actionPointers.insert(action);
    }

    void addSeparator()
    {
        if (m_menu == nullptr) {
            return;
        }
        m_menu->addSeparator();
    }

    void handle(QAction *action, QLineEdit *lineEdit)
    {
        if (!m_actionPointers.contains(action)) {
            return;
        }

        const int cursorPos = lineEdit->cursorPosition();
        const int index = action->data().toUInt();
        const QString &before = m_insertBefore[index];
        const QString &after = m_insertAfter[index];
        lineEdit->insert(before + after);
        lineEdit->setCursorPosition(cursorPos + before.size());
        lineEdit->setFocus();
    }
};

} // anon namespace

LpSearchBar::LpSearchBar(QWidget *parent, Librepad *lpad)
    : QWidget(parent)
    , m_librePad(lpad)
    , m_centralWidget(new QWidget(this))
    , m_layout(new QVBoxLayout())
    , m_widget(nullptr)
    , m_incUi(nullptr)
    , m_powerUi(nullptr)
    , m_powerMode(0)
{
    QHBoxLayout *layout = new QHBoxLayout(this);

    // NOTE: Here be cosmetics.
    layout->setContentsMargins(0, 0, 0, 0);

    // widget to be used as parent for the real content
    layout->addWidget(m_centralWidget);
    setFocusProxy(m_centralWidget);

    // hide button
    m_closeButton = new QToolButton(this);
    closeButton()->setAutoRaise(true);
    closeButton()->setIcon(QIcon::fromTheme(QStringLiteral("dialog-close")));
    connect(closeButton(), &QToolButton::clicked, m_librePad, &Librepad::hideSearch);
#if 0
    layout->addWidget(closeButton());
    layout->setAlignment(closeButton(), Qt::AlignCenter | Qt::AlignVCenter);
#endif

    centralWidget()->setLayout(m_layout);
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_incMatchCase = m_librePad->incMatchCase();
    m_powerMatchCase = m_librePad->powerMatchCase();
    m_powerMode = m_librePad->powerMatchMode();
#if 0
    // Load one of either dialogs
    if (m_librePad->powerSearch())
    {
        enterPowerMode();
    } else
    {
        enterIncrementalMode();
    }
#endif
}

LpSearchBar::~LpSearchBar()
{
    m_widget->deleteLater();
    delete m_layout;
    delete m_closeButton;
    delete m_centralWidget;

    delete m_incUi;
    delete m_powerUi;
}

bool LpSearchBar::find(bool direction, bool reset)
{
    QString query = searchPattern();
    QRegularExpression re;

    if (query.trimmed().isEmpty())
    {
        return false;
    }
    if (m_librePad->editor() == nullptr)
    {
        return false;
    }

    QTextCursor cur  = m_librePad->editor()->textCursor();

    if (reset)
    {
        cur = m_librePad->editor()->textCursor();
        cur.movePosition(QTextCursor::Start);
        m_librePad->editor()->setTextCursor(cur);
    }

    switch (searchMode())
    {
        case MODE_WHOLE_WORDS:
        {
            if (direction)
            {
                if(matchCase())
                {
                    return m_librePad->editor()->find(query, QTextDocument::FindWholeWords | QTextDocument::FindCaseSensitively);
                }
                else{
                    return m_librePad->editor()->find(query, QTextDocument::FindWholeWords);
                }
            }
            else
            {
                if(matchCase())
                {
                    return m_librePad->editor()->find(query, QTextDocument::FindWholeWords | QTextDocument::FindBackward | QTextDocument::FindCaseSensitively);
                }
                else{
                    return m_librePad->editor()->find(query, QTextDocument::FindWholeWords | QTextDocument::FindBackward);
                }
            }
        }
            break;
#if 0
        case MODE_ESCAPE_SEQUENCES:
            break;
#endif
        case MODE_REGEX:
        {
            re = QRegularExpression(query);
            if (direction)
            {
                return m_librePad->editor()->find(re);
            }
            else
            {
               return m_librePad->editor()->find(re, QTextDocument::FindBackward);
            }
        }
            break;
        default:
        {
            if (direction)
            {
                if(matchCase())
                {
                    return m_librePad->editor()->find(query, QTextDocument::FindCaseSensitively);
                }
                else{
                    return m_librePad->editor()->find(query);
                }
            }
            else
            {
                if(matchCase())
                {
                    return m_librePad->editor()->find(query, QTextDocument::FindBackward | QTextDocument::FindCaseSensitively);
                }
                else{
                    return m_librePad->editor()->find(query, QTextDocument::FindBackward);
                }
            }
        }
            break;
    }
}

void LpSearchBar::findAll()
{
    m_matchCounter = 0;
    m_replaceMode = false;

    if (m_librePad->editor() == nullptr)
    {
        return;
    }

    QTextCursor cur  = m_librePad->editor()->textCursor();
    cur = m_librePad->editor()->textCursor();
    cur.movePosition(QTextCursor::Start);
    m_librePad->editor()->setTextCursor(cur);

    QList<QTextEdit::ExtraSelection> extraSelections;

    while(find(true, false))
    {
        QTextEdit::ExtraSelection extra;
        extra.format.setBackground(m_librePad->editor()->palette().highlight().color());
        extra.format.setForeground(m_librePad->editor()->palette().highlightedText().color());
        extra.cursor = m_librePad->editor()->textCursor();
        extraSelections.append(extra);
        ++m_matchCounter;
    }
    m_librePad->editor()->setExtraSelections(extraSelections);

    showResultMessage();
}

void LpSearchBar::replaceNext()
{
    // What to find/replace?
    const QString replacement = m_powerUi->replacement->currentText();

    if (m_librePad->editor() == nullptr)
    {
        return;
    }
    m_librePad->editor()->textCursor().insertText(replacement);
    find(true, false);
}

void LpSearchBar::replaceAll()
{
    // What to find/replace?
    const QString replacement = m_powerUi->replacement->currentText();
    m_matchCounter = 0;
    m_replaceMode = true;

    if (m_librePad->editor() == nullptr)
    {
        return;
    }

    QTextCursor cur  = m_librePad->editor()->textCursor();
    cur = m_librePad->editor()->textCursor();
    cur.movePosition(QTextCursor::Start);
    m_librePad->editor()->setTextCursor(cur);

    while(find(true, false))
    {
        m_librePad->editor()->textCursor().insertText(replacement);
        ++m_matchCounter;
    }

    showResultMessage();
}

void LpSearchBar::onPowerPatternChanged(const QString & /*pattern*/)
{
    givePatternFeedback();
#if 0
    indicateMatch(MatchNothing);
#endif
}

bool LpSearchBar::isPatternValid() const
{
    if (searchPattern().isEmpty()) {
        return false;
    }

    return m_powerUi->searchMode->currentIndex() == LpSearchBar::MODE_PLAIN_TEXT ? searchPattern().trimmed() == searchPattern() :
    m_powerUi->searchMode->currentIndex() == LpSearchBar::MODE_REGEX ?QRegularExpression(searchPattern(), QRegularExpression::UseUnicodePropertiesOption).isValid()
    : true;
}

void LpSearchBar::givePatternFeedback()
{
    // Enable/disable next/prev and replace next/all
    m_powerUi->findNext->setEnabled(isPatternValid());
    m_powerUi->findPrev->setEnabled(isPatternValid());
    m_powerUi->replaceNext->setEnabled(isPatternValid());
    m_powerUi->replaceAll->setEnabled(isPatternValid());
    m_powerUi->findAll->setEnabled(isPatternValid());
}

void LpSearchBar::addCurrentTextToHistory(QComboBox *combo)
{
    const QString text = combo->currentText();
    const int index = combo->findText(text);

    if (index > 0) {
        combo->removeItem(index);
    }

    if (index != 0) {
        combo->insertItem(0, text);
        combo->setCurrentIndex(0);
    }

    // sync to application config
    QStringList hist;

    for (int i = 0; i < combo->count(); i++)
    {
        hist.append(combo->itemText(i));
    }

    if(m_powerUi != nullptr)
    {
        m_librePad->writePowerFind(hist);
    }
    else
    {
        m_librePad->writeIncFind(hist);
    }
}

void LpSearchBar::backupConfig(bool ofPower)
{
    if (ofPower) {
        m_powerMatchCase = m_powerUi->matchCase->isChecked();
        m_powerMode = m_powerUi->searchMode->currentIndex();
    } else {
        m_incMatchCase = m_incUi->matchCase->isChecked();
    }
}

void LpSearchBar::findNext()
{
    const bool found = find(true, false);

    if (found) {
        QComboBox *combo = m_powerUi != nullptr ? m_powerUi->pattern : m_incUi->pattern;

        // Add to search history
        addCurrentTextToHistory(combo);
    }
}

void LpSearchBar::findPrevious()
{
    const bool found = find(false, false);

    if (found) {
        QComboBox *combo = m_powerUi != nullptr ? m_powerUi->pattern : m_incUi->pattern;

        // Add to search history
        addCurrentTextToHistory(combo);
    }
}

void LpSearchBar::showResultMessage()
{
    QString text;

    if (m_replaceMode) {
        if (m_matchCounter == 1)
        {
            text = QObject::tr("1 replacement made");
        }
        else
        {
            text = QObject::tr("%1 replacements made").arg(m_matchCounter);
        }

    } else {
        if (m_matchCounter == 1)
        {
            text = QObject::tr("1 match found");
        }
        else
        {
            text = QObject::tr("%1 matches found").arg(m_matchCounter);
        }
    }
    m_librePad->message(text);
}

void LpSearchBar::onMatchCaseToggled(bool matchCase)
{
    m_powerMatchCase = m_incMatchCase = matchCase;
    m_librePad->writePowerMatchCase(m_powerMatchCase);
    m_librePad->writeIncMatchCase(m_incMatchCase);
#if 0
    if (m_incUi != nullptr) {
        // Re-search with new settings
        const QString pattern = m_incUi->pattern->currentText();
        onIncPatternChanged(pattern);
    } else {
        indicateMatch(MatchNothing);
    }
#endif
}

bool LpSearchBar::matchCase() const
{
    return isPower() ? m_powerUi->matchCase->isChecked() : m_incUi->matchCase->isChecked();
}

LpSearchBar::SearchMode LpSearchBar::searchMode() const
{
    return (LpSearchBar::SearchMode) (isPower() ? m_powerUi->searchMode->currentIndex() : 0);
}

void LpSearchBar::onReturnPressed()
{
    const Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
    const bool shiftDown = (modifiers & Qt::ShiftModifier) != 0;
    const bool controlDown = (modifiers & Qt::ControlModifier) != 0;

    if (shiftDown) {
        // Shift down, search backwards
        findPrevious();
    } else {
        // Shift up, search forwards
        findNext();
    }

    if (controlDown)
    {
        hide();
    }
}

void LpSearchBar::showExtendedContextMenu(bool forPattern, const QPoint &pos)
{
    // Make original menu
    QComboBox *comboBox = forPattern ? m_powerUi->pattern : m_powerUi->replacement;
    QMenu *const contextMenu = comboBox->lineEdit()->createStandardContextMenu();

    if (contextMenu == nullptr) {
        return;
    }

    bool extendMenu = false;
    bool regexMode = false;
    switch (m_powerUi->searchMode->currentIndex()) {
    case MODE_REGEX:
        regexMode = true;
        // FALLTHROUGH
#if 0
    case MODE_ESCAPE_SEQUENCES:
        extendMenu = true;
        break;
#else
        break;
#endif

    default:
        break;
    }

    AddMenuManager addMenuManager(contextMenu, 37);
    if (!extendMenu) {
        addMenuManager.enableMenu(extendMenu);
    } else {
        // Build menu
        if (forPattern) {
            if (regexMode) {
                addMenuManager.addEntry(QStringLiteral("^"), QString(), QObject::tr("Beginning of line"));
                addMenuManager.addEntry(QStringLiteral("$"), QString(), QObject::tr("End of line"));
                addMenuManager.addSeparator();
                addMenuManager.addEntry(QStringLiteral("."), QString(), QObject::tr("Match any character excluding new line (by default)"));
                addMenuManager.addEntry(QStringLiteral("+"), QString(), QObject::tr("One or more occurrences"));
                addMenuManager.addEntry(QStringLiteral("*"), QString(), QObject::tr("Zero or more occurrences"));
                addMenuManager.addEntry(QStringLiteral("?"), QString(), QObject::tr("Zero or one occurrences"));
                addMenuManager.addEntry(QStringLiteral("{a"),
                                        QStringLiteral(",b}"),
                                        QObject::tr("<a> through <b> occurrences"),
                                        QStringLiteral("{"),
                                        QStringLiteral(",}"));

                addMenuManager.addSeparator();
                addMenuManager.addSeparator();
                addMenuManager.addEntry(QStringLiteral("("), QStringLiteral(")"), QObject::tr("Group, capturing"));
                addMenuManager.addEntry(QStringLiteral("|"), QString(), QObject::tr("Or"));
                addMenuManager.addEntry(QStringLiteral("["), QStringLiteral("]"), QObject::tr("Set of characters"));
                addMenuManager.addEntry(QStringLiteral("[^"), QStringLiteral("]"), QObject::tr("Negative set of characters"));
                addMenuManager.addSeparator();
            }
        } else {
            addMenuManager.addEntry(QStringLiteral("\\0"), QString(), QObject::tr("Whole match reference"));
            addMenuManager.addSeparator();
            if (regexMode) {
                const QString pattern = m_powerUi->pattern->currentText();
                const QList<QString> capturePatterns = getCapturePatterns(pattern);

                const int captureCount = capturePatterns.count();
                for (int i = 1; i <= 9; i++) {
                    const QString number = QString::number(i);
                    /* FIXME */
                    const QString &captureDetails =
                        (i <= captureCount) ? QLatin1String(" = (") + QStringView(capturePatterns[i - 1]).left(30).toLatin1() + QLatin1Char(')') : QString();

                    addMenuManager.addEntry(QLatin1String("\\") + number, QString(), QObject::tr("Reference") + QLatin1Char(' ') + number + captureDetails);
                }

                addMenuManager.addSeparator();
            }
        }

        addMenuManager.addEntry(QStringLiteral("\\n"), QString(), QObject::tr("Line break"));
        addMenuManager.addEntry(QStringLiteral("\\t"), QString(), QObject::tr("Tab"));

        if (forPattern && regexMode) {
            addMenuManager.addEntry(QStringLiteral("\\b"), QString(), QObject::tr("Word boundary"));
            addMenuManager.addEntry(QStringLiteral("\\B"), QString(), QObject::tr("Not word boundary"));
            addMenuManager.addEntry(QStringLiteral("\\d"), QString(), QObject::tr("Digit"));
            addMenuManager.addEntry(QStringLiteral("\\D"), QString(), QObject::tr("Non-digit"));
            addMenuManager.addEntry(QStringLiteral("\\s"), QString(), QObject::tr("Whitespace (excluding line breaks)"));
            addMenuManager.addEntry(QStringLiteral("\\S"), QString(), QObject::tr("Non-whitespace"));
            addMenuManager.addEntry(QStringLiteral("\\w"), QString(), QObject::tr("Word character (alphanumerics plus '_')"));
            addMenuManager.addEntry(QStringLiteral("\\W"), QString(), QObject::tr("Non-word character"));
        }

        addMenuManager.addEntry(QStringLiteral("\\0???"), QString(), QObject::tr("Octal character 000 to 377 (2^8-1)"), QStringLiteral("\\0"));
        addMenuManager.addEntry(QStringLiteral("\\x{????}"), QString(), QObject::tr("Hex character 0000 to FFFF (2^16-1)"), QStringLiteral("\\x{....}"));
        addMenuManager.addEntry(QStringLiteral("\\\\"), QString(), QObject::tr("Backslash"));

        if (forPattern && regexMode) {
            addMenuManager.addSeparator();
            addMenuManager.addEntry(QStringLiteral("(?:E"), QStringLiteral(")"), QObject::tr("Group, non-capturing"), QStringLiteral("(?:"));
            addMenuManager.addEntry(QStringLiteral("(?=E"), QStringLiteral(")"), QObject::tr("Positive Lookahead"), QStringLiteral("(?="));
            addMenuManager.addEntry(QStringLiteral("(?!E"), QStringLiteral(")"), QObject::tr("Negative lookahead"), QStringLiteral("(?!"));
            // variable length positive/negative lookbehind is an experimental feature in Perl 5.30
            // see: https://perldoc.perl.org/perlre.html
            // currently QRegularExpression only supports fixed-length positive/negative lookbehind (2020-03-01)
            addMenuManager.addEntry(QStringLiteral("(?<=E"), QStringLiteral(")"), QObject::tr("Fixed-length positive lookbehind"), QStringLiteral("(?<="));
            addMenuManager.addEntry(QStringLiteral("(?<!E"), QStringLiteral(")"), QObject::tr("Fixed-length negative lookbehind"), QStringLiteral("(?<!"));
        }

        if (!forPattern) {
            addMenuManager.addSeparator();
            addMenuManager.addEntry(QStringLiteral("\\L"), QString(), QObject::tr("Begin lowercase conversion"));
            addMenuManager.addEntry(QStringLiteral("\\U"), QString(), QObject::tr("Begin uppercase conversion"));
            addMenuManager.addEntry(QStringLiteral("\\E"), QString(), QObject::tr("End case conversion"));
            addMenuManager.addEntry(QStringLiteral("\\l"), QString(), QObject::tr("Lowercase first character conversion"));
            addMenuManager.addEntry(QStringLiteral("\\u"), QString(), QObject::tr("Uppercase first character conversion"));
            addMenuManager.addEntry(QStringLiteral("\\#[#..]"), QString(), QObject::tr("Replacement counter (for Replace All)"), QStringLiteral("\\#"));
        }
    }

    // Show menu
    QAction *const result = contextMenu->exec(comboBox->mapToGlobal(pos));
    if (result != nullptr) {
        addMenuManager.handle(result, comboBox->lineEdit());
    }
}

void LpSearchBar::onPowerModeChanged(int index)
{
    if (m_powerUi->searchMode->currentIndex() == MODE_REGEX) {
        m_powerUi->matchCase->setChecked(true);
    }
#if 0
    indicateMatch(MatchNothing);
#endif
    givePatternFeedback();

    m_powerMode = index;
    m_librePad->writePowerMode(m_powerMode);
}

void LpSearchBar::enterPowerMode()
{
    QString initialPattern;
#if 0
    bool selectionOnly = false;
#endif
    m_librePad->writePowerSearch(true);

    // Guess settings from context: init pattern with current selection
    if (m_librePad->editor() != nullptr)
    {
        QTextCursor cursor = m_librePad->editor()->textCursor();
        const bool selected = cursor.hasSelection();
        if (selected) {
            initialPattern = m_librePad->editor()->textCursor().selectedText();
        }
    }

    if (initialPattern.isNull()) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        const bool selected = clipboard->text() != "" && !clipboard->text().contains("\n");
        if (selected) {
            initialPattern = clipboard->text();
        }
    }

    // If there's no new selection, we'll use the existing pattern
    if (initialPattern.isNull()) {
        // Coming from power search?
        const bool fromReplace = (m_powerUi != nullptr) && (m_widget->isVisible());
        if (fromReplace) {
            QLineEdit *const patternLineEdit = m_powerUi->pattern->lineEdit();
            Q_ASSERT(patternLineEdit != nullptr);
            patternLineEdit->selectAll();
            m_powerUi->pattern->setFocus(Qt::MouseFocusReason);
            return;
        }

        // Coming from incremental search?
        const bool fromIncremental = (m_incUi != nullptr) && (m_widget->isVisible());
        if (fromIncremental) {
            initialPattern = m_incUi->pattern->currentText();
        } else {
            // Search bar probably newly opened. Reset initial replacement text to empty
            m_replacement.clear();
        }
    }

    // Create dialog
    const bool create = (m_powerUi == nullptr);
    if (create) {
        // Kill incremental widget
        if (m_incUi != nullptr) {
            // Backup current settings
            const bool OF_INCREMENTAL = false;
            backupConfig(OF_INCREMENTAL);

            // Kill widget
            delete m_incUi;
            m_incUi = nullptr;
            m_layout->removeWidget(m_widget);
            m_widget->deleteLater(); // I didn't get a crash here but for symmetrie to the other mutate slot^
        }

        // Add power widget
        m_widget = new QWidget(this);
        m_powerUi = new Ui::PowerSearchBar;
        m_powerUi->setupUi(m_widget);
        m_layout->addWidget(m_widget);
        m_layout->setAlignment(m_widget, Qt::AlignTop);

        // Bind to historys
        m_powerUi->pattern->setDuplicatesEnabled(false);
        m_powerUi->pattern->setInsertPolicy(QComboBox::InsertAtTop);

        QStringList phist = m_librePad->powerFind();
        if(phist.size())
        {
            qDebug() << "[LpSearchBar::enterIncrementalMode] power search history size:" << phist.size();
            qDebug() << "[LpSearchBar::enterIncrementalMode] power search history:" << phist;
            m_powerUi->pattern->setMaxCount(phist.size());
            m_powerUi->pattern->addItems(phist);
        }

        m_powerUi->pattern->lineEdit()->setClearButtonEnabled(true);
        m_powerUi->pattern->setCompleter(nullptr);

        m_powerUi->replacement->setDuplicatesEnabled(false);
        m_powerUi->replacement->setInsertPolicy(QComboBox::InsertAtTop);

        QStringList rhist = m_librePad->powerFind();
        if(rhist.size())
        {
            qDebug() << "[LpSearchBar::enterIncrementalMode] replace history size:" << rhist.size();
            qDebug() << "[LpSearchBar::enterIncrementalMode] replace history:" << rhist;
            m_powerUi->replacement->setMaxCount(rhist.size());
            m_powerUi->replacement->addItems(rhist);
        }

        m_powerUi->replacement->lineEdit()->setClearButtonEnabled(true);
        m_powerUi->replacement->setCompleter(nullptr);

        // Filter Up/Down arrow key inputs to save unfinished search/replace text
        m_powerUi->pattern->installEventFilter(this);
        m_powerUi->replacement->installEventFilter(this);

        // Icons
        // Gnome does not seem to have all icons we want, so we use fall-back icons for those that are missing.
        QIcon mutateIcon = QIcon::fromTheme(QStringLiteral("games-config-options"), QIcon::fromTheme(QStringLiteral("preferences-system")));
        QIcon matchCaseIcon = QIcon::fromTheme(QStringLiteral("format-text-superscript"), QIcon::fromTheme(QStringLiteral("format-text-bold")));
        m_powerUi->mutate->setIcon(mutateIcon);
        m_powerUi->mutate->setChecked(true);
        m_powerUi->findNext->setIcon(QIcon::fromTheme(QStringLiteral("go-down-search")));
        m_powerUi->findPrev->setIcon(QIcon::fromTheme(QStringLiteral("go-up-search")));
        m_powerUi->findAll->setIcon(QIcon::fromTheme(QStringLiteral("edit-find")));
        m_powerUi->matchCase->setIcon(matchCaseIcon);
#if 0
        m_powerUi->selectionOnly->setIcon(QIcon::fromTheme(QStringLiteral("edit-select-all")));
#else
        m_powerUi->selectionOnly->hide();
#endif
        // Focus proxy
        centralWidget()->setFocusProxy(m_powerUi->pattern);
    }
#if 0
    m_powerUi->selectionOnly->setChecked(selectionOnly);
#endif
    // Restore previous settings
    if (create) {
        m_powerUi->matchCase->setChecked(m_powerMatchCase);
        m_powerUi->searchMode->setCurrentIndex(m_powerMode);
    }

    // force current index of -1 --> <cursor down> shows 1st completion entry instead of 2nd
    m_powerUi->pattern->setCurrentIndex(-1);
    m_powerUi->replacement->setCurrentIndex(-1);

    // Set initial search pattern
    QLineEdit *const patternLineEdit = m_powerUi->pattern->lineEdit();
    Q_ASSERT(patternLineEdit != nullptr);
    patternLineEdit->setText(initialPattern);
    patternLineEdit->selectAll();
    patternLineEdit->setFocus();

    // Set initial replacement text
    QLineEdit *const replacementLineEdit = m_powerUi->replacement->lineEdit();
    Q_ASSERT(replacementLineEdit != nullptr);
    replacementLineEdit->setText(m_replacement);

    // Propagate settings (slots are still inactive on purpose)
    onPowerPatternChanged(initialPattern);
    givePatternFeedback();

    if (create) {
        // Slots
        //connect(m_powerUi->mutate, &QToolButton::clicked, this, &LpSearchBar::enterIncrementalMode);
        connect(m_powerUi->mutate, &QToolButton::clicked, m_librePad, &Librepad::find);
        connect(m_powerUi->findNext, &QToolButton::clicked, this, &LpSearchBar::findNext);
        connect(m_powerUi->findPrev, &QToolButton::clicked, this, &LpSearchBar::findPrevious);
        connect(m_powerUi->pattern, &QComboBox::currentTextChanged, this, [=]() { find(true, true);});

        // Make [return] in pattern line edit trigger <find next> action
        connect(patternLineEdit, &QLineEdit::returnPressed, this, &LpSearchBar::onReturnPressed);

        connect(m_powerUi->replaceNext, &QPushButton::clicked, this, &LpSearchBar::replaceNext);
        connect(m_powerUi->replaceAll, &QPushButton::clicked, this, &LpSearchBar::replaceAll);
        connect(m_powerUi->searchMode, &QComboBox::currentIndexChanged, this, &LpSearchBar::onPowerModeChanged);
        connect(m_powerUi->matchCase, &QToolButton::toggled, this, &LpSearchBar::onMatchCaseToggled);
        connect(m_powerUi->findAll, &QPushButton::clicked, this, &LpSearchBar::findAll);
        connect(patternLineEdit, &QLineEdit::textChanged, this, &LpSearchBar::onPowerPatternChanged);
#if 0
        connect(m_powerUi->cancel, &QPushButton::clicked, this, &KateSearchBar::onPowerCancelFindOrReplace);
#endif
        // Hook into line edit context menus
        m_powerUi->pattern->setContextMenuPolicy(Qt::CustomContextMenu);

        connect(m_powerUi->pattern, &QComboBox::customContextMenuRequested, this, qOverload<const QPoint &>(&LpSearchBar::onPowerPatternContextMenuRequest));
        m_powerUi->replacement->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_powerUi->replacement,
                &QComboBox::customContextMenuRequested,
                this,
                qOverload<const QPoint &>(&LpSearchBar::onPowerReplacmentContextMenuRequest));
    }

    // move close button to right layout, ensures properly at top for both incremental + advanced mode
    m_powerUi->gridLayout->addWidget(closeButton(), 0, 2, 1, 1);

    // Focus
    if (m_widget->isVisible()) {
        m_powerUi->pattern->setFocus(Qt::MouseFocusReason);
    }
}

void LpSearchBar::enterIncrementalMode()
{
    QString initialPattern;
    m_librePad->writePowerSearch(false);

    // Guess settings from context: init pattern with current selection
    if (m_librePad->editor() != nullptr)
    {
        QTextCursor cursor = m_librePad->editor()->textCursor();
        const bool selected = cursor.hasSelection();
        if (selected) {
            initialPattern = m_librePad->editor()->textCursor().selectedText();
        }
    }

    if (initialPattern.isNull()) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        const bool selected = clipboard->text()  != "" && !clipboard->text().contains("\n");
        if (selected) {
            initialPattern = clipboard->text();
        }
    }

    // If there's no new selection, we'll use the existing pattern
    if (initialPattern.isNull()) {
        // Coming from incremental search?
        const bool fromIncremental = (m_incUi != nullptr) && (m_widget->isVisible());
        if (fromIncremental) {
            m_incUi->pattern->lineEdit()->selectAll();
            m_incUi->pattern->setFocus(Qt::MouseFocusReason);
            return;
        }

        // Coming from power search?
        const bool fromReplace = (m_powerUi != nullptr) && (m_widget->isVisible());
        if (fromReplace) {
            initialPattern = m_powerUi->pattern->currentText();
            // current text will be used as initial replacement text later
            m_replacement = m_powerUi->replacement->currentText();
        }
    }

    // Still no search pattern? Use the word under the cursor
    if (initialPattern.isNull() && m_librePad->editor() != nullptr)
    {
        m_librePad->editor()->textCursor().select(QTextCursor::WordUnderCursor);
    }

    // Create dialog
    const bool create = (m_incUi == nullptr);
    if (create) {
        // Kill power widget
        if (m_powerUi != nullptr) {
            // Backup current settings
            const bool OF_POWER = true;
            backupConfig(OF_POWER);

            // Kill widget
            delete m_powerUi;
            m_powerUi = nullptr;
            m_layout->removeWidget(m_widget);
            m_widget->deleteLater(); // deleteLater, because it's not a good idea too delete the widget and there for the button triggering this slot
        }

        // Add incremental widget
        m_widget = new QWidget(this);
        m_incUi = new Ui::IncrementalSearchBar;
        m_incUi->setupUi(m_widget);
        m_layout->addWidget(m_widget);
        m_layout->setAlignment(m_widget, Qt::AlignTop);

        // Filter Up/Down arrow key inputs to save unfinished search text
        m_incUi->pattern->installEventFilter(this);

        //         new QShortcut(KStandardShortcut::paste().primary(), m_incUi->pattern, SLOT(paste()), 0, Qt::WidgetWithChildrenShortcut);
        //         if (!KStandardShortcut::paste().alternate().isEmpty())
        //             new QShortcut(KStandardShortcut::paste().alternate(), m_incUi->pattern, SLOT(paste()), 0, Qt::WidgetWithChildrenShortcut);

        // Icons
        // Gnome does not seem to have all icons we want, so we use fall-back icons for those that are missing.
        QIcon mutateIcon = QIcon::fromTheme(QStringLiteral("games-config-options"), QIcon::fromTheme(QStringLiteral("preferences-system")));
        QIcon matchCaseIcon = QIcon::fromTheme(QStringLiteral("format-text-superscript"), QIcon::fromTheme(QStringLiteral("format-text-bold")));
        m_incUi->mutate->setIcon(mutateIcon);
        m_incUi->next->setIcon(QIcon::fromTheme(QStringLiteral("go-down-search")));
        m_incUi->prev->setIcon(QIcon::fromTheme(QStringLiteral("go-up-search")));
        m_incUi->matchCase->setIcon(matchCaseIcon);

        // Ensure minimum size
        m_incUi->pattern->setMinimumWidth(12 * m_incUi->pattern->fontMetrics().height());

        // Customize status area
        m_incUi->status->setAlignment(Qt::AlignLeft);

        // Focus proxy
        centralWidget()->setFocusProxy(m_incUi->pattern);

        m_incUi->pattern->setDuplicatesEnabled(false);
        m_incUi->pattern->setInsertPolicy(QComboBox::InsertAtTop);

        QStringList hist = m_librePad->incFind();
        if(hist.size())
        {
            qDebug() << "[LpSearchBar::enterIncrementalMode] history size:" << hist.size();
            qDebug() << "[LpSearchBar::enterIncrementalMode] history:" << hist;
            m_incUi->pattern->setMaxCount(hist.size());
            m_incUi->pattern->addItems(hist);
        }

        m_incUi->pattern->lineEdit()->setClearButtonEnabled(true);
        m_incUi->pattern->setCompleter(nullptr);
    }

    // Restore previous settings
    if (create)
    {
        m_incUi->matchCase->setChecked(m_incMatchCase);
    }

    // force current index of -1 --> <cursor down> shows 1st completion entry instead of 2nd
    m_incUi->pattern->setCurrentIndex(-1);
#if 0

    // Set initial search pattern
    if (!create) {
        disconnect(m_incUi->pattern, &QComboBox::editTextChanged, this, &LpSearchBar::onIncPatternChanged);
    }
#endif
    m_incUi->pattern->setEditText(initialPattern);
#if 0
    connect(m_incUi->pattern, &QComboBox::editTextChanged, this, &LpSearchBar::onIncPatternChanged);
#endif
    m_incUi->pattern->lineEdit()->selectAll();
 #if 0

    // Propagate settings (slots are still inactive on purpose)
    if (initialPattern.isEmpty()) {
        // Reset edit color
        indicateMatch(MatchNothing);
    }
#endif
    // Enable/disable next/prev
    m_incUi->next->setDisabled(initialPattern.isEmpty());
    m_incUi->prev->setDisabled(initialPattern.isEmpty());

    if (create) {
        // Slots
        //connect(m_incUi->mutate, &QToolButton::clicked, this, &LpSearchBar::enterPowerMode);
        connect(m_incUi->mutate, &QToolButton::clicked, m_librePad, &Librepad::replace);
        connect(m_incUi->next, &QToolButton::clicked, this, &LpSearchBar::findNext);
        connect(m_incUi->prev, &QToolButton::clicked, this, &LpSearchBar::findPrevious);
        connect(m_incUi->pattern->lineEdit(), &QLineEdit::returnPressed, this, &LpSearchBar::onReturnPressed);
        connect(m_incUi->pattern, &QComboBox::currentTextChanged, this, [=]() { find(true, true);});
        connect(m_incUi->matchCase, &QToolButton::toggled, this, &LpSearchBar::onMatchCaseToggled);
    }

    // move close button to right layout, ensures properly at top for both incremental + advanced mode
    m_incUi->hboxLayout->addWidget(closeButton());

    // Focus
    if (m_widget->isVisible()) {
        m_incUi->pattern->setFocus(Qt::MouseFocusReason);
    }
}

bool LpSearchBar::eventFilter(QObject *obj, QEvent *event)
{
    QComboBox *combo = qobject_cast<QComboBox *>(obj);
    if (combo && event->type() == QEvent::KeyPress) {
        const int key = static_cast<QKeyEvent *>(event)->key();
        const int currentIndex = combo->currentIndex();
        const QString currentText = combo->currentText();
        QString &unfinishedText = (m_powerUi && combo == m_powerUi->replacement) ? m_replacement : m_unfinishedSearchText;
        if (key == Qt::Key_Up && currentIndex <= 0 && unfinishedText != currentText) {
            // Only restore unfinished text if we are already in the latest entry
            combo->setCurrentIndex(-1);
            combo->setCurrentText(unfinishedText);
        } else if (key == Qt::Key_Down || key == Qt::Key_Up) {
            // Only save unfinished text if it is not empty and it is modified
            const bool isUnfinishedSearch = (!currentText.trimmed().isEmpty() && (currentIndex == -1 || combo->itemText(currentIndex) != currentText));
            if (isUnfinishedSearch && unfinishedText != currentText) {
                unfinishedText = currentText;
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

QString LpSearchBar::replacementPattern() const
{
    Q_ASSERT(isPower());

    return m_powerUi->replacement->currentText();
}

void LpSearchBar::onPowerPatternContextMenuRequest(const QPoint &pos)
{
    const bool FOR_PATTERN = true;
    showExtendedContextMenu(FOR_PATTERN, pos);
}

void LpSearchBar::onPowerPatternContextMenuRequest()
{
    onPowerPatternContextMenuRequest(m_powerUi->pattern->mapFromGlobal(QCursor::pos()));
}

void LpSearchBar::onPowerReplacmentContextMenuRequest(const QPoint &pos)
{
    const bool FOR_REPLACEMENT = false;
    showExtendedContextMenu(FOR_REPLACEMENT, pos);
}

void LpSearchBar::onPowerReplacmentContextMenuRequest()
{
    onPowerReplacmentContextMenuRequest(m_powerUi->replacement->mapFromGlobal(QCursor::pos()));
}

bool LpSearchBar::isPower() const
{
    return m_powerUi != nullptr;
}

QString LpSearchBar::searchPattern() const
{
    return (m_powerUi != nullptr) ? m_powerUi->pattern->currentText() : m_incUi->pattern->currentText();
}

void LpSearchBar::setSelectionOnly(bool selectionOnly)
{
    if (this->selectionOnly() == selectionOnly) {
        return;
    }

    if (isPower()) {
        m_powerUi->selectionOnly->setChecked(selectionOnly);
    }
}

bool LpSearchBar::selectionOnly() const
{
    return isPower() ? m_powerUi->selectionOnly->isChecked() : false;
}

struct ParInfo {
    int openIndex;
    bool capturing;
    int captureNumber; // 1..9
};

QList<QString> LpSearchBar::getCapturePatterns(const QString &pattern) const
{
    QList<QString> capturePatterns;
    capturePatterns.reserve(9);
    QStack<ParInfo> parInfos;

    const int inputLen = pattern.length();
    int input = 0; // walker index
    bool insideClass = false;
    int captureCount = 0;

    while (input < inputLen) {
        if (insideClass) {
            // Wait for closing, unescaped ']'
            if (pattern[input].unicode() == L']') {
                insideClass = false;
            }
            input++;
        } else {
            switch (pattern[input].unicode()) {
            case L'\\':
                // Skip this and any next character
                input += 2;
                break;

            case L'(':
                ParInfo curInfo;
                curInfo.openIndex = input;
                curInfo.capturing = (input + 1 >= inputLen) || (pattern[input + 1].unicode() != '?');
                if (curInfo.capturing) {
                    captureCount++;
                }
                curInfo.captureNumber = captureCount;
                parInfos.push(curInfo);

                input++;
                break;

            case L')':
                if (!parInfos.empty()) {
                    ParInfo &top = parInfos.top();
                    if (top.capturing && (top.captureNumber <= 9)) {
                        const int start = top.openIndex + 1;
                        const int len = input - start;
                        if (capturePatterns.size() < top.captureNumber) {
                            capturePatterns.resize(top.captureNumber);
                        }
                        capturePatterns[top.captureNumber - 1] = pattern.mid(start, len);
                    }
                    parInfos.pop();
                }

                input++;
                break;

            case L'[':
                input++;
                insideClass = true;
                break;

            default:
                input++;
                break;
            }
        }
    }

    return capturePatterns;
}

#endif // DEVELOPER
