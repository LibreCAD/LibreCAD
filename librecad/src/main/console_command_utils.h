// File: console_command_utils.h

/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 * ********************************************************************************
 */

#ifndef CONSOLE_COMMAND_UTILS_H
#define CONSOLE_COMMAND_UTILS_H

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

namespace LC_Console {

struct CommandContext {
    QString commandName;
    QString executablePath;
    bool usesSubcommand = false;

    QString displayCommand() const;
};

class NormalizedArgv {
public:
    NormalizedArgv(int argc, char** argv, const CommandContext& context);

    int argc() const;
    char** argv();

private:
    QVector<QByteArray> m_storage;
    QVector<char*> m_argv;
};

QStringList converterCommandNames();
CommandContext detectCommand(int argc, char** argv, const QStringList& commands);
CommandContext contextForCommand(int argc, char** argv, const QString& commandName);

QStringList acceptedExtensions(const QString& primaryExt,
                               const QStringList& compatibilityExts = {});
QString extensionDescription(const QStringList& extensions);
QStringList collectInputFiles(const QStringList& positionalArgs,
                              const QStringList& extensions);
bool containsDwgInput(const QStringList& files);
bool dwgSupportAvailable();

QString defaultOutputPath(const QString& inputFile, const QString& outputExt,
                          const QString& outputDir = {});
bool ensureOutputDirectory(const QString& outputDir, QString* errorMessage = nullptr);
bool validateOutputOptions(int inputCount, const QString& outputFile,
                           const QString& outputDir,
                           bool allowOutputWithMultipleInputs,
                           bool allowOutputAndDirectory,
                           QString* errorMessage = nullptr);

} // namespace LC_Console

#endif // CONSOLE_COMMAND_UTILS_H
