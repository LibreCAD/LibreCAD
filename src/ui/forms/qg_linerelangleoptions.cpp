#include "qg_linerelangleoptions.h"

#include <qvariant.h>
#include "qg_linerelangleoptions.ui.h"
/*
 *  Constructs a QG_LineRelAngleOptions as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_LineRelAngleOptions::QG_LineRelAngleOptions(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_LineRelAngleOptions::~QG_LineRelAngleOptions()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_LineRelAngleOptions::languageChange()
{
    retranslateUi(this);
}

