#include "qg_dlgarc.h"

#include <qvariant.h>
#include "rs_arc.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "qg_widgetpen.h"
#include "qg_layerbox.h"
#include "qg_dlgarc.ui.h"
/*
 *  Constructs a QG_DlgArc as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
QG_DlgArc::QG_DlgArc(QWidget* parent, const char* name, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, name, modal, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DlgArc::~QG_DlgArc()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DlgArc::languageChange()
{
    retranslateUi(this);
}

