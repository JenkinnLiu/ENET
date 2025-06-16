#include <QListWidget>

class CustomWgt;
class ListWidget : public QListWidget
{
    Q_OBJECT
public:
    ListWidget(QWidget* parent = nullptr);
    ~ListWidget();
    void AddWidget(CustomWgt *widget);
signals:
    void itemCliked(int index);
protected:
    void mousePressEvent(QMouseEvent* event)override;
private:

};
