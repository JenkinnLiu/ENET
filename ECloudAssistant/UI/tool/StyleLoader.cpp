/**
 * @file StyleLoader.cpp
 * @brief 样式加载器实现，提供从 QSS 文件读取并应用样式的功能
 * @date 2025-06-16
 */

#include "StyleLoader.h"
#include <mutex>
#include <QFile>

std::unique_ptr<StyleLoader> StyleLoader::instance_ = nullptr;

/**
 * @brief 析构函数，清理资源
 */
StyleLoader::~StyleLoader()
{

}

/**
 * @brief 获取单例实例，线程安全
 * @return StyleLoader* 单例指针
 */
StyleLoader *StyleLoader::getInstance()
{
    static std::once_flag flag;
    std::call_once(flag,[&](){
        instance_.reset(new StyleLoader());
    });
    return instance_.get();
}

/**
 * @brief 从给定文件路径加载 QSS 样式并应用到指定控件
 * @param filepath 样式文件路径
 * @param w 目标 QWidget 指针
 */
void StyleLoader::loadStyle(const QString &filepath, QWidget *w)
{
    QFile file(filepath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }
    QString qss = QString::fromUtf8(file.readAll().data());
    //设置这个样式表
    w->setStyleSheet(qss);
}

/**
 * @brief 私有构造函数，防止外部创建
 */
StyleLoader::StyleLoader()
{

}

