/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 Dongxu Li (dongxuli2011@gmail.com)
** Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include<vector>

#include <QObject>
#include <QRegularExpression>
#include <QTextStream>

#include "lc_commandItems.h"
#include "rs_commands.h"
#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_math.h"
#include "rs_system.h"

namespace {

const char* g_FnPrefix = "Fn";
const char* g_AltPrefix = "Alt-";
const char* g_MetaPrefix = "Meta-";

struct LC_CommandItem {
    std::vector<std::pair<QString, QString>> const fullCmdList;
    std::vector<std::pair<QString, QString>> const shortCmdList;
    RS2::ActionType actionType;
};

// helper function to check and report command collision
template<typename T1, typename T2>
bool isCollisionFree(std::map<T1, T2> const& lookUp, T1 const& key, T2 const& value)
{
    if(!lookUp.count(key)) return true;

    //report command string collision
    QString msg=__FILE__+QObject::tr(": duplicated command: %1 is already taken by %2")
                                 .arg(key).arg(value);

    RS_DEBUG->print(RS_Debug::D_ERROR, "%s\n", msg.toStdString().c_str());
    return false;
}

// write alias file
void writeAliasFile(QFile& file,
                    const std::map<QString, RS2::ActionType>& shortCommands,
                    const std::map<QString, RS2::ActionType>& mainCommands
                    )
{
    QFile f{QFileInfo{file}.absoluteFilePath()};
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    QTextStream ts(&f);
    ts << "#LibreCAD alias v1\n\n";
    ts << "# lines starting with # are comments\n";
    ts << "# format are:\n";
    ts << R"(# <alias>\t<command-untranslated>)" "\n";
    ts << "# example\n";
    ts << "# l\tline\n\n";

    // the reverse look up from action type to avoid quadratic time complexity
    std::map<RS2::ActionType, QString> actionToMain;
    for(auto const& [cmd, action]: mainCommands)
        if (actionToMain.count(action) == 0)
            actionToMain.emplace(action, cmd);
    for(auto const& [alias, action]: shortCommands) {
        if (actionToMain.count(action) == 1)
            ts<<alias<<'\t'<<actionToMain.at(action)<<Qt::endl;
    }
}
}

RS_Commands* RS_Commands::instance() {
    static RS_Commands* uniqueInstance = new RS_Commands();
    return uniqueInstance;
}


/**
 * Constructor. Initiates main command dictionary.
 * mainCommand keeps a map from translated commands to actionType
 * shortCommand keeps a list of translated short commands
 * cmdTranslation contains both ways of mapping between translated and English
 * Command order:
 *      mainCommand (long form): Category (long) + Parameter(s)
 *      shortCommand: 2 letter keycode followed by legacy commands
 * Commands form:
 *    list all <main (full) command and translation string> pairs (category+parameters, i.e "line2p")
 *    Category: (long form for mainCommands, also appear is alias file as "command-untranslated")
 *        line - lin / li / l
 *        ...
 *        (others as req'd)
 *    Parameters:
 *        point - pt / p
 *        circle - cir / c
 *        radius - rad / r
 *        tangent - tan / t
 *        angled - ang / a
 *        vertical - ver / v
 *        horizontal - hor / h
 *        (others as req'd)
 *    Two character key-codes:
 *        first two letters for 'base' command or
 *        first letter of catagory followed by parameter (best choice when possible)
 *           draw line - li
 *           ...
 *           etc.
 */

RS_Commands::RS_Commands() {

    for(auto const& c0: g_commandList){
        auto const act=c0.actionType;
        //add full commands
        for(auto const& p0: c0.fullCmdList){
            if(isCollisionFree(cmdTranslation, p0.first, p0.second))
                cmdTranslation[p0.first]=p0.second;
            if(isCollisionFree(mainCommands, p0.second, act))
                mainCommands[p0.second]=act;
        }
        //add short commands
        for(auto const& p1: c0.shortCmdList){
            if(isCollisionFree(cmdTranslation, p1.first, p1.second))
                cmdTranslation[p1.first]=p1.second;
            if(isCollisionFree(shortCommands, p1.second, act))
                shortCommands[p1.second]=act;
        }
    }

    // translations
    for(auto const& [command, translation]: g_transList) {
        cmdTranslation[command] = translation;
    }

    for (const auto& [command, translation]: cmdTranslation) {
        m_revTranslation[translation] = command;
        if (mainCommands.count(translation) == 1)
            mainCommands[command] = mainCommands[translation];
        else if (shortCommands.count(translation) == 1)
            shortCommands[command] = shortCommands[translation];
    }

    for(const auto& [command, action]: mainCommands) {
        m_actionToCommand[action] = command;
    }
}

/**
 * Read existing alias file or create one new.
 * In OS_WIN32 "c:\documents&settings\<user>\local configuration\application data\LibreCAD\librecad.alias"
 * In OS_MAC "/Users/<user>/Library/Application Support/LibreCAD/librecad.alias"
 * In OS_LINUX "/home/<user>/.local/share/data/LibreCAD/librecad.alias"
 */
void RS_Commands::updateAlias()
{
    LC_LOG << __func__ << "(): begin";
    QString aliasName = RS_SYSTEM->getAppDataDir();
    if (aliasName.isEmpty())
        return;
    aliasName += "/librecad.alias";
    if (!QFileInfo::exists(aliasName))
        return;

    QFile f(aliasName);
    LC_LOG<<__func__<<"(): Command alias file: "<<aliasName;
    auto validateCmd = [this](QString cmd) {
        return mainCommands.count(cmd) == 1 || shortCommands.count(cmd) == 1
               || cmdTranslation.count(cmd) == 1;
    };
    std::map<QString, QString> aliasList;
    if (f.exists() && f.open(QIODevice::ReadOnly)) {

        //alias file exists, read user defined alias
        QTextStream ts(&f);
        //check if is empty file or not alias file
        while(!ts.atEnd())
        {
            QString line=ts.readLine().trimmed();
            if (line.isEmpty() || line.at(0)=='#' )
                continue;
            // Read alias
            static QRegularExpression re(R"(\s)");
            QStringList txtList = line.split(re,Qt::SkipEmptyParts);
            if (txtList.size() < 2 || txtList[0].startsWith('#'))
                continue;

            const QString& alias = txtList[0];
            const QString& cmd = txtList[1];
            if (validateCmd(cmd)) {
                const RS2::ActionType action = commandToAction(cmd);
                if (action == RS2::ActionNone) {
                    LC_ERR<<__func__<<"(): requesting alias("<<alias<<") for unknown command: "<<cmd;
                    continue;
                }
                if (m_actionToCommand.count(action) == 0)
                    m_actionToCommand[action] = cmd;
                const RS2::ActionType actionAlias = commandToAction(alias);
                if (actionAlias != action &&  actionAlias != RS2::ActionNone) {
                    LC_ERR<<__func__<<"(): cannot overwrite existing alias: "<<alias<<"="<<m_actionToCommand.at(action)<<": with "<<alias<<"=" << cmd;
                } else {
                    aliasList[alias] = m_actionToCommand[action];
                }
            } else {
                LC_ERR<<__func__<<"(): invalid alias, command not found: "<<line;
            }
        }
        f.close();
    } else {
        //alias file does no exist, create one with translated shortCommands
        writeAliasFile(f, shortCommands, mainCommands);
    }
    //update alias file with non present commands
    //RLZ: to be written

    //add alias to shortCommands
    for(auto const& [alias, cmd]: aliasList){
        if(shortCommands.count(alias) == 1 || mainCommands.count(alias) == 1)
            continue;
        if(mainCommands.count(cmd) == 1){
            RS_DEBUG->print("adding command alias: %s\t%s\n", alias.toStdString().c_str(), cmd.toStdString().c_str());
            shortCommands[alias]=mainCommands[cmd];
        }else if(cmdTranslation.count(cmd) == 1){
            RS_DEBUG->print("adding command alias: %s\t%s\n", alias.toStdString().c_str(), cmdTranslation[cmd].toStdString().c_str());
            shortCommands[alias]=mainCommands[cmdTranslation[cmd]];
        }
    }
    LC_LOG << __func__ << "(): done";
}

RS2::ActionType RS_Commands::commandToAction(const QString& command) const
{
    if (mainCommands.count(command)==1)
        return mainCommands.at(command);
    if (shortCommands.count(command)==1)
        return shortCommands.at(command);
    if (cmdTranslation.count(command) == 1) {
        QString translated = cmdTranslation.at(command);
        if (mainCommands.count(translated) == 1)
            return mainCommands.at(translated);
        if (shortCommands.count(translated) == 1)
            return shortCommands.at(translated);
    }
    return RS2::ActionNone;
}

/**
 * Tries to complete the given command (e.g. when tab is pressed).
 */
QStringList RS_Commands::complete(const QString& cmd) const {
    QStringList ret;
    for(auto const& p: mainCommands){
        if(p.first.startsWith(cmd, Qt::CaseInsensitive)){
            ret << p.first;
        }
    }
    ret.sort();

    return ret;
}

/**
 * @return Command for triggering the given action in the currently chosen
 * language for commands.
 *
 * @param action ID of the action who's command will be returned.
 * @param num Number of the command. There might be multiple commands
 *            for the same action (e.g. 'line' and 'l')
 *
 * @return The translated command.
 */
RS2::ActionType RS_Commands::cmdToAction(const QString& cmd, bool verbose) const {
    QString full = cmd.toLower();
    RS2::ActionType ret = RS2::ActionNone;

    // find command:
    for(const auto& table: {mainCommands, shortCommands})
    {
        if (table.count(cmd)) {
            ret = table.at(cmd);
            break;
        }
    }
    if (ret==RS2::ActionNone)
        return ret;

    if (!verbose) return ret;
    // find full command to confirm to user:
    for(auto const& p: mainCommands){
        if(p.second==ret){
            RS_DEBUG->print("RS_Commands::cmdToAction: commandMessage");
            RS_DIALOGFACTORY->commandMessage(QObject::tr("Command: %1 (%2)").arg(full).arg(p.first));
            //                                        RS_DialogFactory::instance()->commandMessage( QObject::tr("Command: %1").arg(full));
            RS_DEBUG->print("RS_Commands::cmdToAction: "
                            "commandMessage: ok");
            return ret;
        }
    }
    RS_DEBUG->print(QObject::tr("RS_Commands:: command not found: %1").arg(full).toStdString().c_str());
    return ret;
}

/**
 * Gets the action for the given keycode. A keycode is a sequence
 * of key-strokes that is entered like hotkeys.
 */
RS2::ActionType RS_Commands::keycodeToAction(const QString& code) const {
    if(code.size() < 1)
        return RS2::ActionNone;

    if(!(code.startsWith(g_FnPrefix) ||
          code.startsWith(g_AltPrefix) ||
          code.startsWith(g_MetaPrefix))) {
        if(code.size() < 1 || code.contains(QRegularExpression("^[a-zA-Z].*")) == false )
            return RS2::ActionNone;
    }

    auto action = commandToAction(code);

    if (action != RS2::ActionNone) {
        //found
        const QString& cmd = (m_actionToCommand.count(action) == 1) ? m_actionToCommand.at(action) : QString{};
        RS_DIALOGFACTORY->commandMessage(QObject::tr("keycode: %1 (%2)").arg(code).arg(cmd));
    } else {
        RS_DIALOGFACTORY->commandMessage(QObject::tr("invalid keycode: %1").arg(code));
    }

    return action;
}


/**
 * @return translated command for the given English command.
 */
QString RS_Commands::command(const QString& cmd) {
    auto it= instance()->cmdTranslation.find(cmd);
    if(it != instance()->cmdTranslation.end()){
        return instance()->cmdTranslation[cmd];
    }
    RS_DIALOGFACTORY->commandMessage(QObject::tr("Command not found: %1").arg(cmd));
    RS_DEBUG->print(RS_Debug::D_WARNING,
                    "RS_Commands::command: command '%s' unknown", cmd.toLatin1().data());
    return "";
}



/**
 * Checks if the given string 'str' matches the given command 'cmd' for action
 * 'action'.
 *
 * @param cmd The command we want to check for (e.g. 'angle').
 * @param action The action which wants to know.
 * @param str The string typically entered by the user.
 */
bool RS_Commands::checkCommand(const QString& cmd, const QString& str,
                               RS2::ActionType /*action*/) {

    QString const& strl = str.toLower();
    QString const& cmdLower = cmd.toLower();
    auto it = instance()->cmdTranslation.find(cmdLower);
    if(it != instance()->cmdTranslation.end()){
        RS2::ActionType type0=instance()->cmdToAction(it->second, false);
        if( type0  != RS2::ActionNone ) {
            return  type0 ==instance()->cmdToAction(strl);
        }
    }

    it =  instance()->cmdTranslation.find(strl);
    if(it !=  instance()->cmdTranslation.end()) return it->second == cmdLower;
    return false;
}


/**
 * @return the local translation for "Commands available:".
 */
QString RS_Commands::msgAvailableCommands() {
    return QObject::tr("Available commands:");
}
