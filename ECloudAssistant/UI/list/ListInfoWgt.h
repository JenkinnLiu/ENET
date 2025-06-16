#ifndef LISTINFOWGT_H
#define LISTINFOWGT_H
#include <QWidget>
#include <QPushButton>
class CustomWgt;
class ListWidget;
class ListInfoWgt : public QWidget
{
    Q_OBJECT
public:
    explicit ListInfoWgt(QWidget *parent = nullptr);
signals:
    void sig_Select(int index);
protected slots:
    void HandleItemSelect(int index);
private:
    QPushButton* userBtn_;
    ListWidget* listWgt_;
    std::vector<CustomWgt*> customWgts_;
};

#endif // LISTINFOWGT_H
