#ifndef MAINWGT_H
#define MAINWGT_H
#include <QWidget>
#include <QStackedWidget>

class LoginWgt;
class RemoteWgt;
class MainWgt : public QWidget
{
    Q_OBJECT
public:
    explicit MainWgt(QWidget *parent = nullptr);
public slots:
    void slot_ItemCliked(int index);
private:
    QStackedWidget* stackWgt_;
    LoginWgt* login_;
    RemoteWgt* remoteWgt_;
    QWidget* deviceWgt_;
    QWidget* settingWgt_;
};

#endif // MAINWGT_H
