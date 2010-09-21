#include "qg_commandwidget.h"

#include <qvariant.h>
#include "qg_commandedit.h"
#include "qg_commandwidget.ui.h"
/*
 *  Constructs a QG_CommandWidget as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_CommandWidget::QG_CommandWidget(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_CommandWidget::~QG_CommandWidget()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_CommandWidget::languageChange()
{
    retranslateUi(this);
}

