#ifndef LC_DLGSPLINEPOINTS_H
#define LC_DLGSPLINEPOINTS_H

#include "lc_entitypropertiesdlg.h"

class LC_SplinePoints;

namespace Ui {
    class DlgSplinePoints;
}

class LC_DlgSplinePoints : public LC_EntityPropertiesDlg{
	Q_OBJECT
public:
    LC_DlgSplinePoints(QWidget* parent, LC_GraphicViewport* vp, LC_SplinePoints * splinePoints);
    ~LC_DlgSplinePoints() override;
public slots:
    void updateEntity() override;
    void updatePoints();
protected slots:
    void languageChange();
protected:
    void setEntity(LC_SplinePoints* b);
    LC_DlgSplinePoints(LC_DlgSplinePoints const&) = delete;
    LC_DlgSplinePoints& operator = (LC_DlgSplinePoints const&) = delete;
    LC_DlgSplinePoints(LC_DlgSplinePoints &&) = delete;
    LC_DlgSplinePoints& operator = (LC_DlgSplinePoints &&) = delete;
    LC_SplinePoints* m_entity;
    std::unique_ptr<Ui::DlgSplinePoints> ui;
};
#endif // LC_DLGSPLINEPOINTS_H
