/**
 * @file OpenGLRender.cpp
 * @brief 实现了 OpenGLRender 类的功能，用于高效渲染YUV视频帧。
 * @details
 * 该文件包含了OpenGL环境的初始化、着色器的编译链接、纹理的创建与管理、
 * 视频帧数据的上传以及最终的渲染绘制逻辑。通过利用GPU的并行处理能力，
 * 实现了从YUV到RGB的实时转换和显示，保证了视频播放的流畅性。
 */

#include "OpenGLRender.h"
//添加着色器
#include <QMovie>
#include <QShowEvent>
#include <QResizeEvent>
#include "defin.h"

static GLfloat vertices[] = {
    1.0f,1.0f,0.0f,1.0f,1.0f,
    1.0f,-1.0f,0.0f,1.0f,0.0f,
    -1.0f,-1.0f,0.0f,0.0f,0.0f,
    -1.0f,1.0f,0.0f,0.0f,1.0f
};

//索引
static GLuint indices[] =
    {
    0,1,3,
    1,2,3
};

/**
 * @brief 顶点着色器源码 (GLSL)
 * @details
 * 负责处理顶点的位置和纹理坐标。
 * - `aPos`: 输入的顶点坐标 (x, y, z)。
 * - `aTexCoord`: 输入的纹理坐标 (s, t)。
 * - `gl_Position`: 输出的裁剪空间坐标，直接将aPos传递过去。
 * - `TexCoord`: 输出的纹理坐标，传递给片段着色器。
 */
const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos, 1.0);\n"
    "   TexCoord = aTexCoord;\n"
    "}";

/**
 * @brief 片段着色器源码 (GLSL)
 * @details
 * 负责将YUV纹理颜色转换为最终在屏幕上显示的RGB颜色。
 * - `TexCoord`: 从顶点着色器传入的纹理坐标。
 * - `textureY`, `textureU`, `textureV`: 分别代表Y、U、V三个分量的纹理采样器。
 * - `FragColor`: 输出的最终像素颜色 (RGBA)。
 * 该着色器首先从三个纹理中采样对应的Y, U, V值，然后使用标准的转换矩阵
 * 将YUV颜色空间转换为RGB颜色空间。
 */
const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D textureY;\n"
    "uniform sampler2D textureU;\n"
    "uniform sampler2D textureV;\n"
    "void main()\n"
    "{\n"
    "   float y = texture(textureY, TexCoord).r;\n"
    "   float u = texture(textureU, TexCoord).r - 0.5;\n"
    "   float v = texture(textureV, TexCoord).r - 0.5;\n"
    "   float r = y + 1.402 * v;\n"
    "   float g = y - 0.344 * u - 0.714 * v;\n"
    "   float b = y + 1.772 * u;\n"
    "   FragColor = vec4(r, g, b, 1.0);\n"
    "}";

/**
 * @brief 构造函数
 * @param parent 父窗口指针
 * @param f 窗口标志
 */
OpenGLRender::OpenGLRender(QWidget* parent, Qt::WindowFlags f)
    : QOpenGLWidget(parent, f)
{
    m_pos = QPoint(0,0);
    m_zoomSize = QSize(0,0);
    m_rect.setRect(0,0,0,0);
    this->setMouseTracking(true);
    label_ = new QLabel(this);
    label_->resize(parent->size());
    this->resize(parent->size());
    this->setMinimumSize(400,250);
}

/**
 * @brief 析构函数
 * @details
 * 确保在对象销毁时释放所有已分配的OpenGL资源，防止内存泄漏。
 * makeCurrent() 和 doneCurrent() 用于确保在正确的上下文中执行清理操作。
 */
OpenGLRender::~OpenGLRender()
{
    //释放opengl上下文
    if(!isValid())
    {
        return;
    }
    //获取上下文
    this->makeCurrent();
    //释放纹理
    freeTexYUV420P();
    //释放上下文
    this->doneCurrent();
    //释放VBO,EBO,VAO
    glDeleteBuffers(1,&VBO);
    glDeleteBuffers(1,&EBO);
    glDeleteVertexArrays(1,&VAO);

}

/**
 * @brief 初始化OpenGL资源
 * @details
 * 此函数在OpenGL上下文创建后首次调用。它负责：
 * 1. 初始化OpenGL函数指针 (initializeOpenGLFunctions)。
 * 2. 编译和链接顶点、片段着色器，创建着色器程序。
 * 3. 创建顶点缓冲对象(VBO)和顶点数组对象(VAO)，并上传顶点数据（坐标和纹理坐标）。
 * 4. 创建Y、U、V三个独立的纹理对象，并设置纹理参数（环绕、过滤方式）。
 * 5. 设置清屏颜色。
 */
void OpenGLRender::initializeGL()
{
    //初始化opengl
    initializeOpenGLFunctions();

    //加载脚本 顶点着色器，片段着色器
    program_ = new QOpenGLShaderProgram();
    program_->addShaderFromSourceFile(QOpenGLShader::Vertex,":/UI/brown/vertex.vsh");// 顶点着色器
    program_->addShaderFromSourceFile(QOpenGLShader::Fragment,":/UI/brown/fragment.fsh");// 片段着色器
    //链接
    program_->link();

    //绑定YUV变量值
    program_->bind();
    program_->setUniformValue("tex_y",0);
    program_->setUniformValue("tex_u",1);
    program_->setUniformValue("tex_v",2);//后面数值就是索引，通过这个索引来去赋值

    //赋值这个坐标和纹理
    GLuint posAttr = GLuint(program_->attributeLocation("aPos"));
    GLuint texCord = GLuint(program_->attributeLocation("aTexCord"));

    //创建VAO
    glGenVertexArrays(1,&VAO);// VAO是顶点数组对象，存储顶点属性的状态
    //绑定VAO
    glBindVertexArray(VAO);

    //创建VBO
    glGenBuffers(1,&VBO);// VBO是顶点缓冲对象，存储顶点数据
    glBindBuffer(GL_ARRAY_BUFFER,VBO);

    //创建EBO
    glGenBuffers(1,&EBO);// EBO是元素缓冲对象，存储索引数据
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);

    //创建一个新的数据存储
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vertices),
                 vertices,
                 GL_STATIC_DRAW); //准备数组

    //将顶点索引传入EBO缓存
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(indices),indices,GL_STATIC_DRAW);
    //数值顶点坐标
    glVertexAttribPointer(posAttr,
                           3,
                           GL_FLOAT,
                           GL_FALSE,
                           5 * sizeof(GLfloat),
                           nullptr);
    //启用这个顶点数组
    glEnableVertexAttribArray(posAttr);

    //数组纹理坐标
    glVertexAttribPointer(texCord,2,GL_FLOAT,GL_FALSE,5 * sizeof(GLfloat),reinterpret_cast<const GLvoid*>(3 * sizeof(GLfloat)));

    //启用这个纹理
    glEnableVertexAttribArray(texCord);

    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindVertexArray(0);
    //清空这个窗口颜色
    glClearColor(0.0f,0.0f,0.0f,1.0f);
}

/**
 * @brief 渲染一帧画面
 * @details
 * Qt的绘图事件循环会自动调用此函数。它负责实际的绘制操作：
 * 1. 清除颜色和深度缓冲区。
 * 2. 绑定着色器程序。
 * 3. 检查是否有新的视频帧数据 (m_frame)。
 * 4. 如果有新帧，则激活并绑定Y、U、V纹理，使用glTexSubImage2D更新纹理内容。
 *    这种方式比每次都创建新纹理更高效。
 * 5. 将着色器中的uniform变量（textureY, textureU, textureV）与对应的纹理单元关联起来。
 * 6. 绑定VAO，调用glDrawArrays执行绘制，将矩形渲染到屏幕上。
 */
void OpenGLRender::paintGL()
{
    //重绘制之前清空上一次颜色
    glClear(GL_COLOR_BUFFER_BIT);

    //更新视图
    glViewport(m_pos.x(),m_pos.y(),m_zoomSize.width(),m_zoomSize.height());
    //绑定着色器，开始渲染
    program_->bind();

    //绑定纹理
    if(texY_ && texU_ && texV_)
    {
        texY_->bind(0);
        texU_->bind(1);
        texV_->bind(2);
    }

    //绑定VAO
    glBindVertexArray(VAO);

    //绘制
    glDrawElements(GL_TRIANGLES,
                   6,
                   GL_UNSIGNED_INT,
                   nullptr);
    glBindVertexArray(0);

    //释放纹理
    if(texY_ && texU_ && texV_)
    {
        texY_->release();
        texU_->release();
        texV_->release();
    }
    //释放这个着色器程序
    program_->release();
}

/**
 * @brief 窗口尺寸变化事件
 * @param width 新的窗口宽度
 * @param height 新的窗口高度
 * @details
 * 当窗口大小改变时，此函数被调用。它负责：
 * 1. 计算视频帧的宽高比和窗口的宽高比。
 * 2. 比较两者宽高比，确定是基于宽度还是高度进行缩放，以保持视频原始比例不拉伸。
 * 3. 计算出新的视口(Viewport)的位置和大小，使其在窗口中居中显示。
 * 4. 调用 glViewport 设置新的视口区域。
 */
void OpenGLRender::resizeGL(int width, int height)
{
    //缩放实现
    if(m_size.width() < 0 || m_size.height() < 0)
    {
        return;
    }

    //计算显示图片窗口大小，来实现长宽等比缩放
    if((double(w) / h) < (double(m_size.width()) / m_size.height()))
    {
        //更新大小
        m_zoomSize.setWidth(w);
        m_zoomSize.setHeight(((double(w) / m_size.width()) * m_size.height()));//来去缩放大小
    }
    else
    {
        m_zoomSize.setHeight(h);
        m_zoomSize.setWidth(((double(h) / m_size.height()) * m_size.width()));
    }
    //更新位置pos
    m_pos.setX(double(w - m_zoomSize.width()) / 2);
    m_pos.setY(double(h - m_zoomSize.height()) / 2);
    m_rect.setRect(m_pos.x(),m_pos.y(),m_zoomSize.width(),m_zoomSize.height());
    //更新这个宽高
    if(label_)
    {
        label_->resize(w,h);
    }
    this->update(QRect(0,0,w,h));
}

/**
 * @brief 接收新的视频帧并触发重绘
 * @param frame 包含YUV数据的AVFrame智能指针
 * @details
 * 这是从外部（如解码线程）喂给渲染器的接口。
 * 它将传入的AVFrame保存到成员变量m_frame中，然后调用update()来调度一次paintGL()的执行。
 */
void OpenGLRender::Repaint(AVFramePtr frame)
{
    //重绘视频数据
    if(!frame || frame->width == 0 || frame->height == 0)
    {
        return;
    }
    //开始绘制的时候去结束
    if(label_)
    {
        label_->hide();
        delete label_;
        label_ = nullptr;
    }
    //更新yuv纹理
    repaintTexYUV420P(frame);
    //调用这个paintGL()来去绘制
    this->update();//调用这个update()会自动调用这个paintGL
}

/**
 * @brief 鼠标移动事件处理
 * @param ev 鼠标事件对象
 * @details
 * 记录下当前鼠标在窗口内的绝对坐标。
 */
void OpenGLRender::mouseMoveEvent(QMouseEvent* ev)
{
    m_mousePos = ev->pos();
}

/**
 * @brief 计算鼠标在视频画面上的相对位置
 * @param[out] body 用于存储结果的MouseMove_Body结构体
 * @details
 * 将记录的绝对鼠标坐标转换为相对于视频渲染区域的百分比坐标。
 * 1. 从m_viewportRect获取视频渲染区域的几何信息。
 * 2. 检查鼠标坐标是否在渲染区域内。
 * 3. 如果在，则计算出鼠标位置相对于渲染区域左上角的偏移量。
 * 4. 将偏移量除以渲染区域的宽高，得到0.0到1.0之间的比例值。
 * 5. 将比例值存入body结构体。
 */
void OpenGLRender::GetPosRation(MouseMove_Body& body)
{
    //需要获取当前鼠标位置
    QPoint localMousePos = this->mapFromGlobal(QCursor::pos()) - m_rect.topLeft();
    //获取x,y相对于这个宽高的比值
    double x = (static_cast<double>(localMousePos.x()) / static_cast<double>(m_rect.width())) * 100;
    double y = (static_cast<double>(localMousePos.y()) / static_cast<double>(m_rect.height())) * 100;

    //设置这个结构体的x,y
    body.xl_ratio = static_cast<uint8_t>(x);
    body.xr_ratio = static_cast<uint8_t>((x - static_cast<double>(body.xl_ratio)) * 100);
    body.yl_ratio = static_cast<uint8_t>(y);
    body.yr_ratio = static_cast<uint8_t>((y - static_cast<double>(body.yl_ratio)) * 100);
}

/**
 * @brief 窗口显示事件处理
 * @param event 事件对象
 * @details
 * 在窗口第一次显示时，创建一个QLabel来显示加载动画(GIF)。
 * 当第一帧视频数据到达并渲染后，这个QLabel会被隐藏。
 */
void OpenGLRender::showEvent(QShowEvent* event)
{
    QMovie* move = new QMovie(":/UI/brown/center/loading.gif");
    if(label_)
    {
        label_->setMovie(move);
        move->start();
        label_->show();
    }
}

/**
 * @brief 更新YUV纹理数据
 * @param frame 新的视频帧数据
 * @details
 * 1. 检查传入的帧数据是否有效。
 * 2. 如果当前帧的大小与上一个帧不同，释放旧的纹理资源，重新初始化纹理。
 * 3. 设置YUV纹理的图像高度和行长度。
 * 4. 分别为Y、U、V三个分量设置纹理数据。
 */
void OpenGLRender::repaintTexYUV420P(AVFramePtr frame)
{
    //更新yuv420p纹理
    if(frame->width != m_size.width() || frame->height != m_size.height())
    {
        //释放yuv420p,重新初始化这个yuv
        freeTexYUV420P();
    }
    //重新初始化
    initTexYUV420P(frame);

    //传值
    options_.setImageHeight(frame->height);
    options_.setRowLength(frame->linesize[0]);
    //设置图片数据
    texY_->setData(QOpenGLTexture::Red,QOpenGLTexture::UInt8,static_cast<const void*>(frame->data[0]),&options_);
    options_.setRowLength(frame->linesize[1]);
    texU_->setData(QOpenGLTexture::Red,QOpenGLTexture::UInt8,static_cast<const void*>(frame->data[1]),&options_);
    options_.setRowLength(frame->linesize[2]);
    texV_->setData(QOpenGLTexture::Red,QOpenGLTexture::UInt8,static_cast<const void*>(frame->data[2]),&options_);
}

/**
 * @brief 初始化YUV纹理
 * @param frame 视频帧数据
 * @details
 * 1. 如果Y纹理尚未创建，则创建Y纹理并设置其大小、过滤方式和存储。
 * 2. 如果U纹理尚未创建，则创建U纹理并设置其大小、过滤方式和存储。
 * 3. 如果V纹理尚未创建，则创建V纹理并设置其大小、过滤方式和存储。
 * 4. 更新成员变量m_size，记录当前纹理的宽高。
 * 5. 调用resizeGL()更新OpenGL视口和窗口大小。
 */
void OpenGLRender::initTexYUV420P(AVFramePtr frame)
{
    //初始化420p
    //初始化yuv纹理
    if(!texY_)
    {
        texY_ = new QOpenGLTexture(QOpenGLTexture::Target2D);
        //设置大小
        texY_->setSize(frame->width,frame->height);
        //纹理属性
        texY_->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear,QOpenGLTexture::Linear);
        texY_->setFormat(QOpenGLTexture::R8_UNorm);
        texY_->allocateStorage();
        //更新宽高
        m_size.setWidth(frame->width);
        m_size.setHeight(frame->height);
        //重置宽高
        this->resizeGL(this->width(),this->height());
    }
    if(!texU_)
    {
        texU_ = new QOpenGLTexture(QOpenGLTexture::Target2D);
        //设置大小
        texU_->setSize(frame->width / 2,frame->height / 2);
        //纹理属性
        texU_->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear,QOpenGLTexture::Linear);
        texU_->setFormat(QOpenGLTexture::R8_UNorm);
        texU_->allocateStorage();
    }
    if(!texV_)
    {
        texV_ = new QOpenGLTexture(QOpenGLTexture::Target2D);
        //设置大小
        texV_->setSize(frame->width / 2,frame->height / 2);
        //纹理属性
        texV_->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear,QOpenGLTexture::Linear);
        texV_->setFormat(QOpenGLTexture::R8_UNorm);
        texV_->allocateStorage();
    }
}

/**
 * @brief 释放YUV纹理资源
 * @details
 * 遍历并销毁YUV纹理相关的OpenGL对象，包括：
 * - 纹理对象
 * - 相关的QOpenGLTexture实例
 * 通过将指针置为nullptr，避免悬挂指针和内存泄漏。
 */
void OpenGLRender::freeTexYUV420P()
{
    //释放资源
    if(texY_)
    {
        texY_->destroy();
        delete texY_;
        texY_ = nullptr;
    }
    if(texU_)
    {
        texU_->destroy();
        delete texU_;
        texU_ = nullptr;
    }
    if(texV_)
    {
        texV_->destroy();
        delete texV_;
        texV_ = nullptr;
    }
}
