/**
 * @file OpenGLRender.h
 * @brief 定义了 OpenGLRender 类，一个用于渲染视频帧的 QOpenGLWidget。
 * @details
 * 这个类继承自 QOpenGLWidget 和 QOpenGLFunctions_3_3_Core，专门设计用于
 * 高效地渲染 YUV420P 格式的视频流。它通过自定义的顶点和片段着色器
 * 将YUV数据在GPU上转换为RGB格式并显示出来，从而获得高性能的视频播放效果。
 * 同时，它还处理窗口的缩放以保持视频的原始宽高比，并能计算鼠标
 * 在视频画面上的相对位置。
 */
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

/**
 * @class OpenGLRender
 * @brief 使用 QOpenGLWidget 和 QOpenGLFunctions_3_3_Core 实现的视频渲染窗口。
 * @details
 * 专门用于高效渲染 YUV420P 格式的视频帧。
 * 它通过OpenGL着色器将YUV数据转换为RGB并在屏幕上显示。
 * 该类管理着OpenGL的初始化、资源（纹理、着色器、VBO/VAO）的生命周期、
 * 渲染循环以及窗口事件的处理。
 */
class OpenGLRender : public QOpenGLWidget, protected  QOpenGLFunctions_3_3_Core
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     * @param f 窗口标志
     */
    explicit OpenGLRender(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    /**
     * @brief 删除拷贝构造函数，确保类的实例唯一。
     */
    OpenGLRender(const OpenGLRender&) = delete;

    /**
     * @brief 删除赋值运算符，确保类的实例唯一。
     */
    OpenGLRender& operator=(const OpenGLRender&) = delete;

    /**
     * @brief 析构函数
     * @details 负责释放所有OpenGL相关的资源，如纹理、着色器程序、VBO、VAO等。
     */
    virtual ~OpenGLRender();

public:
    /**
     * @brief 接收并准备渲染新的视频帧
     * @param frame 包含待渲染视频数据的 AVFrame 智能指针
     * @details 此函数是外部调用以更新视频画面的主要接口。
     *          它会触发OpenGL的重绘事件。
     */
    virtual void Repaint(AVFramePtr frame);

    /**
     * @brief 获取鼠标在视频帧上的相对位置比例
     * @param[out] body 用于存储计算出的位置比例的结构体
     * @details 将窗口内的绝对鼠标坐标转换为相对于视频画面的百分比坐标。
     *          这对于远程控制等功能非常有用。
     */
    void GetPosRation(MouseMove_Body& body);

protected:
    /**
     * @brief 窗口显示事件处理
     * @param event 事件对象
     * @details 在窗口首次显示时，展示一个加载动画。
     */
    virtual void showEvent(QShowEvent *event);

    /**
     * @brief 初始化 OpenGL 资源。
     * @details 此函数在第一次调用 paintGL() 或 resizeGL() 之前被调用一次。
     *          它负责设置着色器、顶点缓冲区和其他 OpenGL 对象。
     */
    virtual void initializeGL() override;

    /**
     * @brief 处理窗口部件的尺寸调整事件。
     * @param w 窗口部件的新宽度。
     * @param h 窗口部件的新高度。
     * @details 调整视口并保持视频的宽高比。
     */
    virtual void resizeGL(int w, int h) override;

    /**
     * @brief 渲染 OpenGL 场景。
     * @details 每当窗口部件需要被绘制时，此函数就会被调用。
     *          它使用准备好的纹理和着色器绘制视频帧。
     */
    virtual void paintGL() override;

private:
    /**
     * @brief 使用新的帧数据更新 YUV 纹理。
     * @param frame 包含新 YUV 数据的 AVFrame。
     */
    void repaintTexYUV420P(AVFramePtr frame);

    /**
     * @brief 如果 YUV 纹理不存在或视频分辨率发生变化，则初始化它们。
     * @param frame 用于确定纹理尺寸和格式的 AVFrame。
     */
    void initTexYUV420P(AVFramePtr frame);

    /**
     * @brief 释放已分配的 YUV 纹理。
     */
    void freeTexYUV420P();

private:
    QLabel* label_ = nullptr; ///< 用于显示加载动画的标签。

    QOpenGLTexture* texY_ = nullptr; ///< Y (亮度) 分量的纹理。
    QOpenGLTexture* texU_ = nullptr; ///< U (色度) 分量的纹理。
    QOpenGLTexture* texV_ = nullptr; ///< V (色度) 分量的纹理。
    QOpenGLShaderProgram* program_ = nullptr; ///< 用于将 YUV 转换为 RGB 的着色器程序。
    QOpenGLPixelTransferOptions options_; ///< 像素数据传输选项。

    GLuint VBO = 0; ///< 顶点缓冲对象 ID。
    GLuint VAO = 0; ///< 顶点数组对象 ID。
    GLuint EBO = 0; ///< 元素缓冲对象 ID。
    QSize   m_size; ///< 输入视频帧的尺寸。
    QSizeF  m_zoomSize; ///< 视频帧缩放后以适应窗口部件并保持宽高比的尺寸。
    QRect   m_rect;   ///< 视频在窗口部件内实际绘制的矩形区域。
    QPointF m_pos;    ///< 视频绘制矩形的左上角位置。
};
#endif // OPENGLRENDER_H
