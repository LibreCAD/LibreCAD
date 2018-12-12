/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 R. van Twisk (librecad@rvt.dds.nl)
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

#include "rs_locale.h"

RS_Locale::RS_Locale() {
}
RS_Locale::RS_Locale(const QString &canonical):
QLocale(toCanonical(canonical)){
}

void RS_Locale::setCanonical(const QString &_canonical) {
    canonical=_canonical;
}

QString RS_Locale::toCanonical(const QString &canonical){
    QString languageCode("C");
    int i1=canonical.indexOf('_');
    if(i1 >= 2 ) {
        languageCode= canonical.mid(0,i1).toLower();
        i1++;
        if(canonical.size() == i1+2 ){
            languageCode += QString('_')+canonical.mid(i1,canonical.size()-i1).toUpper();
        }
    }else{
        languageCode=canonical.toLower();
    }
    return languageCode;
}

void RS_Locale::setDirection(RS2::TextLocaleDirection _direction) {
    direction=_direction;
}
void RS_Locale::setName(const QString &_name) {
    localeName=_name;
}

QString RS_Locale::getCanonical() {
    return canonical;
}
QString RS_Locale::getName() {
    return localeName;
}

QString RS_Locale::name() const{
        return 	languageToString(language())+QString(" (")+countryToString(country())+QString(")");
}

//EOF

