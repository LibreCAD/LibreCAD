#include "qg_colordlg.h"
#include "rs_filterdxfrw.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpression>

QG_ColorDlg::QG_ColorDlg(QWidget *parent, int options, int initial, const QColor qinitial)
    : QDialog(parent)
    , m_options(options)
    , m_initial(qinitial)
    , m_qcolor(qinitial)
{
    mode = 0;
    QVBoxLayout *mainLayout = new QVBoxLayout;

    if (QG_ColorDlgOptions::DXFIndex & m_options)
    {
        mode = 1;
        QVBoxLayout *indexLayout = new QVBoxLayout;
        QHBoxLayout *infoLayout = new QHBoxLayout;
        QHBoxLayout *buttonLayout = new QHBoxLayout;
        QHBoxLayout *selectLayout = new QHBoxLayout;
        QVBoxLayout *editLayout = new QVBoxLayout;

        indexWidget = new QWidget;
        indexWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        solidColorPalette = new QColorWell(this, QColorWell::Solid);
        pastelColorPalette = new QColorWell(this, QColorWell::Pastel);
        nameColorPalette = new QColorWell(this, QColorWell::Name);
        shadeColorPalette = new QColorWell(this, QColorWell::Shade);

        connect(pastelColorPalette, &QColorWell::indexColorChanged, this, &QG_ColorDlg::indexColorChanged);
        connect(shadeColorPalette, &QColorWell::indexColorChanged, this, &QG_ColorDlg::indexColorChanged);
        connect(solidColorPalette, &QColorWell::indexColorChanged, this, &QG_ColorDlg::indexColorChanged);
        connect(nameColorPalette, &QColorWell::indexColorChanged, this, &QG_ColorDlg::indexColorChanged);

        edit = new QLineEdit;
        edit->setMaximumWidth(100);

        connect(edit, &QLineEdit::textChanged, this, &QG_ColorDlg::editChanged);

        indexLabel = new QLabel(tr("Index color: "));
        rgbLabel = new QLabel(tr("Red, Green, Blue: "));
        rgbNumLabel = new QLabel(tr("???, ???, ???"));
        rgbNumLabel->setMinimumWidth(76);
        colorButton = new QG_ColorPromptButton(initial);

        connect(colorButton, &QG_ColorPromptButton::colorChanged, this, &QG_ColorDlg::indexColorChanged);
        connect(colorButton, &QG_ColorPromptButton::colorChanged, this, &QG_ColorDlg::colorButtonChanged);

        colorButton->setMinimumHeight(64);
        colorButton->setMinimumWidth(64);
        colorButton->setMaximumHeight(64);
        colorButton->setMaximumWidth(64);

        infoLayout->addWidget(indexLabel);
        infoLayout->addStretch();
        infoLayout->addWidget(rgbLabel);
        infoLayout->addWidget(rgbNumLabel);

        indexLayout->addWidget(new QLabel(tr("LibreCAD Color Index (LCI):")));
        indexLayout->addWidget(solidColorPalette);
        indexLayout->addWidget(pastelColorPalette);
        indexLayout->addLayout(infoLayout);
        indexLayout->addSpacing(4);

        buttonLayout->addWidget(nameColorPalette);
        buttonLayout->addStretch();

        if (!(QG_ColorDlgOptions::NoButton & m_options))
        {
            buttonLayer = new QPushButton(tr("By &Layer"));
            buttonBlock = new QPushButton(tr("By Bloc&k"));

            buttonLayer->setMaximumWidth(80);
            buttonBlock->setMaximumWidth(80);

            connect(buttonLayer, &QPushButton::pressed, this, &QG_ColorDlg::setByLayer);
            connect(buttonBlock, &QPushButton::pressed, this, &QG_ColorDlg::setByBlock);

            buttonLayout->addWidget(buttonLayer);
            buttonLayout->addWidget(buttonBlock);
        }

        indexLayout->addLayout(buttonLayout);

        editLayout->addWidget(shadeColorPalette);
        editLayout->addSpacing(4);
        editLayout->addWidget(new QLabel(tr("Color:")));
        editLayout->addWidget(edit);

        selectLayout->addLayout(editLayout);
        selectLayout->addStretch();
        selectLayout->addWidget(colorButton);

        indexLayout->addLayout(selectLayout);
        indexWidget->setLayout(indexLayout);
    }

    if (QG_ColorDlgOptions::TrueType & m_options)
    {
        mode = 2;
        trueTypeWidget = new QColorDialog(m_initial, this);
        trueTypeWidget->setOption(QColorDialog::NoButtons);

        trueTypeWidget->setWindowFlags(Qt::Widget);
        //trueTypeWidget->setOption(QColorDialog::DontUseNativeDialog);
        trueTypeWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                   | QDialogButtonBox::Cancel
                                   | QDialogButtonBox::Help);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested, this, &QG_ColorDlg::helpRequested);

    if ((QG_ColorDlgOptions::TrueType & m_options) &&
            (QG_ColorDlgOptions::DXFIndex & m_options))
    {
        mode = 3;
        tabWidget = new QTabWidget;
        tabWidget->addTab(indexWidget, tr("Index Color"));
        tabWidget->addTab(trueTypeWidget, tr("True Color"));

        connect(tabWidget, &QTabWidget::currentChanged, this, &QG_ColorDlg::currentTabChanged);

        if (m_initial != QColor())
        {
            tabWidget->setCurrentIndex(1);
        }

        mainLayout->addWidget(tabWidget);
    }
    else if (QG_ColorDlgOptions::DXFIndex & m_options)
    {
        mainLayout->addWidget(indexWidget);
    }
    else
    {
        mainLayout->addWidget(trueTypeWidget);
    }

    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    setIndex(initial);
    setWindowTitle(tr("LibreCAD Color Dialog"));
}

QG_ColorDlg::~QG_ColorDlg()
{
}

void QG_ColorDlg::accept()
{
    int color = validIndexColor(edit->text());

    switch (mode) {
    case 1:
    {
        if(color >= 0)
        {
            if(color == index)
            {
                QDialog::accept();
            }
            else
            {
                index = color;
                QDialog::accept();
            }
        }
    }
        break;
    case 2:
    {
        m_qcolor = trueTypeWidget->selectedColor();
        QDialog::accept();
    }
        break;
    case 3:
    {
        if(tabWidget->currentIndex() == 0)
        {
            if(color > 1)
            {
                if(color != index)
                {
                    index = color;
                }
            }

            const QColor qc = RS_DXFColor::toQColor(index);
            m_qcolor = qc;
        }
        else
        {
            RS_Color color(m_qcolor);
            m_qcolor = trueTypeWidget->selectedColor();
            RS_FilterDXFRW::colorToNumber(color, &index);
        }
        QDialog::accept();
    }
        break;
    default:
        break;
    }
}

void QG_ColorDlg::editChanged(const QString &text)
{
    if (text.isNull())
    {
        return;
    }

    int color = validIndexColor(text);

    if(color > -2)
    {
        if (colorButton->getColor() != color)
        {
            colorButton->setColor(color);
        }
        return;
    }
    edit->setText(edit->text().chopped(1));
}

void QG_ColorDlg::indexColorChanged(int color)
{
    setIndex(color);
}

void QG_ColorDlg::colorButtonChanged(int color)
{
    RS_DXFColor c(color);

    if(c.hasName())
    {
        nameColorPalette->setCurrentColor(color);
    }
    else if(c.shade())
    {
        shadeColorPalette->setCurrentColor(color);
    }
    else if(c.solid())
    {
        solidColorPalette->setCurrentColor(color);
    }
    else if(c.pastel())
    {
        pastelColorPalette->setCurrentColor(color);
    }
    else {}
}

void QG_ColorDlg::currentTabChanged(int /*index*/)
{
#if 0
    //resize();
    qDebug() << "[QG_ColorDlg::currentTabChanged index" << index;

    for(int i = 0; i < tabWidget->count(); i++)
    {
        if(i != index)
        {
            tabWidget->widget(i)->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        }
    }

    tabWidget->widget(index)->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    tabWidget->widget(index)->resize(tabWidget->widget(index)->minimumSizeHint());
    tabWidget->widget(index)->adjustSize();
    resize(minimumSizeHint());
    adjustSize();
#endif
}

void QG_ColorDlg::setIndex(int color)
{
    if (m_options & QG_ColorDlgOptions::DXFIndex)
    {
        if (color == 256)
        {
            indexLabel->setText(tr("Index color: ") + "256");
            edit->setText(tr("BYLAYER"));
            rgbNumLabel->setText("");
            colorButton->setColor(7);
        }

        if (color == 0)
        {
            indexLabel->setText(tr("Index color: ") + "0");
            edit->setText(tr("BYBLOCK"));
            rgbNumLabel->setText("");
            colorButton->setColor(7);
        }
        else
        {
            RS_DXFColor c(color);

            indexLabel->setText(tr("Index color: ") + QString::number(c.color()));
            edit->setText(tr(c.getName()));

            rgbNumLabel->setText(QString::number(c.r())
                                 + ", "
                                 + QString::number(c.g())
                                 + ", "
                                 + QString::number(c.b())
                                 );
            colorButton->setColor(color);
        }

        index = color;
    }
}

void QG_ColorDlg::setByLayer()
{
    edit->setText(tr("BYLAYER"));
}

void QG_ColorDlg::setByBlock()
{
    edit->setText(tr("BYBLOCK"));
}

void QG_ColorDlg::helpRequested()
{
    qDebug() << "[QG_ColorDlg::helpRequested]";
}

int QG_ColorDlg::validIndexColor(const QString &text)
{
    static const QRegularExpression intRegExpr(QStringLiteral("^[0-9]+"));
    QRegularExpressionMatch match = intRegExpr.match(text);

    if (match.hasMatch() && RS_DXFColor::isValid(text.toInt()))
    {
        return text.toInt();
    }

    if (text == tr("BYLAYER"))
    {
        return 256;
    }

    if (text == tr("BYBLOCK"))
    {
        return 0;
    }

    for (unsigned int i = 0; i < MAX_DXF_IDX_NAME_COLOR; i++)
    {
        const QString name = RS_DXFColor::nameList()[i];

        if (name == text)
        {
            return i;
        }

        if (name.startsWith(text))
        {
            return -1;
        }
    }

    return -2;
}


