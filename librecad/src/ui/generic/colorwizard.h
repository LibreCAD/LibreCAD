#ifndef COLORWIZARD_H
#define COLORWIZARD_H

#include <QFrame>
#include <QColor>

class QListWidgetItem;

namespace Ui {
class ColorWizard;
}

class ColorWizard : public QFrame
{
    Q_OBJECT

public:
    explicit ColorWizard(QWidget* parent = 0);
    ~ColorWizard();

    QStringList getFavList();
    void addFavorite(QString color);

private:
    Ui::ColorWizard* ui;

signals:
    void requestingColorChange(QColor);
    void requestingSelection(QColor);
    void colorDoubleClicked(QColor);

protected slots:
    void requestColorChange();
    void requestSelection();
    void invokeColorDialog();
    void addOrRemove();
    void removeFavorite();
    void handleDoubleClick(QListWidgetItem* item);
};

#endif // COLORWIZARD_H
