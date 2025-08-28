/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
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

#ifndef MAIN_H
#define MAIN_H

#include <QMainWindow>
#include<QStringList>
#include <QPushButton>
#include <QLabel>

#define STR(x)   #x
#define XSTR(x)  STR(x)

class QMainWindow;
/**
 * @brief handleArgs
 * @param argc cli argument counter from main()
 * @param argv cli arguments from main()
 * @param argClean a list of indices to be ignored
 * @return
 */
QStringList handleArgs(int argc, char** argv, const QList<int>& argClean);

/**
 * @brief LCReleaseLabel return a label for the current release based on LC_VERSION in src.pro
 * @return "Release Candidate" - if LC_VERSION contains rc;
 *         "BETA" - if LC_VERSION contains beta
 *         "ALPHA" - if LC_VERSION contains alpha
 */
QString LCReleaseLabel();

#include <QApplication>
#include <QMainWindow>
#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QFrame>

// Overlay widget to dim the background
class OverlayWidget : public QWidget
{
public:
    explicit OverlayWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, false);
        setStyleSheet("background-color: rgba(0, 0, 0, 100);");
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QWidget::paintEvent(event);
    }
};

// Main window class
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
protected:
    void resizeEvent(QResizeEvent* event) override;
private slots:
    void showModalDialog();
    void onTargetClicked();
    void showAboutDialog();

private:
    void enableOnlyTargetWidget();
    void restoreUI();

    bool eventFilter(QObject* obj, QEvent* event) override;

    QPushButton* targetWidget;
    QPushButton* otherWidget1;
    QLabel* otherWidget2;
    OverlayWidget* overlay;
};

#endif
