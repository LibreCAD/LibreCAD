#include "qg_librarywidget.h"

#include <qvariant.h>
#include <iostream>
#include "rs_system.h"
#include "rs_painterqt.h"
#include "rs_staticgraphicview.h"
#include "rs_graphic.h"
#include "rs_actionlibraryinsert.h"
#include "qg_librarywidget.ui.h"
/*
 *  Constructs a QG_LibraryWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LibraryWidget::QG_LibraryWidget(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LibraryWidget::~QG_LibraryWidget()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LibraryWidget::languageChange()
{
    retranslateUi(this);
}

