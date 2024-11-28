// Copyright (C) 2024 Emanuel Strobel
// GPLv2

#include <QFile>
#include <QSettings>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QtPrintSupport/qtprintsupportglobal.h>
#include <QPrintDialog>
#include <QPrinter>
#include <QFont>
#include <QFontDialog>
#include <QPainter>
#include <QTabBar>
#include <QToolBar>
#include <QAction>

#include "librepad.h"
#include "texteditor.h"
#include "ui_librepad.h"

#ifdef DEVELOPER

Librepad::Librepad(QWidget *parent, const QString& name, const QString& fileName)
    : QMainWindow(parent)
    , m_editorName(name)
    , m_fileName(fileName)
    , m_font(QFont("Monospace",10))
    , ui(new Ui::Librepad)
    , m_maxFileNr(10)
{
    this->hide();
    ui->setupUi(this);
    setCentralWidget(ui->tabWidget);
    ui->tabWidget->tabBar()->setTabsClosable(true);

    m_searchLineEdit = new QLineEdit;
    m_searchLineEdit->setMaximumWidth(180);
    ui->searchToolBar->addWidget(m_searchLineEdit);

    QAction* recentFileAction = 0;
    for(unsigned int i = 0; i < m_maxFileNr; ++i){
        recentFileAction = new QAction(this);
        recentFileAction->setVisible(false);
        QObject::connect(recentFileAction, &QAction::triggered,
                         this, &Librepad::openRecent);
        m_recentFileActionList.append(recentFileAction);
        ui->menuOpen_Recent->addAction(recentFileAction);
    }

    updateRecentActionList();

    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &Librepad::slotTabClose);
    connect(ui->actionNew, &QAction::triggered, this, &Librepad::newDocument);
    connect(ui->actionOpen, &QAction::triggered, this, &Librepad::open);
    connect(ui->actionSave, &QAction::triggered, this, &Librepad::save);
    connect(ui->actionSave_as, &QAction::triggered, this, &Librepad::saveAs);
    connect(ui->actionReload, &QAction::triggered, this, &Librepad::reload);
    connect(ui->actionPrint, &QAction::triggered, this, &Librepad::print);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionUndo, &QAction::triggered, this, &Librepad::undo);
    connect(ui->actionRedo, &QAction::triggered, this, &Librepad::redo);
    connect(ui->actionFont, &QAction::triggered, this, &Librepad::setFont);
    connect(ui->actionAbout, &QAction::triggered, this, &Librepad::about);
    connect(ui->actionHelp, &QAction::triggered, this, &Librepad::help);

    connect(ui->actionCmdDock, &QAction::triggered, this, &Librepad::cmdDock);

    connect(ui->actionToolBarMain, &QAction::triggered, this, &Librepad::toolBarMain);
    connect(ui->actionToolBarSearch, &QAction::triggered, this, &Librepad::toolBarSearch);
    connect(ui->actionToolBarBuild, &QAction::triggered, this, &Librepad::toolBarBuild);

    connect(ui->actionRun, &QAction::triggered, this, &Librepad::run);
    connect(ui->actionLoadScript, &QAction::triggered, this, &Librepad::loadScript);
    connect(ui->actionPrevious, &QAction::triggered, this, [=]() {
        slotSearchChanged(m_searchLineEdit->text(), false, false);
    });
    connect(ui->actionNext, &QAction::triggered, this, [=]() {
        slotSearchChanged(m_searchLineEdit->text(), true, false);
    });
    connect(m_searchLineEdit, &QLineEdit::textChanged, this, [=]() {
        slotSearchChanged(m_searchLineEdit->text(), true, true);});

    connect(ui->actionCopy, &QAction::triggered, this, &Librepad::copy);
    connect(ui->actionPaste, &QAction::triggered, this, &Librepad::paste);

    ui->actionAbout->setToolTip("about " + editorName());

    readSettings();

    if(m_fileName.isEmpty()) {
        addNewTab("*" + editorNametolower());
    }
    else {
        addNewTab(m_fileName);
    }

}

void Librepad::slotTabChanged(int index) {
    TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(index));
    if (editor == nullptr)
    {
        return;
    }

    connect(editor, &QPlainTextEdit::redoAvailable, ui->actionRedo, &QAction::setEnabled);
    connect(editor, &QPlainTextEdit::undoAvailable, ui->actionUndo, &QAction::setEnabled);
    connect(editor, &TextEditor::documentChanged, this, [=]() {
        ui->tabWidget->tabBar()->setTabText(index, editor->fileName());
        ui->tabWidget->tabBar()->setTabToolTip(index, editor->fileName());
    });

    setWindowTitle(editorName() + " - [ " + editor->fileName() + " ]");
    ui->tabWidget->tabBar()->setTabText(index, editor->fileName());
}

void Librepad::closeEvent(QCloseEvent *event)
{
    writeSettings();

    for(int i = 0; i < ui->tabWidget->count(); i++) {
        TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(i));
        if (editor == nullptr)
        {
            return;
        }

        if (editor->document()->isModified())
        {
            QMessageBox::StandardButton btn = QMessageBox::question(this,
                                                                    tr("Save document"),
                                                                    tr("Save changes to the following item?\n%1").arg(editor->fileName()),
                                                                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            if (btn == QMessageBox::Save)
            {
                event->ignore();
                ui->tabWidget->setCurrentWidget(editor);
                editor->saveAs();
            }
            if (btn == QMessageBox::Cancel)
            {
                event->ignore();
            }
            if(btn == QMessageBox::Discard) {
                event->accept();
            }
        }
    }
}

void Librepad::enableIDETools()
{
    ui->buildToolBar->show();
    ui->actionCmdDock->setVisible(true);
    ui->actionCmdDock->setEnabled(true);
}

void Librepad::setCmdWidgetChecked(bool val)
{
    ui->actionCmdDock->setChecked(val);
}

QString Librepad::toPlainText() const
{
    TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(ui->tabWidget->currentIndex()));
    if (editor == nullptr)
    {
        return "";
    }
    return editor->toPlainText();
}

void Librepad::reload()
{
    TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(ui->tabWidget->currentIndex()));
    if (editor == nullptr)
    {
        return;
    }
    editor->reload();
}


void Librepad::redo()
{
    TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(ui->tabWidget->currentIndex()));
    if (editor == nullptr)
    {
        return;
    }
    connect(editor, &QPlainTextEdit::redoAvailable, ui->actionRedo, &QAction::setEnabled);
    editor->redo();
}

void Librepad::undo()
{
    TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(ui->tabWidget->currentIndex()));
    if (editor == nullptr)
    {
        return;
    }
    connect(editor, &QPlainTextEdit::undoAvailable, ui->actionUndo, &QAction::setEnabled);
    editor->undo();
}

void Librepad::copy()
{
    TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(ui->tabWidget->currentIndex()));
    if (editor == nullptr)
    {
        return;
    }
    editor->copy();
}

void Librepad::paste()
{
    TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(ui->tabWidget->currentIndex()));
    if (editor == nullptr)
    {
        return;
    }
    editor->paste();
}

void Librepad::slotSearchChanged(const QString &text, bool direction, bool reset)
{
    QString search_text = text;
    if (search_text.trimmed().isEmpty())
    {
        return;
    }

    TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->currentWidget());
    if (editor == nullptr)
    {
        return;
    }

    QTextDocument *document = editor->document();
    QTextCursor    cur      = editor->textCursor();

    static QList<QTextCursor> highlight_cursors;
    static int                index = 0;
    if (reset)
    {
        /* Traverse and search all */
        cur = editor->textCursor();
        cur.clearSelection();
        cur.movePosition(QTextCursor::Start);

        highlight_cursors.clear();
        QTextCursor highlight_cursor = document->find(search_text);
        while (!highlight_cursor.isNull())
        {
            highlight_cursors.append(highlight_cursor);
            highlight_cursor = document->find(search_text, highlight_cursor);
        }
    }
    else
    {
        if (direction)
        {
            index += 1;
        }
        else
        {
            index -= 1;
        }
        index = qMax(0, index);

        index = index % highlight_cursors.size();
    }

    QList<QTextEdit::ExtraSelection> list; /* = editor->extraSelections();*/

    if (highlight_cursors.size() > 0 && index < highlight_cursors.size())
    {
        QTextCharFormat highlightFormat;
        highlightFormat.setBackground(Qt::yellow);
        highlightFormat.setForeground(Qt::blue);
        QTextEdit::ExtraSelection selection;
        selection.cursor = highlight_cursors[index];
        selection.format = highlightFormat;

        list.append(selection);

        editor->setTextCursor(highlight_cursors[index]);
        editor->setExtraSelections(list);
    }
}

void Librepad::slotTabClose(int index)
{
    TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(index));
    if (editor == nullptr)
    {
        return;
    }

    if (editor->document()->isModified())
    {
        QMessageBox::StandardButton btn = QMessageBox::question(this,
                                                                tr("Save document"),
                                                                tr("The changes were not saved. Do you still want to close it?"));
        if (btn != QMessageBox::Yes)
        {
            ui->tabWidget->setCurrentWidget(editor);
            editor->saveAs();
        }
    }
    ui->tabWidget->removeTab(index);
    setWindowTitle(editorName());
    delete editor;
}

void Librepad::addNewTab(const QString& path)
{
    TextEditor *editor = new TextEditor(this, path);
    editor->setFont(m_font);

    ui->tabWidget->addTab(editor, editor->fileName());
    int index = ui->tabWidget->count() - 1;
    ui->tabWidget->setCurrentIndex(index);
    ui->tabWidget->tabBar()->setTabText(index, editor->fileName());
    ui->tabWidget->tabBar()->setTabToolTip(index, editor->fileName());
    setWindowTitle(editorName() + " - [ " + editor->fileName() + " ]");

    connect(editor, &TextEditor::documentChanged, this, [=]() {
        setWindowTitle(editorName() + " - [ " + editor->fileName() + " ]");
        ui->tabWidget->tabBar()->setTabText(index, editor->fileName());
        ui->tabWidget->tabBar()->setTabToolTip(index, editor->fileName());
    });

    ui->actionRedo->setEnabled(false);
    ui->actionUndo->setEnabled(false);
    connect(editor, &QPlainTextEdit::redoAvailable, ui->actionRedo, &QAction::setEnabled);
    connect(editor, &QPlainTextEdit::undoAvailable, ui->actionUndo, &QAction::setEnabled);

    editor->setFocus();

    if (path.at(0) != '*')
    {
        writeRecentSettings(path);
    }

}

Librepad::~Librepad()
{
    delete ui;
}

void Librepad::newDocument()
{
    addNewTab("*" + editorNametolower());
}

void Librepad::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open the file", "All Files (*.*)");
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, "Warning", "Cannot open file: " + file.errorString());
        return;
    }
    addNewTab(fileName);
}

void Librepad::openRecent()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        addNewTab(action->data().toString());
    }
}

void Librepad::save()
{
    TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(ui->tabWidget->currentIndex()));
    if (editor == nullptr)
    {
        return;
    }
    editor->save();
}

void Librepad::saveAs()
{
    TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(ui->tabWidget->currentIndex()));
    if (editor == nullptr)
    {
        return;
    }
    editor->saveAs();
    qDebug() << __func__ << editor->path();
    m_fileName = editor->path();
    writeRecentSettings(editor->path());
    setWindowTitle(editorName() + " - [ " + editor->fileName() + " ]");
}

void Librepad::print()
{
    TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(ui->tabWidget->currentIndex()));
    if (editor == nullptr)
    {
        return;
    }
    editor->printer();
}

void Librepad::setFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(
        &ok, QFont("Monospace", 10), this);
    if (ok) {
        TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(ui->tabWidget->currentIndex()));
        if (editor == nullptr)
        {
            return;
        }
        m_font = font;
        writeFontSettings();
        editor->setFont(font);
    }
}

bool Librepad::firstSave() const
{
    TextEditor *editor = dynamic_cast<TextEditor *>(ui->tabWidget->widget(ui->tabWidget->currentIndex()));
    if (editor == nullptr)
    {
        return false;
    }

    return editor->firstSave();
}

void Librepad::about()
{
    QMessageBox::about(this,
                       tr("About ") + editorName(),
                       QString("<b>" + editorName() + "</b>") + tr("<br>LibreCAD embedded IDE</br><br>by Emanuel GPLv2 (c) 2024</br>")
                      );
}

void Librepad::help()
{
    QMessageBox::about(this,
                       editorName(),
                       QString("<b>Help</b>") + tr("<br>not implemented yet!</br><br>X-(</br>")
                       );
}

void Librepad::toolBarMain()
{
    if (ui->mainToolBar->isHidden())
    {
        ui->mainToolBar->show();
    }
    else
    {
        ui->mainToolBar->hide();
    }
}

void Librepad::toolBarSearch()
{
    if (ui->searchToolBar->isHidden())
    {
        ui->searchToolBar->show();
    }
    else
    {
        ui->searchToolBar->hide();
    }
}

void Librepad::toolBarBuild()
{
    if (ui->buildToolBar->isHidden())
    {
        ui->buildToolBar->show();
    }
    else
    {
        ui->buildToolBar->hide();
    }
}

void Librepad::writeSettings()
{
    QSettings settings("Librepad", "Librepad");

    settings.beginGroup("MainWindow");
    settings.setValue(editorNametolower() + "/geometry", saveGeometry());
    settings.endGroup();

    settings.beginGroup("Toolbars");
    settings.setValue(editorNametolower() + "/mainToolBar", ui->mainToolBar->isHidden());
    settings.setValue(editorNametolower() + "/searchToolBar", ui->searchToolBar->isHidden());
    settings.setValue(editorNametolower() + "/scriptToolBar", ui->buildToolBar->isHidden());
    settings.endGroup();
}

void Librepad::writeFontSettings()
{
    QSettings settings("Librepad", "Librepad");

    settings.beginGroup("Font");
    settings.setValue(editorNametolower() + "/fontpointsize", m_font.pointSize());
    settings.setValue(editorNametolower() + "/fontfamily", m_font.family());
    settings.setValue(editorNametolower() + "/fontbold", m_font.bold());
    settings.setValue(editorNametolower() + "/fontitalic", m_font.italic());
    settings.endGroup();
}

void Librepad::writeRecentSettings(const QString &filePath)
{
    QSettings settings("Librepad", "Librepad");

    settings.beginGroup("Files");
    QStringList recentFilePaths =
        settings.value(editorNametolower() + "/recentFiles").toStringList();
    recentFilePaths.removeAll(filePath);
    recentFilePaths.prepend(filePath);
    while (recentFilePaths.size() > m_maxFileNr)
        recentFilePaths.removeLast();
    settings.setValue(editorNametolower() + "/recentFiles", recentFilePaths);
    settings.endGroup();

    updateRecentActionList();
}

void Librepad::readSettings()
{
    QSettings settings("Librepad", "Librepad");

    settings.beginGroup("MainWindow");
    const auto geometry = settings.value(editorNametolower() + "/geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty())
    {
        setGeometry(320, 280, 1280, 720);
    }
    else
    {
        restoreGeometry(geometry);
    }
    settings.endGroup();

    settings.beginGroup("Font");
    const auto pointsize = settings.value(editorNametolower() + "/fontpointsize").toInt();
    if (settings.contains(editorNametolower() + "/fontpointsize")) {
        m_font.setPointSize(pointsize);
    }
    else {
        m_font.setPointSize(10);
    }

    if (settings.contains(editorNametolower() + "/fontfamily")) {
        const auto family = settings.value(editorNametolower() + "/fontfamily").toString();
        m_font.setFamily(family);
    }
    else {
        m_font.setFamily("Monospace");
    }

    if (settings.contains(editorNametolower() + "/fontbold")) {
        const bool bold = settings.value(editorNametolower() + "/fontbold").toBool();
        m_font.setBold(bold);
    }
    else {
        m_font.setBold(false);
    }

    if (settings.contains(editorNametolower() + "/fontitalic")) {
        const bool italic = settings.value(editorNametolower() + "/fontitalic").toBool();
        m_font.setItalic(italic);
    }
    else {
        m_font.setItalic(false);
    }
    settings.endGroup();



    settings.beginGroup("Toolbars");

    if (settings.contains(editorNametolower() + "/mainToolBar")) {
        const bool isHidden = settings.value(editorNametolower() + "/mainToolBar").toBool();
        if (!isHidden)
        {
            ui->actionToolBarMain->setChecked(true);
            ui->mainToolBar->show();
        }
        else
        {
            ui->actionToolBarMain->setChecked(false);
            ui->mainToolBar->hide();
        }
    }

    if (settings.contains(editorNametolower() + "/searchToolBar")) {
        const bool isHidden = settings.value(editorNametolower() + "/searchToolBar").toBool();
        if (!isHidden)
        {
            ui->actionToolBarSearch->setChecked(true);
            ui->searchToolBar->show();
        }
        else
        {
            ui->actionToolBarSearch->setChecked(false);
            ui->searchToolBar->hide();
        }
    }

    if (settings.contains(editorNametolower() + "/scriptToolBar")) {
        const bool isHidden = settings.value(editorNametolower() + "/scriptToolBar").toBool();
        if (!isHidden)
        {
            ui->actionToolBarBuild->setChecked(true);
            ui->buildToolBar->show();
        }
        else
        {
            ui->actionToolBarBuild->setChecked(false);
            ui->buildToolBar->hide();
        }
    }

    settings.endGroup();
}

void Librepad::updateRecentActionList()
{
    QSettings settings("Librepad", "Librepad");
    settings.beginGroup("Files");
    QStringList recentFilePaths =
        settings.value(editorNametolower() + "/recentFiles").toStringList();
    settings.endGroup();

    auto itEnd = 0u;
    if(recentFilePaths.size() <= m_maxFileNr)
    {
        itEnd = recentFilePaths.size();
    }
    else
    {
        itEnd = m_maxFileNr;
    }

    for (auto i = 0u; i < itEnd; ++i) {
        QString strippedName = QFileInfo(recentFilePaths.at(i)).fileName();
        m_recentFileActionList.at(i)->setText(strippedName);
        m_recentFileActionList.at(i)->setData(recentFilePaths.at(i));
        m_recentFileActionList.at(i)->setVisible(true);
    }

    for (auto i = itEnd; i < m_maxFileNr; ++i)
        m_recentFileActionList.at(i)->setVisible(false);

}

#endif // DEVELOPER
