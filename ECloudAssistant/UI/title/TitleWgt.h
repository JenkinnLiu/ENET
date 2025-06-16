#ifndef TITLEWGT_H
#define TITLEWGT_H
#include <QWidget>
#include <QPushButton>

class TitleWgt : public QWidget
{
    Q_OBJECT
public:
    explicit TitleWgt(QWidget *parent = nullptr);

private:
    QPushButton* minBtn_;
    QPushButton* closeBtn_;
};

#endif // TITLEWGT_H
