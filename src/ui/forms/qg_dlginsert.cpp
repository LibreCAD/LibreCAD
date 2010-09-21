#include "qg_dlginsert.h"

#include <qvariant.h>
#include "rs_insert.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "qg_widgetpen.h"
#include "qg_layerbox.h"
#include "qg_dlginsert.ui.h"
/*
 *  Constructs a QG_DlgInsert as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgInsert::QG_DlgInsert(QWidget* parent, const char* name, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, name, modal, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgInsert::~QG_DlgInsert()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgInsert::languageChange()
{
    retranslateUi(this);
}

