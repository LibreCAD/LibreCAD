#ifndef QG_TRUECOLORDLG_H
#define QG_TRUECOLORDLG_H

#include <QWidget>
#include <QDialog>
#include <QPushButton>
#include <QColorDialog>
#include <QColor>


class QG_TrueColorDlg : public QColorDialog
{
    Q_OBJECT

public:
    QG_TrueColorDlg(const QColor &initial, QWidget *parent = nullptr, bool noButton = false);
    ~QG_TrueColorDlg() {}
    void setByLayer() { m_index = 256; }
    void setByBlock() { m_index = 257; }
    int getIndex() { return m_index; }

    static QColor getColor(const QColor &initial = Qt::white, QWidget *parent = nullptr, const QString &title = QString(), bool noButton = false,
                                  ColorDialogOptions options = ColorDialogOptions())
    {
        QG_TrueColorDlg dlg(initial, parent, noButton);
        if (!title.isEmpty())
            dlg.setWindowTitle(title);
        dlg.setOptions(options);
        dlg.setCurrentColor(initial);

        if (dlg.exec())
            return dlg.selectedColor();
        else
            return QColor();
    }

private:
    int m_index;
};

#endif // QG_TRUECOLORDLG_H
