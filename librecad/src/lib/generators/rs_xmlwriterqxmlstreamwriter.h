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

#ifndef RS_XMLWRITERQXMLSTREAMWRITER_H
#define RS_XMLWRITERQXMLSTREAMWRITER_H

#include <rs_xmlwriterinterface.h>

#include <QXmlStreamWriter>

class RS_XMLWriterQXmlStreamWriter : public RS_XMLWriterInterface {
public:
    RS_XMLWriterQXmlStreamWriter();

    ~RS_XMLWriterQXmlStreamWriter();

    void createRootElement(const std::string &name, const std::string &namespace_uri = "");

    void addElement(const std::string &name, const std::string &namespace_uri = "");

    void addAttribute(const std::string &name, const std::string &value, const std::string &namespace_uri = "");

    void addNamespaceDeclaration(const std::string &prefix, const std::string &namespace_uri);

    void closeElement();

    std::string documentAsString();

private:

    QXmlStreamWriter* xmlWriter;

    QString xml;
};

#endif
