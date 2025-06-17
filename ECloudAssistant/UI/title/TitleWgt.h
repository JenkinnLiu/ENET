/**
 * @file TitleWgt.h
 * @brief 标题栏控件，提供最小化与关闭功能
 * @date 2025-06-16
 */

#ifndef TITLEWGT_H
#define TITLEWGT_H

#include <QWidget>
#include <QPushButton>

/**
 * @class TitleWgt
 * @brief 自定义标题栏，包含最小化和关闭按钮
 */
class TitleWgt : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数，初始化按钮并设置布局及样式
     * @param parent 父窗口指针
     */
    explicit TitleWgt(QWidget *parent = nullptr);

private:
    QPushButton* minBtn_;    ///< 最小化按钮
    QPushButton* closeBtn_;  ///< 关闭按钮
};

#endif // TITLEWGT_H
