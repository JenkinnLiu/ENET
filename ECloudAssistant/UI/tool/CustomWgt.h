/**
 * @file CustomWgt.h
 * @brief 自定义列表项控件，显示图标与文本，并支持高亮状态切换
 * @date 2025-06-17
 */
#ifndef CUSTOMWGT_H
#define CUSTOMWGT_H

#include <QLabel>
#include <vector>
#include <QWidget>

/**
 * @class CustomWgt
 * @brief 带图标和文本的自定义控件，用于列表项显示，支持高亮切换
 */
class CustomWgt : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数，初始化控件大小并调用 Init 创建内部布局
     * @param parent 父控件指针
     */
    explicit CustomWgt(QWidget *parent = nullptr);

    /**
     * @brief 析构函数，清理资源
     */
    ~CustomWgt();

    /**
     * @brief 切换高亮状态，显示不同的图标
     * @param flag 是否高亮，true 显示高亮图标
     */
    void setHightLight(bool flag = true);

    /**
     * @brief 设置显示文本及正常/高亮图标路径
     * @param text 文本内容
     * @param normal 正常状态图标路径
     * @param hightlight 高亮状态图标路径
     * @param ishigtlight 默认是否高亮显示
     */
    void setImageAndText(const QString& text, const QString& normal,
                         const QString& hightlight, bool ishigtlight = false);
protected:
    /**
     * @brief 初始化内部布局，包括图标和文本标签
     */
    void Init();

    /**
     * @brief 设置标签文本样式
     * @param text 要显示的文本
     */
    void setLableTxt(const QString& text);

    /**
     * @brief 加载并显示图标
     * @param imagepath 图标文件路径
     */
    void setPicture(const QString& imagepath);
private:
    QLabel* name_;                  ///< 文本标签
    QLabel* image_;                 ///< 图标标签
    std::vector<QString> imagePath_;///< 图标路径数组，[0]非高亮，[1]高亮
};

#endif // CUSTOMWGT_H
