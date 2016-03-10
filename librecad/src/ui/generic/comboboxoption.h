#ifndef COMBOBOXOPTION_H
#define COMBOBOXOPTION_H

#include <QFrame>
#include <QStringList>

namespace Ui {
class ComboBoxOption;
}

class ComboBoxOption : public QFrame
{
    Q_OBJECT

public:
    explicit ComboBoxOption(QWidget* parent);
    ~ComboBoxOption();

    void setTitle(const QString& title);
    void setOptionsList(const QStringList& options);
    void setCurrentOption(const QString& option);

protected:
    int last_saved_index;

signals:
    void optionToSave(QString);

private slots:
    void saveIndexAndEmitOption();
    void setButtonState(int);

private:
    Ui::ComboBoxOption* ui;
};

#endif // COMBOBOXOPTION_H
