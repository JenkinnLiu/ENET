#ifndef AVPLAYER_H
#define AVPLAYER_H
#include "OpenGLRender.h"
#include "AudioRender.h"
#include "AVDEMuxer.h"

class EventLoop;
class SigConnection;
class AVPlayer : public OpenGLRender ,public AudioRender
{
    Q_OBJECT
public:
    ~AVPlayer();
    explicit AVPlayer(EventLoop* loop,QWidget* parent = nullptr);
    bool Connect(QString ip,uint16_t port,QString code);
signals:
    void sig_repaint(AVFramePtr frame);
protected:
    void audioPlay();
    void videoPlay();
    void Init();
    void Close();
    virtual void resizeEvent(QResizeEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;
private:
    void HandleStopStream();
    bool HandleStartStream(const QString& streamAddr);
private:
    bool stop_ = false;
    EventLoop* loop_;
    AVContext* avContext_ = nullptr;
    std::shared_ptr<SigConnection> sig_conn_;
    std::unique_ptr<AVDEMuxer> avDEMuxer_ = nullptr;
    std::unique_ptr<std::thread> audioThread_ = nullptr;
    std::unique_ptr<std::thread> videoThread_ = nullptr;
};
#endif // AVPLAYER_H
