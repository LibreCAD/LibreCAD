/****************************************************************************
**
 * QApplication derived class to catch QFileOpenEvent on Mac OS.
 * This implements opening files on double click in Finder,
 * whether the app is running or not.
 * When the event arrives on start up, the file name is cached here,
 * later an eventFilter in QC_ApplicationWindow handles the event directly.

Copyright (C) 2018 A. Stebich (librecad@mail.lordofbikes.de)
Copyright (C) 2018 Simon Wells <simonrwells@gmail.com>

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
**********************************************************************/

#include <QStringList>
#include <QFileOpenEvent>

#include "lc_application.h"

LC_Application::LC_Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
}

// This is only used until the event filter is in place in mainwindow
bool LC_Application::event(QEvent *event)
{
#ifdef Q_OS_MAC
    if (QEvent::FileOpen == event->type()) {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        files.append( openEvent->file());
    }
#endif

    return QApplication::event(event);
}

QStringList const& LC_Application::fileList(void) const
{
    return files;
}
