#ifndef COLORCOMBOBOX_H
#define COLORCOMBOBOX_H

#include <QComboBox>

class ColorComboBox : public QComboBox{
    Q_OBJECT
public:
    explicit ColorComboBox(QWidget* parent);
};

#endif // COLORCOMBOBOX_H
