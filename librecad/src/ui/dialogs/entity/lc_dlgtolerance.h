#ifndef LC_DLGTOLERANCE_H
#define LC_DLGTOLERANCE_H

#include "lc_entitypropertiesdlg.h"
#include "lc_tolerance.h"

class QComboBox;

namespace Ui {
    class LC_DlgTolerance;
}

class LC_DlgTolerance : public LC_EntityPropertiesDlg{
    Q_OBJECT
public:
    explicit LC_DlgTolerance(QWidget *parent, LC_GraphicViewport *pViewport, LC_Tolerance* hatch, bool isNew);
    ~LC_DlgTolerance() override;
public slots:
    QString generateDataString();
    void updateEntity() override;
    void accept() override;
protected slots:
    void languageChange();
    void parseAndSetFields(const QString& string);
    void setEntity(LC_Tolerance* e);
private:
    Ui::LC_DlgTolerance *ui;
    void initModifiersComboBox(QComboBox* comboBox) const;
    void initGeometricCharacterCombobox(QComboBox* comboBox);
    LC_Tolerance* m_entity;
    bool m_isNew = false;
    bool m_showExtendedModifiers = false;
};

#endif // LC_DLGTOLERANCE_H
