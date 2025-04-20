#ifndef COMBOBOXOPTION_H
#define COMBOBOXOPTION_H
#include <QFrame>

namespace Ui{
    class ComboBoxOption;
}

class ComboBoxOption : public QFrame{
    Q_OBJECT
public:
    explicit ComboBoxOption(QWidget* parent);
    ~ComboBoxOption();
    void setTitle(const QString& title) const;
    void setOptionsList(const QStringList& options) const;
    void setCurrentOption(const QString& option);
protected:
    int m_lastSavedIndex;
signals:
    void optionToSave(QString);
private slots:
    void saveIndexAndEmitOption();
    void setButtonState(int);
private:
    Ui::ComboBoxOption* ui;
};

#endif // COMBOBOXOPTION_H
