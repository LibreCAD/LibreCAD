/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2019 Shawn Curry (noneyabiz@mail.wasent.cz)

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
#ifndef LC_HELPBROWSER_H
#define LC_HELPBROWSER_H
#include <QObject>
#include <QWidget>
#include <QString>
#include <QMap>
#include <QUrl>
#include <QPoint>
#include <QProcess>

#define LC_HELP LC_HelpBrowser::instance()

class LC_HelpBrowser : public QObject
{
	Q_OBJECT
public:
	static LC_HelpBrowser* instance();

	void showTableOfContents();
	void showHelpTopic(const QString& topic);
	void showHelpTopic(QObject* w);

	void associateTopic(QObject* w, const QString& topicName);
	void associateTopic(QAction* a, const QString& topicName);

protected:
	bool eventFilter(QObject *obj, QEvent *event) override;

private:
	LC_HelpBrowser();
	virtual ~LC_HelpBrowser();
	LC_HelpBrowser(LC_HelpBrowser const&) = delete;
	LC_HelpBrowser& operator = (LC_HelpBrowser const&) = delete;
	void populateHelpTopics();
	QString getLocaleName();
	QString getRelativeFilePath(const QString& fileName);
	QString getDefaultBrowser();

private:
	QMap<QString, QUrl> topicMap;
	QMap<QObject*, QString> registry;
	QMap<QAction*, QString> registeredActions;
	QProcess proc;
	static LC_HelpBrowser* uniqueInstance;
};

#endif // LC_HELPBROWSER_H
