#include "qg_dlgdimension.h"

#include <qvariant.h>
#include <qmessagebox.h>
#include "qg_widgetpen.h"
#include "qg_layerbox.h"
#include "qg_dimensionlabeleditor.h"
#include "qg_dlgdimension.ui.h"
/*
 *  Constructs a QG_DlgDimension as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgDimension::QG_DlgDimension(QWidget* parent, const char* name, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, name, modal, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgDimension::~QG_DlgDimension()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgDimension::languageChange()
{
    retranslateUi(this);
}

