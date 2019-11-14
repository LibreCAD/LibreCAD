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
#include "lc_helpbrowser.h"
#include "qc_applicationwindow.h"
#include <QDir>
#include <QCoreApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QDockWidget>
#include <QToolButton>
#include <QLibrary>

LC_HelpBrowser* LC_HelpBrowser::uniqueInstance = nullptr;

LC_HelpBrowser::LC_HelpBrowser() : QObject()
{
	populateHelpTopics();
}

LC_HelpBrowser::~LC_HelpBrowser()
{
}

LC_HelpBrowser * LC_HelpBrowser::instance()
{
	if (!uniqueInstance) {
		uniqueInstance = new LC_HelpBrowser();
	}
	return uniqueInstance;
}

void LC_HelpBrowser::showTableOfContents()
{
	showHelpTopic("TableOfContents");
}

void LC_HelpBrowser::showHelpTopic(const QString & topic)
{
	if (topicMap.contains(topic)) {
		QStringList args;
		args << topicMap[topic].toString();
		proc.start(getDefaultBrowser(), args);
	}
}

void LC_HelpBrowser::showHelpTopic(QObject * w)
{
	if (w && registry.contains(w))
		showHelpTopic(registry[w]);
}

void LC_HelpBrowser::associateTopic(QObject * w, const QString& topicName)
{
	if (!w)	return;
	registry[w] = topicName;
}

void LC_HelpBrowser::associateTopic(QAction * a, const QString & topicName)
{
	if (a)
		registeredActions[a] = topicName;
}

bool LC_HelpBrowser::eventFilter(QObject * obj, QEvent * event)
{
	if (event && event->type() == QEvent::KeyPress) {
		QKeyEvent *ke = static_cast<QKeyEvent *>(event);
		if (ke->key() == Qt::Key_F1 || ke->key() == Qt::Key_Help) {
			QString topic = "TableOfContents";
			if (obj && obj->isWidgetType()) {
				QWidget *widget = qobject_cast<QWidget*>(obj);
				QPoint point = widget->mapFromGlobal(QCursor::pos());
				QWidget* w = widget->childAt(point);
				if (w) {
					if (registry.contains(w))
						topic = registry[w];
					else if (w->focusWidget() && registry.contains(w->focusWidget()))
						topic = registry[w->focusWidget()];
					else if (QDockWidget* dw = qobject_cast<QDockWidget*>(w))
					{
						QWidget* docked = dw->widget();
						if (registry.contains(docked))
							topic = registry[docked];
					}
					else 
					{
						QString t = "";
						QWidget* parent = w->parentWidget();
						if (QToolButton* button = qobject_cast<QToolButton*>(w))
						{
							if (registeredActions.contains(button->defaultAction()))
								t = registeredActions[button->defaultAction()];
						}
						while (parent && t.isEmpty()) {
							if (registry.contains(parent))
								t = registry[parent];
							parent = parent->parentWidget();
						}
						if (!t.isEmpty())
							topic = t;
					}
				}
			}
			showHelpTopic(topic);
			return true;
		}
	}
	return false;
}

void LC_HelpBrowser::populateHelpTopics()
{
	QUrl base = QUrl::fromLocalFile(QDir::cleanPath(QCoreApplication::applicationDirPath()) + "/");
	topicMap["TableOfContents"] = base.resolved(QUrl(getRelativeFilePath("Default.htm")));
	topicMap["topic_append_calculator"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Appendices/append_calculator.htm")));
	topicMap["topic_append_commandline"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Appendices/append_commandline.htm")));
	topicMap["topic_append_drawingscales"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Appendices/append_drawingscales.htm")));
	topicMap["topic_append_hatch"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Appendices/append_hatch.htm")));
	topicMap["topic_guide_blocks"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_blocks.htm")));
	topicMap["topic_guide_commandline"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_commandline.htm")));
	topicMap["topic_guide_container"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_container.htm")));
	topicMap["topic_guide_dimensions"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_dimensions.htm")));
	topicMap["topic_guide_drawing_attributes"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_drawing_attributes.htm")));
	topicMap["topic_guide_drawing_properties"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_drawing_properties.htm")));
	topicMap["topic_guide_drawingsetup"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_drawingsetup.htm")));
	topicMap["topic_guide_entities"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_entities.htm")));
	topicMap["topic_guide_layers"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_layers.htm")));
	topicMap["topic_guide_library"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_library.htm")));
	topicMap["topic_guide_plugins"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_plugins.htm")));
	topicMap["topic_guide_printing"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_printing.htm")));
	topicMap["topic_guide_templates"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_templates.htm")));
	topicMap["topic_guide_text"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/User Guides/guide_text.htm")));
	topicMap["topic_intro_pronest"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Getting Started/intro_pronest.htm")));
	topicMap["topic_intro_overview"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Getting Started/intro_overview.htm")));
	topicMap["topic_prefs_drawingpreferences"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Preferences and Customizations/prefs_drawingpreferences.htm")));
	topicMap["topic_snapping"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Snapping/snapping.htm")));
	topicMap["topic_tool_circles"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Tools/tool_circles.htm")));
	topicMap["topic_tool_container"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Tools/tool_container.htm")));
	topicMap["topic_tool_curves"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Tools/tool_curves.htm")));
	topicMap["topic_tool_dimension"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Tools/tool_dimension.htm")));
	topicMap["topic_tool_ellipse"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Tools/tool_ellipse.htm")));
	topicMap["topic_tool_info"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Tools/tool_info.htm")));
	topicMap["topic_tool_lines"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Tools/tool_lines.htm")));
	topicMap["topic_tool_modify"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Tools/tool_modify.htm")));
	topicMap["topic_tool_other"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Tools/tool_other.htm")));
	topicMap["topic_tool_polyline"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Tools/tool_polyline.htm")));
	topicMap["topic_tool_select"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Tools/tool_select.htm")));
	topicMap["topic_tw_blocklistdock"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Toolbars and Widgets/tw_blocklistdock.htm")));
	topicMap["topic_tw_dockwidgets"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Toolbars and Widgets/tw_dockwidgets.htm")));
	topicMap["topic_tw_general"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Toolbars and Widgets/tw_general.htm")));
	topicMap["topic_tw_layerlistdock"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Toolbars and Widgets/tw_layerlistdock.htm")));
	topicMap["topic_tw_librarybrowserdock"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Toolbars and Widgets/tw_librarybrowserdock.htm")));
	topicMap["topic_tw_menucreator"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Toolbars and Widgets/tw_menucreator.htm")));
	topicMap["topic_tw_penwizarddock"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Toolbars and Widgets/tw_penwizarddock.htm")));
	topicMap["topic_tw_printpreview"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Toolbars and Widgets/tw_printpreview.htm")));
	topicMap["topic_tw_toolbarcreator"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Toolbars and Widgets/tw_toolbarcreator.htm")));
	topicMap["topic_Welcome"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Welcome.htm")));
	topicMap["topic_prefs_applicationpreferences"] = base.resolved(QUrl(getRelativeFilePath("Default.htm#Main/Preferences and Customizations/prefs_applicationpreferences.htm")));
}

QString LC_HelpBrowser::getLocaleName()
{
	return QString("en-US");  //TODO: modify this when translations become available
}

QString LC_HelpBrowser::getRelativeFilePath(const QString & fileName)
{
	return QString("doc/%1/%2").arg(getLocaleName(), fileName);
}

QString LC_HelpBrowser::getDefaultBrowser()
{
	QDir dir = QDir::cleanPath(QCoreApplication::applicationDirPath());
	QLibrary library(QFileInfo(dir, "ProNestUtils.dll").filePath());
	typedef wchar_t* (*DefaultBrowserFunc)();
	DefaultBrowserFunc DefaultBrowser = (DefaultBrowserFunc)library.resolve("DefaultBrowserEXEName");
	if (DefaultBrowser) {
		return QString::fromStdWString(DefaultBrowser());
	}
	return QString();
}
