// File: console_command_utils.cpp

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

#include "console_command_utils.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace LC_Console {
namespace {

QString normalizedExt(QString ext) {
    ext = ext.trimmed().toLower();
    if (ext.startsWith('.'))
        ext.remove(0, 1);
    return ext;
}

bool hasAcceptedExtension(const QString& fileName, const QStringList& extensions) {
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    for (const QString& ext : extensions) {
        if (suffix == normalizedExt(ext))
            return true;
    }
    return false;
}

} // namespace

QString CommandContext::displayCommand() const {
    if (usesSubcommand && !commandName.isEmpty())
        return executablePath + " " + commandName;
    return executablePath;
}

NormalizedArgv::NormalizedArgv(int argc, char** argv, const CommandContext& context) {
    m_storage.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        if (context.usesSubcommand && i == 1)
            continue;
        m_storage.append(QByteArray(argv[i]));
    }

    m_argv.reserve(m_storage.size());
    for (QByteArray& arg : m_storage)
        m_argv.append(arg.data());
}

int NormalizedArgv::argc() const {
    return m_argv.size();
}

char** NormalizedArgv::argv() {
    return m_argv.data();
}

QStringList converterCommandNames() {
    return {
        QStringLiteral("dxf2dwg"),
        QStringLiteral("dwg2dxf"),
        QStringLiteral("dxf2pdf"),
        QStringLiteral("dwg2pdf"),
        QStringLiteral("dxf2png"),
        QStringLiteral("dwg2png"),
        QStringLiteral("dxf2svg"),
        QStringLiteral("dwg2svg")
    };
}

CommandContext detectCommand(int argc, char** argv, const QStringList& commands) {
    CommandContext context;
    if (argc <= 0 || argv == nullptr)
        return context;

    context.executablePath = QFile::decodeName(argv[0]);
    const QString baseName = QFileInfo(context.executablePath).baseName();
    for (const QString& command : commands) {
        if (baseName == command) {
            context.commandName = command;
            context.usesSubcommand = false;
            return context;
        }
    }

    if (argc > 1) {
        const QString firstArg = QFile::decodeName(argv[1]);
        for (const QString& command : commands) {
            if (firstArg == command) {
                context.commandName = command;
                context.usesSubcommand = true;
                return context;
            }
        }
    }

    return context;
}

CommandContext contextForCommand(int argc, char** argv, const QString& commandName) {
    CommandContext context = detectCommand(argc, argv, {commandName});
    if (context.commandName.isEmpty()) {
        context.commandName = commandName;
        if (argc > 0 && argv != nullptr)
            context.executablePath = QFile::decodeName(argv[0]);
    }
    return context;
}

QStringList acceptedExtensions(const QString& primaryExt,
                               const QStringList& compatibilityExts) {
    QStringList extensions;
    const QString primary = normalizedExt(primaryExt);
    if (!primary.isEmpty())
        extensions.append(primary);
    for (const QString& ext : compatibilityExts) {
        const QString normalized = normalizedExt(ext);
        if (!normalized.isEmpty() && !extensions.contains(normalized))
            extensions.append(normalized);
    }
    return extensions;
}

QString extensionDescription(const QStringList& extensions) {
    QStringList parts;
    for (const QString& ext : extensions) {
        const QString normalized = normalizedExt(ext);
        if (!normalized.isEmpty())
            parts.append("." + normalized);
    }
    return parts.join("/");
}

QStringList collectInputFiles(const QStringList& positionalArgs,
                              const QStringList& extensions) {
    QStringList files;
    for (const QString& arg : positionalArgs) {
        if (hasAcceptedExtension(arg, extensions))
            files.append(arg);
    }
    return files;
}

bool containsDwgInput(const QStringList& files) {
    for (const QString& file : files) {
        if (QFileInfo(file).suffix().compare(QStringLiteral("dwg"), Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

bool dwgSupportAvailable() {
#ifdef DWGSUPPORT
    return true;
#else
    return false;
#endif
}

QString defaultOutputPath(const QString& inputFile, const QString& outputExt,
                          const QString& outputDir) {
    const QFileInfo inputInfo(inputFile);
    const QString base = inputInfo.completeBaseName() + "." + normalizedExt(outputExt);
    const QString dir = outputDir.isEmpty() ? inputInfo.absolutePath() : outputDir;
    return QDir(dir).filePath(base);
}

bool ensureOutputDirectory(const QString& outputDir, QString* errorMessage) {
    if (outputDir.isEmpty())
        return true;

    if (QDir().mkpath(outputDir))
        return true;

    if (errorMessage != nullptr)
        *errorMessage = QStringLiteral("cannot create directory '%1'").arg(outputDir);
    return false;
}

bool validateOutputOptions(int inputCount, const QString& outputFile,
                           const QString& outputDir,
                           bool allowOutputWithMultipleInputs,
                           bool allowOutputAndDirectory,
                           QString* errorMessage) {
    if (!outputFile.isEmpty() && !outputDir.isEmpty() &&
        !allowOutputAndDirectory) {
        if (errorMessage != nullptr)
            *errorMessage = QStringLiteral("-o/--output and -t/--directory are mutually exclusive.");
        return false;
    }

    if (inputCount > 1 && !outputFile.isEmpty() &&
        !allowOutputWithMultipleInputs) {
        if (errorMessage != nullptr)
            *errorMessage = QStringLiteral("-o/--output can only be used with a single input file.");
        return false;
    }

    return true;
}

} // namespace LC_Console
