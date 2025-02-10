/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include "lc_ucslist.h"

LC_UCSList::LC_UCSList() {
    ucsList.append(wcs);
    setModified(false);
}

void LC_UCSList::clear() {
    ucsList.clear();
    ucsList.append(wcs);
    setModified(true);
}

QList<LC_UCS *>::iterator LC_UCSList::begin() {
    return ucsList.begin();
}

QList<LC_UCS *>::iterator LC_UCSList::end() {
    return ucsList.end();
}

QList<LC_UCS *>::const_iterator LC_UCSList::begin() const {
    return ucsList.begin();
}

QList<LC_UCS *>::const_iterator LC_UCSList::end() const {
    return ucsList.end();
}


void LC_UCSList::add(LC_UCS *ucs) {
    if (ucs == nullptr) {
        return;
    }

    // check if layer already exists:
    LC_UCS *v = find(ucs->getName());
    if (v == nullptr) {
        ucsList.append(ucs);
    }
}

void LC_UCSList::addNew(LC_UCS *ucs) {
    if (ucs == nullptr) {
        return;
    }

    // check if layer already exists:
    LC_UCS *v = find(ucs->getName());
    if (v == nullptr) {
        ucsList.append(ucs);
        setModified(true);
    }
}

void LC_UCSList::remove(LC_UCS *ucs) {
    if (ucs->isUCS()) {
        ucsList.removeOne(ucs);
        setModified(true);
        delete ucs;
    }
}

void LC_UCSList::remove(const QString &name) {
    LC_UCS *v = find(name);
    if (v !=nullptr) {
        remove(v);
    }
}

void LC_UCSList::rename(LC_UCS *ucs, const QString &newName) {
    if (ucs->isUCS()) {
        ucs->setName(newName);
        ucs->setTemporary(false);
        setModified(true);
    }
}

void LC_UCSList::edited([[maybe_unused]]LC_UCS *ucs) {
    setModified(true);
}

LC_UCS *LC_UCSList::find(const QString &name) {
    for (auto v: ucsList){
        if (v->getName() == name){
            return v;
        }
    }
    return nullptr;
}

int LC_UCSList::getIndex(const QString &name) {
    int result = -1;

    for (int i = 0; i < ucsList.size(); i++) {
        LC_UCS *v = ucsList.at(i);
        if (v->getName() == name) {
            result = i;
            break;
        }
    }
    return result;
}

int LC_UCSList::getIndex(LC_UCS *ucs) {
    return ucsList.indexOf(ucs);
}

void LC_UCSList::setModified(bool m) {
    modified = m;
    fireModified(m);
}

LC_UCS *LC_UCSList::tryAddUCS(LC_UCS *candidate) {
    if (candidate == nullptr) {
        return nullptr;
    }
    LC_UCS *existingUCS = findExisting(candidate);

    LC_UCS* result;
    if (existingUCS == nullptr){
        ucsList.append(candidate);
        setModified(true);
        result = candidate;
    }
    else{
        result = existingUCS;
    }
    return result;
}

LC_UCS *LC_UCSList::findExisting(LC_UCS *candidate) {// check if layer already exists:
    LC_UCS *existingUCS = nullptr;
    for (auto v: ucsList){
        if (v == candidate){
            existingUCS = v;
            break;
        }
    }
    if (existingUCS == nullptr){
        for (auto v: ucsList) {
            if (v->isSameTo(candidate)) {
                existingUCS = v;
                break;
            }
        }
    }
    return existingUCS;
}

LC_UCS *LC_UCSList::getWCS() {
    return wcs;
}

void LC_UCSList::tryToSetActive(LC_UCS *ucs) {
    LC_UCS* oldActive = activeUCS;
    activeUCS = findExisting(ucs);
    if (oldActive != activeUCS) {
        if (oldActive == nullptr) {
            modified = true;
            fireModified(modified);
        }
        else if (!oldActive->isSameTo(activeUCS)){
            modified = true;
            fireModified(modified);
        }
    }
}
