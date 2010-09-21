#include "qg_dlghatch.h"

#include <qvariant.h>
#include "qg_patternbox.h"
#include "qg_graphicview.h"
#include "qg_dlghatch.ui.h"
/*
 *  Constructs a QG_DlgHatch as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgHatch::QG_DlgHatch(QWidget* parent, const char* name, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, name, modal, fl)
{
    setupUi(this);

    init();
}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgHatch::~QG_DlgHatch()
{
    destroy();
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgHatch::languageChange()
{
    retranslateUi(this);
}

