#include "screenwidget.hxx"
#include <iostream>
#include <log.hxx>
#include <QFile>
#include <QSurfaceFormat>

// clang-format off

static constexpr float vertices_uvs[] =
{
  -1,  1, 0, 1,
   1,  1, 1, 1,
   1, -1, 1, 0,
   1, -1, 1, 0,
  -1, -1, 0, 0,
  -1,  1, 0, 1,
};

// clang-format on

ScreenWidget::ScreenWidget(QWidget* parent)
    : QOpenGLWidget(parent), vbo_(QOpenGLBuffer::Type::VertexBuffer)
{
}

ScreenWidget::~ScreenWidget()
{
    if (initialized_)
    {
        glDeleteTextures(1, &texture_);
    }
    delete program_;
}

void ScreenWidget::InitializeTexture(int width, int height, int bitdepth, void* tdata)
{
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, bitdepth, tdata);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    initialized_ = true;
}

void ScreenWidget::Redraw(int width, int height, int bitdepth, void* tdata)
{
    if (initialized_) [[likely]]
    {
        if (bitdepth == GL_UNSIGNED_SHORT_5_5_5_1)
        { // n64 (TODO: make a bool in this function that sets this)
            glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
        }
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, bitdepth, tdata);
        glBindTexture(GL_TEXTURE_2D, 0);
        if (bitdepth == GL_UNSIGNED_SHORT_5_5_5_1)
        {
            glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
        }
    }
}

void ScreenWidget::ResetProgram(QString* vertex, QString* fragment)
{
    if (!vertex)
    {
        QFile vfile(":/shaders/simple.vs");
        vfile.open(QIODevice::ReadOnly);
        vshader_source_ = vfile.readAll();
    }
    else
    {
        vshader_source_ = *vertex;
    }
    if (!fragment)
    {
        QFile ffile(":/shaders/simple.fs");
        ffile.open(QIODevice::ReadOnly);
        fshader_source_ = ffile.readAll();
    }
    else
    {
        fshader_source_ = *fragment;
    }
    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceCode(vshader_source_);
    if (!vshader->log().isEmpty())
    {
        auto error = vshader->log().toStdString();
        delete vshader;
        throw std::runtime_error(error);
    }
    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fshader->compileSourceCode(fshader_source_);
    if (!fshader->log().isEmpty())
    {
        auto error = fshader->log().toStdString();
        delete fshader;
        throw std::runtime_error(error);
    }
    if (program_)
    {
        delete program_;
    }
    program_ = new QOpenGLShaderProgram;
    program_->addShader(vshader);
    program_->addShader(fshader);
    program_->link();
    program_->bind();
    glActiveTexture(GL_TEXTURE0);
    GLint tex = glGetUniformLocation(program_->programId(), "tex");
    if (tex == -1)
    {
        Logger::Fatal("Could not find uniform tex");
    }
    glUniform1i(tex, 0);
    delete fshader;
    delete vshader;
}

void ScreenWidget::mouseMoveEvent(QMouseEvent* event)
{
    mouse_move_callback_(event);
}

void ScreenWidget::initializeGL()
{
    initializeOpenGLFunctions();
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    Logger::Info("OpenGL version: {}", version);
    vao_.create();
    vao_.bind();
    vbo_.create();
    glBindBuffer(GL_ARRAY_BUFFER, vbo_.bufferId());
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_uvs), vertices_uvs, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture_);
    glClearColor(0.1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    ResetProgram();
    hide();
}

void ScreenWidget::resizeGL(int, int) {}

void ScreenWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    program_->bind();
    if (initialized_)
    {
        glBindTexture(GL_TEXTURE_2D, texture_);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    program_->release();
}