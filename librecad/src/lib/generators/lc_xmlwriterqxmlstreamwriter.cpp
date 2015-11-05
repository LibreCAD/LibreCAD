/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 Christian Luginbühl (dinkel@pimprecords.com)
**
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along
** with this program; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**
**********************************************************************/

#include <QXmlStreamWriter>
#include "lc_xmlwriterqxmlstreamwriter.h"

#include "lc_xmlwriterinterface.h"

LC_XMLWriterQXmlStreamWriter::LC_XMLWriterQXmlStreamWriter():
	xmlWriter(new QXmlStreamWriter(&xml))
{
	xmlWriter->setAutoFormatting(true);
	xmlWriter->setCodec("UTF-8");
}

LC_XMLWriterQXmlStreamWriter::~LC_XMLWriterQXmlStreamWriter() = default;

void LC_XMLWriterQXmlStreamWriter::createRootElement(const std::string &name, const std::string &namespace_uri) {
    xmlWriter->writeStartDocument();
    xmlWriter->writeDefaultNamespace(QString::fromStdString(namespace_uri));
    xmlWriter->writeStartElement(QString::fromStdString(namespace_uri), QString::fromStdString(name));
}

void LC_XMLWriterQXmlStreamWriter::addElement(const std::string &name, const std::string &namespace_uri) {
    xmlWriter->writeStartElement(QString::fromStdString(namespace_uri), QString::fromStdString(name));
}

void LC_XMLWriterQXmlStreamWriter::addAttribute(const std::string &name, const std::string &value, const std::string &namespace_uri) {
    xmlWriter->writeAttribute(QString::fromStdString(namespace_uri), QString::fromStdString(name), QString::fromStdString(value));
}

void LC_XMLWriterQXmlStreamWriter::addNamespaceDeclaration(const std::string &prefix, const std::string &namespace_uri) {
    xmlWriter->writeNamespace(QString::fromStdString(namespace_uri), QString::fromStdString(prefix));
}

void LC_XMLWriterQXmlStreamWriter::closeElement() {
    xmlWriter->writeEndElement();
}

std::string LC_XMLWriterQXmlStreamWriter::documentAsString() {
    xmlWriter->writeEndDocument();

    return xml.toStdString();
}
