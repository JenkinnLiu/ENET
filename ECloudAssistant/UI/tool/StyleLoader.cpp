#include "StyleLoader.h"
#include <mutex>
#include <QFile>

std::unique_ptr<StyleLoader> StyleLoader::instance_ = nullptr;

StyleLoader::~StyleLoader()
{

}

StyleLoader *StyleLoader::getInstance()
{
    static std::once_flag flag;
    std::call_once(flag,[&](){
        instance_.reset(new StyleLoader());
    });
    return instance_.get();
}

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

StyleLoader::StyleLoader()
{

}

