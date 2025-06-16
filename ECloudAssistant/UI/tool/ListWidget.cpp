#include "ListWidget.h"
#include "CustomWgt.h"
#include <QMouseEvent>

ListWidget::ListWidget(QWidget *parent)
    :QListWidget(parent)
{
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground);
    setCurrentRow(0);
}

ListWidget::~ListWidget()
{

}

void ListWidget::AddWidget(CustomWgt *widget)
{
    QListWidgetItem* listWidgetItem = new QListWidgetItem(this);
    listWidgetItem->setSizeHint(widget->sizeHint());
    this->setItemWidget(listWidgetItem, widget);
}

void ListWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::LeftButton)
    {
        QListWidgetItem* item = itemAt(event->pos());
        if(item)
        {
            int index = row(item);
            //发送信号
            emit itemCliked(index);
        }
        QListWidget::mousePressEvent(event);
    }
}
