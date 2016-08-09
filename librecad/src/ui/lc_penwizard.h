#ifndef LC_PENWIZARD_H
#define LC_PENWIZARD_H

#include <QDockWidget>

class ColorWizard;
class QC_MDIWindow;

class LC_PenWizard : public QDockWidget
{
    Q_OBJECT

public:
    LC_PenWizard(const QString& title, QWidget* parent = 0);
    QC_MDIWindow* mdi_win;

protected:
    ColorWizard* color_wiz;

protected slots:
    void setColorForSelected(QColor color);
    void selectByColor(QColor color);
    void setActivePenColor(QColor color);
};

#endif // LC_PENWIZARD_H
