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

#ifndef RS_XMLWRITERINTERFACE_H
#define RS_XMLWRITERINTERFACE_H

#include <string>

class LC_XMLWriterInterface {
public:
    virtual void createRootElement(const std::string &name, const std::string &default_namespace_uri = "") = 0;

    virtual void addElement(const std::string &name, const std::string &namespace_uri = "") = 0;

    virtual void addAttribute(const std::string &name, const std::string &value, const std::string &namespace_uri = "") = 0;

    virtual void addNamespaceDeclaration(const std::string &prefix, const std::string &namespace_uri) = 0;

    virtual void closeElement() = 0;

    virtual std::string documentAsString() = 0;

	LC_XMLWriterInterface() = default;
	virtual ~LC_XMLWriterInterface() = default;
};

#endif
