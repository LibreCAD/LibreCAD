#ifndef LC_DLGSPLINEPOINTS_H
#define LC_DLGSPLINEPOINTS_H

#include<memory>
#include <QDialog>
#include "lc_dialog.h"

class LC_SplinePoints;

namespace Ui {
class DlgSplinePoints;
}

class LC_DlgSplinePoints : public LC_Dialog{
	Q_OBJECT
public:
	LC_DlgSplinePoints(QWidget* parent = nullptr);
	~LC_DlgSplinePoints() override;

public slots:
	virtual void setSpline(LC_SplinePoints& b);
	virtual void updateSpline();
	void updatePoints();

protected slots:
    virtual void languageChange();

private:
	LC_DlgSplinePoints(LC_DlgSplinePoints const&) = delete;
	LC_DlgSplinePoints& operator = (LC_DlgSplinePoints const&) = delete;
	LC_DlgSplinePoints(LC_DlgSplinePoints &&) = delete;
	LC_DlgSplinePoints& operator = (LC_DlgSplinePoints &&) = delete;

	LC_SplinePoints* bezier;
	std::unique_ptr<Ui::DlgSplinePoints> ui;
};

#endif // LC_DLGSPLINEPOINTS_H
