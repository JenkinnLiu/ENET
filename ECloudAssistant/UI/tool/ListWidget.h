/**
 * @file ListWidget.h
 * @brief 自定义列表控件，用于承载自定义项控件并发出点击信号
 * @date 2025-06-16
 */

#include <QListWidget>

class CustomWgt;
/**
 * @class ListWidget
 * @brief 扩展自 QListWidget，支持添加自定义 QWidget 项并发出点击索引信号
 */
class ListWidget : public QListWidget
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数，初始化列表样式和行为
     * @param parent 父控件指针
     */
    ListWidget(QWidget* parent = nullptr);
    ~ListWidget();
    /**
     * @brief 构造一个列表项并插入自定义控件
     * @param widget 自定义项控件指针
     */
    void AddWidget(CustomWgt *widget);
signals:
    /**
     * @brief 项目单击信号，返回被点击项的索引
     * @param index 点击项的行索引
     */
    void itemCliked(int index);
protected:
    /**
     * @brief 鼠标按下事件，触发 itemClicked 信号
     * @param event 鼠标事件对象
     */
    void mousePressEvent(QMouseEvent* event)override;
private:

};
