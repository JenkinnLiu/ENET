/**
 * @file CustomWgt.cpp
 * @brief 自定义列表项控件实现，支持图标与文本显示及高亮切换
 * @date 2025-06-17
 */

#include "CustomWgt.h"
#include <QHBoxLayout>
#include <QFont>

/**
 * @brief 构造函数，初始化无边框控件大小并调用 Init 完成布局搭建
 */
CustomWgt::CustomWgt(QWidget *parent)
    : QWidget{parent}
    ,imagePath_(2,"")
{
    setWindowFlag(Qt::FramelessWindowHint);
    setFixedSize(180,30);
    Init();
}

/**
 * @brief 析构函数，清理图标路径数组资源
 */
CustomWgt::~CustomWgt()
{
    imagePath_.clear();
}

/**
 * @brief 切换图标高亮状态
 * @param flag true 为高亮，false 为正常状态
 */
void CustomWgt::setHightLight(bool flag)
{
    const QString str = flag ? imagePath_[1] : imagePath_[0];
    setPicture(str);
}

/**
 * @brief 设置文本及对应正常/高亮图标路径，初始化显示状态
 * @param text 显示文本
 * @param normal 正常状态图标路径
 * @param hightlight 高亮状态图标路径
 * @param ishigtlight 是否初始为高亮显示
 */
void CustomWgt::setImageAndText(const QString &text, const QString &normal, const QString &hightlight, bool ishigtlight)
{
    if(normal.isEmpty() || hightlight.isEmpty())
    {
        return;
    }
    imagePath_[0] = normal;
    imagePath_[1] = hightlight;
    const QString str = ishigtlight ? hightlight : normal;
    setPicture(str);
    setLableTxt(text);
}

/**
 * @brief 内部方法，创建 QLabel 布局并配置样式
 */
void CustomWgt::Init()
{
    name_ = new QLabel(this);
    image_ = new QLabel(this);
    image_->setFixedSize(20,20);
    name_->setFixedSize(100,20);

    image_->setStyleSheet("background-color:transparent;border:none;");
    name_->setStyleSheet("background-color:transparent;border:none;");

    QHBoxLayout* layout = new QHBoxLayout(this);

    layout->addWidget(image_);
    layout->addWidget(name_);
    setLayout(layout);
}

/**
 * @brief 更新文本标签内容和样式
 * @param text 新文本内容
 */
void CustomWgt::setLableTxt(const QString &text)
{
    name_->setText(text);
    QFont font("Microsoft Yahei",11);
    name_->setFont(font);
    name_->setStyleSheet("color:#FFFFFF");
}

/**
 * @brief 加载并设置图标到 image_ 标签
 * @param imagepath 图标文件路径
 */
void CustomWgt::setPicture(const QString &imagepath)
{
    QPixmap pixmap;
    if(pixmap.load(imagepath))
    {
        image_->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
        image_->setPixmap(pixmap.scaled(image_->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }
}
