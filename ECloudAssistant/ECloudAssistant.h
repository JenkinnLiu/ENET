#ifndef ECLOUDASSISTANT_H
#define ECLOUDASSISTANT_H

#include <QWidget>
class MainWgt;
class TitleWgt;
class ListInfoWgt;

class ECloudAssistant : public QWidget
{
    Q_OBJECT

public:
    ECloudAssistant(QWidget *parent = nullptr);
    ~ECloudAssistant();
protected:
    void mouseMoveEvent(QMouseEvent* event)override;
    void mousePressEvent(QMouseEvent* event)override;
    void mouseReleaseEvent(QMouseEvent* event)override;
private:
    QPoint point_;
    bool is_press_ = false;
    MainWgt* mainWgt_;
    TitleWgt* titleWgt_;
    ListInfoWgt* listWgt_;
};
#endif // ECLOUDASSISTANT_H
