#include "TitleWgt.h"
#include <QHBoxLayout>
#include "StyleLoader.h"

TitleWgt::TitleWgt(QWidget *parent)
    : QWidget{parent}
{
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground);
    setFixedSize(600,30);

    minBtn_ = new QPushButton(this);
    closeBtn_ = new QPushButton(this);
    minBtn_->setFixedSize(30,30);
    closeBtn_->setFixedSize(30,30);

    closeBtn_->setObjectName("close_Btn");
    minBtn_->setObjectName("min_Btn");

    connect(minBtn_,&QPushButton::clicked,this,[this](){
        this->parentWidget()->showMinimized();
    });

    connect(closeBtn_,&QPushButton::clicked,this,[this](){
        this->parentWidget()->close();
    });

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addStretch(1);
    layout->addWidget(minBtn_);
    layout->addWidget(closeBtn_);
    layout->setContentsMargins(0,0,0,0);
    setLayout(layout);

    StyleLoader::getInstance()->loadStyle(":/UI/brown/main.css",this);
}
