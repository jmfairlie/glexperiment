#include "mygldrawer.h"
#include "helper.hpp"
#include <iostream>
#include<QFile>
#include <assert.h>

MyGLDrawer::MyGLDrawer(const QGLFormat &format): QGLWidget(format),
                                         m_pos_attr_loc(0),
                                         m_tex_attr_loc(1)
{}

MyGLDrawer::MyGLDrawer(QWidget* parent): QGLWidget(parent),
                                         m_pos_attr_loc(0),
                                         m_tex_attr_loc(1)
{}

MyGLDrawer::~MyGLDrawer()
{
  makeCurrent();
  glDeleteProgram(m_program);
  glDeleteShader(m_frag_shader);
  glDeleteShader(m_vert_shader);
  glDeleteTextures(1,&m_tex_name);
  destroyArrayStuff();
  DEBUGMSG("Destroy!");
}

void MyGLDrawer::initializeGL()
{

  m_image = QGLWidget::convertToGLFormat(QImage(":/copper.jpg"));

  DEBUGMSG("exists %d image  %dx%d\n",QFile::exists(QString(":/copper.jpg")),
                                      m_image.width(),
                                      m_image.height());
  // Set up the rendering context, define display lists etc.:
  glClearColor(0.6f, 0.8f, 1.0f, 1.0f);//pvr blue!
  glEnable(GL_DEPTH_TEST);
  bool ok = 0;

#ifndef NDEBUG
  DEBUGMSG("GL logging started: %d",setupGLDebug());
#endif
  ok = setupShaders();
  assert(ok);

  ok = setUpProgram();
  assert(ok);

  ok = setUpAttributes();
  assert(ok);

  ok = setUpTextures();
  assert(ok);

  ok = setUpUniforms();
  assert(ok);
  (void)ok;
}

int MyGLDrawer::setupGLDebug()
{
  QOpenGLContext *ctx = QOpenGLContext::currentContext();
  DEBUGMSG("Loading context version %d.%d",
           ctx->surface()->format().majorVersion(),
           ctx->surface()->format().minorVersion());
  bool khr = ctx->hasExtension(QByteArrayLiteral("GL_KHR_debug"));
  DEBUGMSG("GL_KHR_debug extension availability: %d", khr);

  if(khr)
  {

    m_logger = new QOpenGLDebugLogger(this);
    if (!m_logger->initialize())
      return 0;
    m_logger->enableMessages();
    connect(m_logger,
            SIGNAL(messageLogged(QOpenGLDebugMessage)),
            this,
            SLOT(handleLoggedMessage(QOpenGLDebugMessage)));
    m_logger->startLogging();
    return 1;
  }

  return 0;
}

void MyGLDrawer::resizeGL(int w, int h)
{
  glViewport(0,0,w,h);
}

void MyGLDrawer::paintGL()
{
  // draw the scene:
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, 0);
}

bool MyGLDrawer::event(QEvent * ev)
{
  if(ev->type() == QEvent::KeyPress)
  {
    QKeyEvent *qe((QKeyEvent*)ev);
    switch(qe->key())
    {
    case Qt::Key_Escape:
    case Qt::Key_Q:
      QWidget::close();
      break;
    }
  }
  return QGLWidget::event(ev);
}

int MyGLDrawer::setUpAttributes()
{
  glGenVertexArrays(1,&m_vao);
  glBindVertexArray(m_vao);

  GLubyte indices[] = {0, 1, 2};

  GLfloat attrData[] = {
                         -1.0f, -1.0f, 0.0f,
                         0.0f, 0.0f,
                          0.0f,  1.0f, 0.0f,
                         0.5f, 1.0f,
                          1.0f, -1.0f, 0.0f,
                         1.0f, 1.0f
                         };

  glGenBuffers(1, &m_vert_buffer);

  glBindBuffer(GL_ARRAY_BUFFER, m_vert_buffer);

  unsigned int uiSize = 3 * (sizeof(GLfloat) * 5);
  glBufferData(GL_ARRAY_BUFFER, uiSize, attrData, GL_STATIC_DRAW);

  glVertexAttribPointer(m_pos_attr_loc,
                        3,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(GLfloat) * 5,
                        0);

  glVertexAttribPointer(m_tex_attr_loc,
                        2,
                        GL_FLOAT,
                        GL_FALSE,
                        sizeof(GLfloat) * 5,
                        reinterpret_cast<void*>(sizeof(GLfloat) * 3));

  glEnableVertexAttribArray(m_pos_attr_loc);
  glEnableVertexAttribArray(m_tex_attr_loc);

  glGenBuffers(1, &m_indx_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indx_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               3*sizeof(GLubyte),
               indices,
               GL_STATIC_DRAW);
  return 1;
}

int MyGLDrawer::setUpUniforms()
{
  m_tex_uni_loc=glGetUniformLocation(m_program, "sTex");
  glUniform1i(m_tex_uni_loc, 0);
  return 1;
}

int MyGLDrawer::setUpTextures()
{
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &m_tex_name);
  glBindTexture(GL_TEXTURE_2D, m_tex_name);

  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);


  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               m_image.width(),
               m_image.height(),
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               m_image.bits());
  return 1;
}

int MyGLDrawer::setupShaders()
{
  m_frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  QString fragpath(":/default.frag");
  m_vert_shader = glCreateShader(GL_VERTEX_SHADER);
  QString vertpath(":/default.vert");
  return createShader(m_vert_shader, vertpath) &&
         createShader(m_frag_shader, fragpath);
}

int MyGLDrawer::createShader(int shaderid, QString shaderpath)
{
  QFile s(shaderpath);
  s.open(QFile::ReadOnly | QFile::Text);
  QTextStream in(&s);
  QString str = in.readAll();
  GLint shlen = str.length();
  char* shtxt = new char[shlen];
  strcpy(shtxt, str.toLocal8Bit().constData());

  glShaderSource(shaderid, 1, &shtxt, NULL);
  glCompileShader(shaderid);

  delete[] shtxt;
  s.close();
  PRINTSHADERCODE(shaderid);
  return VERIFYSTATUS(Shader, shaderid);
}

int MyGLDrawer::setUpProgram()
{
  m_program = glCreateProgram();
  glAttachShader(m_program, m_frag_shader);
  glAttachShader(m_program, m_vert_shader);

  glBindAttribLocation(m_program, m_pos_attr_loc, "aVertex");
  glBindAttribLocation(m_program, m_tex_attr_loc, "aTexCoords");

  glLinkProgram(m_program);
  bool ok = VERIFYSTATUS(Program,  m_program);
  if (!ok)
    return 0;

  glUseProgram(m_program);
  return 1;
}

void MyGLDrawer::handleLoggedMessage(QOpenGLDebugMessage msg)
{
  QString error;
  // Format based on severity
  switch (msg.severity())
  {
  case QOpenGLDebugMessage::NotificationSeverity:
    error += "--";
    break;
  case QOpenGLDebugMessage::HighSeverity:
    error += "!!";
    break;
  case QOpenGLDebugMessage::MediumSeverity:
    error += "!~";
    break;
  case QOpenGLDebugMessage::LowSeverity:
    error += "~~";
    break;
  default:
  break;
  }

  error += " (";

  // Format based on source
#define CASE(c) case QOpenGLDebugMessage::c: error += #c; break
  switch (msg.source())
  {
    CASE(APISource);
    CASE(WindowSystemSource);
    CASE(ShaderCompilerSource);
    CASE(ThirdPartySource);
    CASE(ApplicationSource);
    CASE(OtherSource);
    CASE(InvalidSource);
    default:
    break;
  }
#undef CASE

  error += " : ";

  // Format based on type
#define CASE(c) case QOpenGLDebugMessage::c: error += #c; break
  switch (msg.type())
  {
    CASE(ErrorType);
    CASE(DeprecatedBehaviorType);
    CASE(UndefinedBehaviorType);
    CASE(PortabilityType);
    CASE(PerformanceType);
    CASE(OtherType);
    CASE(MarkerType);
    CASE(GroupPushType);
    CASE(GroupPopType);
    default:
    break;
  }
#undef CASE

  error += ")";
  DEBUGMSG("Logger: %s: %s", qPrintable(error), qPrintable(msg.message()));
  (void)msg;
}

void MyGLDrawer::destroyArrayStuff()
{
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glDeleteBuffers(1, &m_vert_buffer);
  glDeleteBuffers(1, &m_indx_buffer);

  glBindVertexArray(0);
  glDeleteVertexArrays(1, &m_vao);
}
