#ifndef OPENGLRENDER_H
#define OPENGLRENDER_H
#include <QLabel>
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include "AV_Common.h"
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLPixelTransferOptions>
#include "defin.h"

class OpenGLRender : public QOpenGLWidget, protected  QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    explicit OpenGLRender(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    OpenGLRender(const OpenGLRender&) = delete;
    OpenGLRender& operator=(const OpenGLRender&) = delete;
    virtual ~OpenGLRender();
public:
    virtual void Repaint(AVFramePtr frame);
    void GetPosRation(MouseMove_Body& body);
protected:
    virtual void showEvent(QShowEvent *event);
    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;
private:
    void repaintTexYUV420P(AVFramePtr frame);
    void initTexYUV420P(AVFramePtr frame);
    void freeTexYUV420P();
private:
    QLabel* label_ = nullptr;

    QOpenGLTexture* texY_ = nullptr;
    QOpenGLTexture* texU_ = nullptr;
    QOpenGLTexture* texV_ = nullptr;
    QOpenGLShaderProgram* program_ = nullptr;
    QOpenGLPixelTransferOptions options_;

    GLuint VBO = 0;
    GLuint VAO = 0;
    GLuint EBO = 0;
    QSize   m_size;
    QSizeF  m_zoomSize;
    QRect   m_rect;
    QPointF m_pos;
};
#endif // OPENGLRENDER_H
