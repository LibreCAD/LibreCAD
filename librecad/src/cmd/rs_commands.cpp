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

#include <QRegularExpression>
#include <QTextStream>

#include "lc_commandItems.h"
#include "rs_commands.h"

#include <QFileInfo>

#include "rs_debug.h"
#include "rs_dialogfactory.h"
#include "rs_dialogfactoryinterface.h"
#include "rs_settings.h"

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
bool isCollisionFree(std::map<T1, T2> const& lookUp, T1 const& key, T2 const& value, QString cmd = {})
{
    if (key == cmd)
        return false;

    if(lookUp.count(key) == 0 || lookUp.at(key) == value)
        return true;

    //report command string collision
    QString msg = __FILE__ + QObject::tr(": duplicated command: %1 is already taken by %2");
    if constexpr (std::is_same_v<T2, RS2::ActionType>)
        msg = msg.arg(key).arg(cmd);
    else
        msg = msg.arg(key).arg(value);

    RS_DEBUG->print(RS_Debug::D_ERROR, "%s\n", msg.toStdString().c_str());
    return false;
}

// write alias file
void writeAliasFile(const QString& aliasName,
                    const std::map<QString, RS2::ActionType>& m_shortCommands,
                    const std::map<QString, RS2::ActionType>& m_mainCommands
                    )
{
    LC_LOG<<__func__<<"(): begin";
    LC_LOG<<"Creating "<<QFileInfo(aliasName).absoluteFilePath();

    QFile aliasFile{aliasName};
    if (!aliasFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        LC_ERR<<__func__<<"(): line "<<__LINE__<<": failed to create "<<QFileInfo(aliasName).absoluteFilePath();
        return;
    }
    QTextStream ts(&aliasFile);
    ts << "#LibreCAD alias v1\n\n";
    ts << "# lines starting with # are comments\n";
    ts << "# format are:\n";
    ts << R"(# <alias>\t<command-untranslated>)" "\n";
    ts << "# the alias cannot be an existing command";
    ts << "# example\n";
    ts << "# l\tline\n\n";

    // the reverse look up from action type to avoid quadratic time complexity
    std::map<RS2::ActionType, QString> actionToMain;

    // full commands should be used first
    for(const auto& item: g_commandList) {
        for(const auto& [fullCmd, translation]: item.fullCmdList)
            actionToMain.emplace(item.actionType, fullCmd);
    }

    for(auto const& [cmd, action]: m_mainCommands)
        if (actionToMain.count(action) == 0)
            actionToMain.emplace(action, cmd);
    for(auto const& [alias, action]: m_shortCommands) {
        if (actionToMain.count(action) == 1)
            ts<<alias<<'\t'<<actionToMain.at(action)<<Qt::endl;
    }
    LC_LOG<<__func__<<"(): end";
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
 * m_cmdTranslation contains both ways of mapping between translated and English
 * Command order:
 *      mainCommand (long form): Category (long) + Parameter(s)
 *      shortCommand: 2 letter keycode followed by legacy commands
 * Commands form:
 *    list all <main (full) command and translation string> pairs (category+parameters, i.e "line2p")
 *    Category: (long form for m_mainCommands, also appear is alias file as "command-untranslated")
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

    for(auto const& [fullCmdList, aliasList, action]: g_commandList){
        //add full commands
        for(auto const& [fullCmd, cmdTranslation]: fullCmdList){
            if (fullCmd == cmdTranslation)
                continue;
            // use translated commands first
            if (isCollisionFree(m_cmdTranslation, fullCmd, cmdTranslation))
                m_cmdTranslation.emplace(fullCmd, cmdTranslation);
            if (isCollisionFree(m_mainCommands, cmdTranslation, action, m_actionToCommand.count(action) ? m_actionToCommand[action] : QString{})) {
                m_mainCommands.emplace(cmdTranslation, action);
                m_actionToCommand.emplace(action, cmdTranslation);
            }
        }
        for(auto const& [fullCmd, cmdTranslation]: fullCmdList){
            if(isCollisionFree(m_mainCommands, fullCmd, action, m_actionToCommand.count(action) ? m_actionToCommand[action] : QString{})) {
                // enable english commands, if no conflict is found
                m_mainCommands.emplace(fullCmd, action);
                m_actionToCommand.emplace(action, fullCmd);
            }
        }
        //add short commands
        for(auto const& [alias, aliasTranslation]: aliasList){
            if (alias == aliasTranslation)
                continue;
            // use translated alias first
            if(isCollisionFree(m_cmdTranslation, alias, aliasTranslation))
                m_cmdTranslation.emplace(alias, aliasTranslation);
            if(isCollisionFree(m_shortCommands, aliasTranslation, action, m_actionToCommand.count(action) ? m_actionToCommand[action] : QString{})) {
                m_shortCommands.emplace(aliasTranslation, action);
                if (m_actionToCommand.count(action) == 0)
                    m_actionToCommand.emplace(action, aliasTranslation);
            }
        }
        for(auto const& [alias, aliasTranslation]: aliasList){
            if(isCollisionFree(m_shortCommands, alias, action, m_actionToCommand.count(action) ? m_actionToCommand[action] : QString{})) {
                // enable english short commands, if no conflict is found
                m_shortCommands.emplace(alias, action);
                if (m_actionToCommand.count(action) == 0)
                    m_actionToCommand.emplace(action, aliasTranslation);
            }
        }
    }

    // translations, overriding existing translation
    for(auto const& [command, translation]: g_transList) {
        m_cmdTranslation[command] = translation;
    }

    // prefer to use translated commands and aliases
    for (const auto& [command, translation]: m_cmdTranslation) {
        m_revTranslation[translation] = command;
        if (m_shortCommands.count(translation) == 1)
            m_shortCommands[command] = m_shortCommands[translation];
    }

    // ensure action to command mapping is consistent
    for(const auto& [command, action]: m_mainCommands) {
        m_actionToCommand[action] = command;
    }
}

QString RS_Commands::getAliasFile()
{
    QString settingsDir = LC_GET_ONE_STR("Paths","OtherSettingsDir", RS_System::instance()->getAppDataDir()).trimmed();
    if (settingsDir.isEmpty()) {
        LC_ERR << __func__ << "(): line "<<__LINE__<<": empty alias folder name: aborting";
        return {};
    }
    QString aliasName = settingsDir + "/librecad.alias";
    return aliasName;
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

    QString aliasName = getAliasFile();
    if (aliasName.isEmpty()) {
        LC_ERR << __func__ << "(): line "<<__LINE__<<": empty alias folder name: aborting";
        return;
    }

    std::map<QString, QString> aliasList = readAliasFile(aliasName);
    if (aliasList.empty()) {
        //alias file does no exist, create one with translated m_shortCommands
        LC_ERR<<"Writing alias file";
        writeAliasFile(aliasName, m_shortCommands, m_mainCommands);
    }

    //update alias file with non present commands

    //add alias to m_shortCommands
    for(auto const& [alias, cmd]: aliasList){
        // Do not override commands, but reusing aliases is allowed
        if(m_mainCommands.count(alias) == 1) {
            LC_ERR<<__func__<<"(): "<<QObject::tr("cannot change meaning of commands. Refused to reuse command %1 to mean %2").arg(alias, cmd);
            continue;
        }

        if(m_mainCommands.count(cmd) == 1){
            RS_DEBUG->print("adding command alias: %s\t%s\n", alias.toStdString().c_str(), cmd.toStdString().c_str());
            m_shortCommands[alias]=m_mainCommands[cmd];
        }else if(m_cmdTranslation.count(cmd) == 1){
            RS_DEBUG->print("adding command alias: %s\t%s\n", alias.toStdString().c_str(), m_cmdTranslation[cmd].toStdString().c_str());
            m_shortCommands[alias]=m_mainCommands[m_cmdTranslation[cmd]];
        }
    }
    LC_LOG << __func__ << "(): done";
}

std::map<QString, QString> RS_Commands::readAliasFile(const QString& aliasName)
{
    LC_ERR<<__func__<<"(): Command alias file: "<<aliasName;
    std::map<QString, QString> aliasList;

    QFile aliasFile{aliasName};
    if (!aliasFile.exists() || !aliasFile.open(QIODevice::ReadOnly))
        return aliasList;

    //alias file exists, read user defined alias
    QTextStream ts(&aliasFile);
    //check if is empty file or not alias file
    while(!ts.atEnd())
    {
        // Read alias
        static QRegularExpression re(R"(\s)");
        QStringList txtList=ts.readLine().trimmed().split(re, Qt::SkipEmptyParts);
        if (txtList.size() < 2 || txtList.front().startsWith('#') || txtList[0] == txtList[1])
            continue;

        const QString& alias = txtList[0];
        const QString& cmd = txtList[1];
        const RS2::ActionType action = commandToAction(cmd);
        if (action == RS2::ActionNone) {
            LC_ERR<<__func__<<"(): "<<QObject::tr("requesting alias(%1) for unknown command(%2): ignored").arg(alias, cmd);
            continue;
        }

        // just in case
        if (m_actionToCommand.count(action) == 0)
            m_actionToCommand[action] = cmd;
        // Logging aliases changed by the alias file
        const RS2::ActionType actionAlias = commandToAction(alias);
        if (actionAlias != action &&  actionAlias != RS2::ActionNone) {
            LC_ERR<<__func__<<"(): "<<QObject::tr("reusing an existing alias: was %1=%2, changed to %1=%3").arg(alias, m_actionToCommand.at(actionAlias), m_actionToCommand[action]);
        }
        if (alias != m_actionToCommand[action]) {
            aliasList.emplace(alias, cmd);
        } else {
            // Do not override commands, but reusing aliases is allowed
            LC_ERR<<__func__<<"(): "<<QObject::tr("cannot change meaning of commands. Refused to reuse command %1 to mean %2").arg(alias, cmd);
        }
    }
    return aliasList;
}

RS2::ActionType RS_Commands::commandToAction(const QString& command) const
{
    if (m_mainCommands.count(command)==1)
        return m_mainCommands.at(command);
    if (m_shortCommands.count(command)==1)
        return m_shortCommands.at(command);
    if (m_cmdTranslation.count(command) == 1) {
        QString translated = m_cmdTranslation.at(command);
        if (m_mainCommands.count(translated) == 1)
            return m_mainCommands.at(translated);
        if (m_shortCommands.count(translated) == 1)
            return m_shortCommands.at(translated);
    }
    return RS2::ActionNone;
}

/**
 * Tries to complete the given command (e.g. when tab is pressed).
 */
QStringList RS_Commands::complete(const QString& cmd) const {
    QStringList ret;
    for(auto const& p: m_mainCommands){
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
    for(const auto& table: {m_mainCommands, m_shortCommands})
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
    for(auto const& p: m_mainCommands){
        if(p.second==ret){
            RS_DEBUG->print("RS_Commands::cmdToAction: commandMessage");
            // fixme - sand - indicate current command somewhere in UI, not in the history window!!!
//            RS_DIALOGFACTORY->commandMessage(QObject::tr("Command: %1 (%2)").arg(full).arg(p.first));
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
        // fixme - sand - make better command context indication - #2084
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
    auto it= instance()->m_cmdTranslation.find(cmd);
    if(it != instance()->m_cmdTranslation.end()){
        return instance()->m_cmdTranslation[cmd];
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
    auto it = instance()->m_cmdTranslation.find(cmdLower);
    if(it != instance()->m_cmdTranslation.end()){
        RS2::ActionType type0=instance()->cmdToAction(it->second, false);
        if( type0  != RS2::ActionNone ) {
            return  type0 ==instance()->cmdToAction(strl);
        }
    }

    it =  instance()->m_cmdTranslation.find(strl);
    if(it !=  instance()->m_cmdTranslation.end()) return it->second == cmdLower;
    return false;
}


/**
 * @return the local translation for "Commands available:".
 */
QString RS_Commands::msgAvailableCommands() {
    return QObject::tr("Available commands:");
}
