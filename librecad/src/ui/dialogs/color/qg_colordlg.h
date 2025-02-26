#ifndef QG_COLORDLG_H
#define QG_COLORDLG_H

#include <QWidget>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QColor>
#include <QColorDialog>
#include <QCloseEvent>

#include "rs_color.h"
#include "qg_colorwell.h"

class QG_ColorDlgOptions
{
public:
    static const int NoButton = 2;
    static const int DXFIndex = 4;
    static const int TrueType = 8;
    static const int Tab = 4|8;
};

class QG_ColorDlg : public QDialog
{
    Q_OBJECT

public:
    QG_ColorDlg(QWidget *parent = nullptr, int options = QG_ColorDlgOptions::Tab, int initial=7, const QColor qinitial=QColor());
    ~QG_ColorDlg();

    static bool getIndexColor(int &result, QWidget *parent = nullptr, int initial=7, bool buttons=true)
    {
        int opt = 0;
        opt |= QG_ColorDlgOptions::DXFIndex;

        if(!buttons)
        {
            opt |= QG_ColorDlgOptions::NoButton;
        }

        QG_ColorDlg dlg(parent, opt, initial);
        if (dlg.exec())
        {
            result = dlg.getIndex();
            return true;
        }
        else
            return false;
    }

    static bool getTrueColor(int &tresult, int &result, QWidget *parent = nullptr, int tinitial=-1, int initial=-1, bool buttons=true, int tbyinitial=-1, int byinitial=-1)
    {
        int opt = 0;
        opt |= QG_ColorDlgOptions::Tab;

        if(!buttons)
        {
            opt |= QG_ColorDlgOptions::NoButton;
        }

        QG_ColorDlg dlg(parent, opt, initial, tinitial > -1 ? QColor(tinitial >> 16, tinitial >> 8 & 0xFF, tinitial & 0xFF) : QColor());
        if (dlg.exec())
        {
            tresult = dlg.getTrueType().rgb();
            result = dlg.getIndex();

            if (byinitial != -1 && (dlg.getIndex() == 0 || dlg.getIndex() == 256))
            {
                result = byinitial;
            }

            if (tbyinitial != -1 && (dlg.getIndex() == 0 || dlg.getIndex() == 256))
            {
                tresult = tbyinitial;
            }

            return true;
        }
        else
            return false;
    }

    static QColor getTrueTypeColor(QWidget *parent = nullptr, QColor initial=QColor(Qt::white))
    {
        QColor color = QColorDialog::getColor(initial, parent, "");
        return color;
    }

    static QColor getColor(QWidget *parent = nullptr, QColor initial=QColor(Qt::white), bool buttons=true)
    {
        int opt = 0;
        opt |= QG_ColorDlgOptions::Tab;

        if(!buttons)
        {
            opt |= QG_ColorDlgOptions::NoButton;
        }

        int icolor = RS_DXFColor::fromQColor(initial);
        if (icolor == -1)
        {
            icolor = 7;
        }

        QG_ColorDlg dlg(parent, opt, icolor);
        if (dlg.exec())
            return dlg.getIndex();
        else
            return initial;
    }

    const QColor getTrueType() { return m_qcolor; }
    int getIndex() { return index; }

public slots:
    void accept() override;

private slots:
    void editChanged(const QString &text);
    void indexColorChanged(int color);
    void colorButtonChanged(int color);
    void currentTabChanged(int index);
    void setByLayer();
    void setByBlock();
    void helpRequested();

private:
    int m_options;
    const QColor m_initial;
    QColor m_qcolor;
    int index;
    unsigned int mode;
    QTabWidget *tabWidget;
    QWidget *indexWidget;
    QColorDialog *trueTypeWidget;
    QDialogButtonBox *buttonBox;

    QColorWell *solidColorPalette;
    QColorWell *pastelColorPalette;
    QColorWell *nameColorPalette;
    QColorWell *shadeColorPalette;
    QLineEdit *edit;
    QLabel *indexLabel;
    QLabel *rgbLabel;
    QLabel *rgbNumLabel;
    QG_ColorPromptButton *colorButton;
    QPushButton *buttonLayer;
    QPushButton *buttonBlock;

    void setIndex(int color);
    int validIndexColor(const QString &text);
};
#endif // QG_COLORDLG_H
