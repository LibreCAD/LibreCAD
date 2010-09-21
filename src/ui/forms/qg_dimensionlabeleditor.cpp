#include "qg_dimensionlabeleditor.h"

#include <qvariant.h>
#include <iostream>
#include "qg_dimensionlabeleditor.ui.h"
/*
 *  Constructs a QG_DimensionLabelEditor as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QG_DimensionLabelEditor::QG_DimensionLabelEditor(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QWidget(parent, name, fl)
{
    setupUi(this);

}

/*
 *  Destroys the object and frees any allocated resources
 */
QG_DimensionLabelEditor::~QG_DimensionLabelEditor()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QG_DimensionLabelEditor::languageChange()
{
    retranslateUi(this);
}

