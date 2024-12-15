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
#include "lpmessage.h"
#include "ui_librepad.h"

#include "lc_dlgabout.h"
#include "textfileviewer.h"


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
    m_searchWidget = new LpSearchBar(this, this);
    /* must be hidden bevor binding to statusBar */
    m_searchWidget->hide();

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
    connect(ui->actionSelectAll, &QAction::triggered, this, &Librepad::selectAll);
    connect(ui->actionDeselect, &QAction::triggered, this, &Librepad::deselect);
    connect(ui->actionCopy, &QAction::triggered, this, &Librepad::copy);
    connect(ui->actionPaste, &QAction::triggered, this, &Librepad::paste);
    connect(ui->actionReload, &QAction::triggered, this, &Librepad::reload);
    connect(ui->actionReplace, &QAction::triggered, this, &Librepad::replace);
    connect(ui->actionFind, &QAction::triggered, this, &Librepad::find);
    connect(ui->actionPrint, &QAction::triggered, this, &Librepad::print);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionUndo, &QAction::triggered, this, &Librepad::undo);
    connect(ui->actionRedo, &QAction::triggered, this, &Librepad::redo);
    connect(ui->actionFont, &QAction::triggered, this, &Librepad::setFont);
    connect(ui->actionAbout, &QAction::triggered, this, &Librepad::about);
    connect(ui->actionAboutIde, &QAction::triggered, this, &Librepad::aboutIde);
    connect(ui->actionLicense, &QAction::triggered, this, &Librepad::licence);
    connect(ui->actionHelp, &QAction::triggered, this, &Librepad::help);
    connect(ui->actionRun, &QAction::triggered, this, &Librepad::run);
    connect(ui->actionLoadScript, &QAction::triggered, this, &Librepad::loadScript);

    connect(ui->actionToolBarMain, &QAction::triggered, this, &Librepad::toolBarMain);
    connect(ui->actionToolBarBuild, &QAction::triggered, this, &Librepad::toolBarBuild);
    connect(ui->actionToolBarSearch, &QAction::triggered, this, &Librepad::toolBarSearch);
    connect(ui->actionCmdDock, &QAction::triggered, this, &Librepad::cmdDock);

    ui->actionAboutIde->setToolTip("about " + editorName());

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
    if (editor() != nullptr)
    {
        return editor()->toPlainText();
    }
    return "";
}

void Librepad::reload()
{
    if (editor() != nullptr)
    {
        editor()->reload();
    }
}

void Librepad::redo()
{
    if (editor() != nullptr)
    {
        connect(editor(), &QPlainTextEdit::redoAvailable, ui->actionRedo, &QAction::setEnabled);
        editor()->redo();
    }
}

void Librepad::undo()
{
    if (editor() != nullptr)
    {
        connect(editor(), &QPlainTextEdit::undoAvailable, ui->actionUndo, &QAction::setEnabled);
        editor()->undo();
    }
}

void Librepad::copy()
{
    if (editor() != nullptr)
    {
        editor()->copy();
    }
}

void Librepad::paste()
{
    if (editor() != nullptr)
    {
        editor()->paste();
    }
}

void Librepad::slotTabClose(int index)
{
    if (editor() != nullptr)
    {
        if (editor()->document()->isModified())
        {
            QMessageBox::StandardButton btn = QMessageBox::question(this,
                tr("Save document"),
                tr("The changes were not saved. Do you still want to close it?"));

            if (btn != QMessageBox::Yes)
            {
                ui->tabWidget->setCurrentWidget(editor());
                editor()->saveAs();
            }
        }
        ui->tabWidget->removeTab(index);
        setWindowTitle(editorName());
        delete editor();
    }
}

void Librepad::addNewTab(const QString& path)
{
    TextEditor *editor = new TextEditor(this, path);
    editor->setFont(m_font);

    //QIcon icon = QIcon::fromTheme(QStringLiteral("document-save")));
    //QIcon icon = QIcon::fromTheme(QStringLiteral("text-x-generic")));

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
    if (editor() != nullptr)
    {
        editor()->save();
    }
}

void Librepad::saveAs()
{
    if (editor() != nullptr)
    {
        editor()->saveAs();

        connect(editor(), &TextEditor::documentChanged, this, [=]() {
            m_fileName = editor()->path();
            writeRecentSettings(editor()->path());
            setWindowTitle(editorName() + " - [ " + editor()->fileName() + " ]");
        });
    }
}

void Librepad::selectAll()
{
    if (editor() != nullptr)
    {
        editor()->selectAll();
    }
}

void Librepad::deselect()
{
    if (editor() != nullptr)
    {
        QTextCursor cursor = editor()->textCursor();
        cursor.movePosition(QTextCursor::End);
        editor()->setTextCursor(cursor);
    }
}

void Librepad::print()
{
    if (editor() != nullptr)
    {
        editor()->printer();
    }
}

void Librepad::setFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, QFont("Monospace", 10), this);

    if (ok) {
        if (editor() != nullptr)
        {
            m_font = font;
            writeFontSettings();
            editor()->setFont(font);
        }
    }
}

bool Librepad::firstSave() const
{
    if (editor() != nullptr)
    {
        return editor()->firstSave();
    }
    return false;
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

    settings.beginGroup("Search");
    settings.setValue(editorNametolower() + "/searchbar", m_searchWidget->isHidden());
    settings.endGroup();

    settings.beginGroup("Toolbars");
    settings.setValue(editorNametolower() + "/mainToolBar", ui->mainToolBar->isHidden());
    settings.setValue(editorNametolower() + "/scriptToolBar", ui->buildToolBar->isHidden());
    settings.setValue(editorNametolower() + "/searchToolBar", ui->searchToolBar->isHidden());
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

void Librepad::writePowerMatchCase(bool enabled)
{
    QSettings settings("Librepad", "Librepad");

    settings.beginGroup("Search");
    settings.setValue(editorNametolower() + "/powermatchcase", enabled);
    settings.endGroup();
}

void Librepad::writeIncMatchCase(bool enabled)
{
    QSettings settings("Librepad", "Librepad");

    settings.beginGroup("Search");
    settings.setValue(editorNametolower() + "/incmatchcase", enabled);
    settings.endGroup();
}

void Librepad::writePowerSearch(bool enabled)
{
    QSettings settings("Librepad", "Librepad");

    settings.beginGroup("Search");
    settings.setValue(editorNametolower() + "/powersearch", enabled);
    settings.endGroup();
}

void Librepad::writePowerMode(int index)
{
    QSettings settings("Librepad", "Librepad");

    settings.beginGroup("Search");
    settings.setValue(editorNametolower() + "/powermatchmode", index);
    settings.endGroup();
}

bool Librepad::powerMatchCase()
{
    QSettings settings("Librepad", "Librepad");
    settings.beginGroup("Search");

    const auto matchcase = settings.value(editorNametolower() + "/powermatchcase").toBool();

    if (settings.contains(editorNametolower() + "/powermatchcase"))
    {
        settings.endGroup();
        return matchcase;
    }

    settings.endGroup();
    return false;
}

bool Librepad::incMatchCase()
{
    QSettings settings("Librepad", "Librepad");
    settings.beginGroup("Search");

    const auto matchcase = settings.value(editorNametolower() + "/incmatchcase").toBool();

    if (settings.contains(editorNametolower() + "/incmatchcase"))
    {
        settings.endGroup();
        return matchcase;
    }

    settings.endGroup();
    return false;
}

int Librepad::powerMatchMode()
{
    QSettings settings("Librepad", "Librepad");
    settings.beginGroup("Search");

    const auto matchmode = settings.value(editorNametolower() + "/powermatchmode").toInt();

    if (settings.contains(editorNametolower() + "/powermatchmode"))
    {
        settings.endGroup();
        return matchmode;
    }

    settings.endGroup();
    return 0;
}

bool Librepad::powerSearch()
{
    QSettings settings("Librepad", "Librepad");
    settings.beginGroup("Search");

    const auto matchcase = settings.value(editorNametolower() + "/powersearch").toBool();

    if (settings.contains(editorNametolower() + "/powersearch"))
    {
        settings.endGroup();
        return matchcase;
    }

    settings.endGroup();
    return false;
}

void Librepad::writeSearchHistory(const QString &path, const QStringList &list)
{
    QSettings settings("Librepad", "Librepad");
    settings.beginGroup("Search");

    settings.beginWriteArray(editorNametolower() + path);

    for (int i = 0; i < list.size(); i++) {
        settings.setArrayIndex(i);
        settings.setValue("item", list.at(i));
    }
    settings.endArray();
    settings.endGroup();
}

QStringList Librepad::searchHistory(const QString &path)
{
    QStringList list;
    QSettings settings("Librepad", "Librepad");
    settings.beginGroup("Search");

    int size = settings.beginReadArray(editorNametolower() + path);

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        list.append(settings.value("item").toString());
    }
    settings.endArray();
    settings.endGroup();

    return list;
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

    settings.beginGroup("Search");
    const auto isHidden = settings.value(editorNametolower() + "/searchbar").toBool();
    if (settings.contains(editorNametolower() + "/searchbar"))
    {
        if(!isHidden)
        {
            qDebug() << "!isHidden";
            if(powerSearch())
            {
                qDebug() << "powerSearch()";
                replace();
            }
            else
            {
                qDebug() << "!powerSearch()";
                find();
            }
        }
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

void Librepad::hideSearch()
{
    m_searchWidget->setFixedHeight(0);
    ui->statusBar->removeWidget(m_searchWidget);
}

void Librepad::replace()
{
    m_searchWidget->setFixedHeight(113);
    m_searchWidget->enterPowerMode();
    ui->statusBar->addPermanentWidget(m_searchWidget, 1);
    m_searchWidget->show();
}

void Librepad::find()
{
    m_searchWidget->setFixedHeight(50);
    m_searchWidget->enterIncrementalMode();
    ui->statusBar->addPermanentWidget(m_searchWidget, 1);
    m_searchWidget->show();
}

void Librepad::about()
{
    LC_DlgAbout dlg(this);
    dlg.exec();
}

void Librepad::aboutIde()
{
    QMessageBox::about(this,
        tr("About ") + editorName(),
        QString("<b>" + editorName() + "</b>") +
        tr("<br>LibreCAD embedded IDE</br><br>by Emanuel GPLv2 (c) 2024</br>")
    );
}

void Librepad::licence()
{
    QDialog dlg;
    dlg.setWindowTitle(QObject::tr("License"));

    auto viewer = new TextFileViewer(&dlg);
    auto layout = new QVBoxLayout;
    layout->addWidget(viewer);
    dlg.setLayout(layout);

    viewer->addFile("readme", ":/readme.md");
    viewer->addFile("GPLv2", ":/gpl-2.0.txt");
    viewer->setFile("readme");

    dlg.exec();
}

void Librepad::help()
{
    QMessageBox::about(this,
                       editorName(),
                       QString("<b>Help</b>") + tr("<br>not implemented yet!</br><br>X-(</br>")
                       );
}

void Librepad::message(const QString &msg)
{
    LpMessage::info(msg, this);
}

#endif // DEVELOPER
