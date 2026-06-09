// /****************************************************************************
//
// Utility base class for widgets that represents options for actions
//
// Copyright (C) 2025 LibreCAD.org
// Copyright (C) 2025 sand1024
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
// **********************************************************************
//

#include "lc_xmlwriterqxmlstreamwriter.h"

#include <QXmlStreamWriter>

LC_XMLWriterQXmlStreamWriter::LC_XMLWriterQXmlStreamWriter() : m_xmlWriter(new QXmlStreamWriter(&m_xml)) {
    m_xmlWriter->setAutoFormatting(true);
    //xmlWriter->setEncoding("UTF-8");
}

LC_XMLWriterQXmlStreamWriter::~LC_XMLWriterQXmlStreamWriter() = default;

void LC_XMLWriterQXmlStreamWriter::createRootElement(const std::string& name, const std::string& namespace_uri) {
    m_xmlWriter->writeStartDocument();
    m_xmlWriter->writeDefaultNamespace(QString::fromStdString(namespace_uri));
    m_xmlWriter->writeStartElement(QString::fromStdString(namespace_uri), QString::fromStdString(name));
}

void LC_XMLWriterQXmlStreamWriter::addElement(const std::string& name, const std::string& namespace_uri) {
    m_xmlWriter->writeStartElement(QString::fromStdString(namespace_uri), QString::fromStdString(name));
}

void LC_XMLWriterQXmlStreamWriter::addAttribute(const std::string& name, const std::string& value, const std::string& namespace_uri) {
    m_xmlWriter->writeAttribute(QString::fromStdString(namespace_uri), QString::fromStdString(name), QString::fromStdString(value));
}

void LC_XMLWriterQXmlStreamWriter::addNamespaceDeclaration(const std::string& prefix, const std::string& namespace_uri) {
    m_xmlWriter->writeNamespace(QString::fromStdString(namespace_uri), QString::fromStdString(prefix));
}

void LC_XMLWriterQXmlStreamWriter::closeElement() {
    m_xmlWriter->writeEndElement();
}

std::string LC_XMLWriterQXmlStreamWriter::documentAsString() {
    m_xmlWriter->writeEndDocument();

    return m_xml.toStdString();
}
