#ifndef REMOTEWGT_H
#define REMOTEWGT_H
#include <QLineEdit>
#include <QWidget>
#include <QPushButton>
#include "RemoteManager.h"

class RemoteWgt : public QWidget
{
    Q_OBJECT
public:
    explicit RemoteWgt(QWidget *parent = nullptr);
public slots:
    void handleLogined(const std::string ip, uint16_t port);
private:
    QString ip_ = "";
    uint16_t port_ = -1;
    EventLoop* loop_ = nullptr;
    QLineEdit* selfCodeEdit_;
    QLineEdit* rmoteCodeEdit_;
    QPushButton* startRmoteBtn_;
    std::unique_ptr<RemoteManager> manager_;
};

#endif // REMOTEWGT_H
