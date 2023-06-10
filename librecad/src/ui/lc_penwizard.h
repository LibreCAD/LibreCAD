#ifndef LC_PENWIZARD_H
#define LC_PENWIZARD_H

#include <QDockWidget>

class ColorWizard;
class QC_MDIWindow;

class LC_PenWizard : public QDockWidget
{
    Q_OBJECT

public:
    LC_PenWizard(const QString& title, QWidget* parent = nullptr);

    void setMdiWindow(QC_MDIWindow* mdiWindow);

protected slots:
    void setColorForSelected(QColor color);
    void selectByColor(QColor color);
    void setActivePenColor(QColor color);

private:
    QC_MDIWindow* mdi_win = nullptr;
    ColorWizard* color_wiz = nullptr;

};

#endif // LC_PENWIZARD_H
