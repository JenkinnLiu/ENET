/**
 * @file StyleLoader.h
 * @brief 单例样式加载器，负责从 QSS 文件加载并应用样式表
 * @author 
 * @date 2025-06-16
 */
#ifndef STYLELOADER_H
#define STYLELOADER_H

#include <memory>
#include <QString>
#include <QWidget>

/**
 * @class StyleLoader
 * @brief 提供单例接口，用于加载和应用样式表
 */
class StyleLoader
{
public:
    /**
     * @brief 析构函数
     */
    ~StyleLoader();

    /**
     * @brief 获取单例实例
     * @return StyleLoader* 单例指针
     */
    static StyleLoader* getInstance();

    /**
     * @brief 从文件加载样式并应用到目标控件
     * @param filepath 样式表文件路径（QSS）
     * @param w 目标 QWidget 指针
     */
    void loadStyle(const QString& filepath, QWidget* w);

private:
    /**
     * @brief 私有构造函数，防止外部实例化
     */
    StyleLoader();

    static std::unique_ptr<StyleLoader> instance_;  ///< 单例实例
};

#endif // STYLELOADER_H
