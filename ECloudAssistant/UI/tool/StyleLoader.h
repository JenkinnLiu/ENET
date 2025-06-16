#ifndef STYLELOADER_H
#define STYLELOADER_H
#include <memory>
#include <QString>
#include <QWidget>

class StyleLoader
{
public:
    ~StyleLoader();
    static StyleLoader* getInstance();
    void loadStyle(const QString& filepath,QWidget* w);
private:
    StyleLoader();
    static std::unique_ptr<StyleLoader> instance_;
};

#endif // STYLELOADER_H
