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

OpenGLRender::OpenGLRender(QWidget *parent, Qt::WindowFlags f)
    :QOpenGLWidget(parent,f)
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

void OpenGLRender::showEvent(QShowEvent *event)
{
    QMovie* move = new QMovie(":/UI/brown/center/loading.gif");
    if(label_)
    {
        label_->setMovie(move);
        move->start();
        label_->show();
    }
}

void OpenGLRender::initializeGL()
{
    //初始化opengl
    initializeOpenGLFunctions();

    //加载脚本 顶点着色器，片段着色器
    program_ = new QOpenGLShaderProgram();
    program_->addShaderFromSourceFile(QOpenGLShader::Vertex,":/UI/brown/vertex.vsh");
    program_->addShaderFromSourceFile(QOpenGLShader::Fragment,":/UI/brown/fragment.fsh");
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
    glGenVertexArrays(1,&VAO);
    //绑定VAO
    glBindVertexArray(VAO);

    //创建VBO
    glGenBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);

    //创建EBO
    glGenBuffers(1,&EBO);
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

void OpenGLRender::resizeGL(int w, int h)
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
