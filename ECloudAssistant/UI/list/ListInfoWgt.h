/**
 * @file ListInfoWgt.h
 * @brief 定义侧边栏列表信息控件，用于显示用户按钮及功能列表，并发出选择信号
 * @date 2025-06-17
 */

#ifndef LISTINFOWGT_H
#define LISTINFOWGT_H

#include <QWidget>
#include <QPushButton>

class CustomWgt;
class ListWidget;

/**
 * @class ListInfoWgt
 * @brief 包含用户按钮与功能列表项的侧边栏控件，负责布局与信号转发
 */
class ListInfoWgt : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数，初始化窗口属性、子控件并设置布局
     * @param parent 父控件指针
     */
    explicit ListInfoWgt(QWidget *parent = nullptr);

signals:
    /**
     * @brief 功能选择信号，通知主界面切换页面
     * @param index 被选择功能项的索引，0 表示用户按钮，其余对应列表项顺序
     */
    void sig_Select(int index);

protected slots:
    /**
     * @brief 列表项点击处理槽，设置项高亮并发出选择信号
     * @param index 被点击项的索引
     */
    void HandleItemSelect(int index);

private:
    QPushButton* userBtn_;                 ///< 用户按钮，点击切换到用户主页
    ListWidget* listWgt_;                  ///< 承载功能项的列表控件
    std::vector<CustomWgt*> customWgts_;    ///< 存储各功能项，用于高亮切换
};

#endif // LISTINFOWGT_H
