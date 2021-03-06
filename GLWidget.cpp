#include "GLWidget.h"

//#define LOOKINGGLASS
#define HP_LOAD_LIBRARY

#ifdef LOOKINGGLASS
#include <holoplay.h>
#endif

GLWidget::GLWidget(QWidget *parent):
    QOpenGLWidget(parent)
{
    //this timer will ensure that painGL() is called at 60fps
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(10);
}

//called once on init
void GLWidget::initializeGL()
{
    //grab window details from Qt
    WIDTH = this->width();
    HEIGHT = this->height();


    //get OpenGL Functions, init them and bind them to varible f
    QOpenGLFunctions_3_2_Core *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_2_Core>();
    if (!f) {
        QMessageBox messageBox;
        messageBox.critical(nullptr, "Error", "This machine does not seem to support Modern OpenGL (>=3.2)");
        messageBox.setFixedSize(500, 200);
    }

    f->initializeOpenGLFunctions();

    //if looking glass mode is on, setup the holoplay library
#ifdef LOOKINGGLASS
    hp_loadLibrary();
    hp_initialize();
    hp_setupQuiltSettings(0);
    hp_setupQuiltTexture();
    hp_setupBuffers();
    WIDTH = 2560;
    HEIGHT = 1600;
#endif

    //////////////////////////////////////////////////////
    /// OpenGL Settings init, including framebuffer setup
    //////////////////////////////////////////////////////
    f->glEnable(GL_BLEND);
    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LESS);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    f->glGenFramebuffers(1, &screenFramebuffer);
    f->glBindFramebuffer(GL_FRAMEBUFFER, screenFramebuffer);

    // Init The texture we're going to render to
    f->glGenTextures(1, &renderedTexture);
    f->glBindTexture(GL_TEXTURE_2D, renderedTexture);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // The depth buffer
    f->glGenRenderbuffers(1, &depthrenderbuffer);
    f->glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    f->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WIDTH, HEIGHT);
    f->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

    // Set "renderedTexture" as our colour attachement #0
    f->glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);

    // Set the list of draw buffers.
    GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    f->glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

    //set background color, and init our two brain objects, give them a refrence to this object
    f->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    primaryBrain = Brain(f, primaryNodeName, primaryEdgeName);
    secondaryBrain = Brain(f, secondaryNodeName, secondaryEdgeName);

    primaryBrain.screen = this;
    secondaryBrain.screen = this;


    ////////////////////////////////////////////////////////
    ///                 CAMERA INIT CODE               //////
    /// Different for looking glass and desktop version//////
    /////////////////////////////////////////////////////////
#ifdef LOOKINGGLASS
    //the full screen quad makes post-proccessing possible, is currently bugged
    //to understand how this section of code works refer to the link below
    // https://docs.lookingglassfactory.com/HoloPlayCAPI/guides/camera/
    fullscreenquad.init(f);
    glm::quat brainRot = glm::quat(glm::vec3(-1.5708f, 1.5708f * 2, 0.0f));
    primaryBrain.position = primaryBrain.position * glm::mat4_cast(brainRot);
    //primaryBrain.position = glm::scale(primaryBrain.position, glm::vec3(0.1, 0.1, 0.1));
    primaryBrain.updatePosition();


    int totalViews = 32;
    float cameraSize = 80.0f;
    float cameraDistance = -cameraSize / std::tan(glm::radians(14.0f) / 2.0f);
    float fov = glm::radians(14.0f); // field of view
    float aspectRatio = (float)WIDTH / (float)HEIGHT;
    float viewCone = glm::radians(40.0f); // 40° in radians

    //camera center
    glm::vec3 focalPosition = glm::vec3(1.52f, -15.28f, 40.23f);

    //init all cameras needed
    for (int currentView = 0; currentView < totalViews; currentView++)
    {
        holoCam[currentView].view = glm::mat4(1.0f);
        holoCam[currentView].proj = glm::mat4(1.0f);
        holoCam[currentView].view = glm::translate(holoCam[currentView].view, focalPosition);
        float offsetAngle = (currentView / (totalViews - 1.0f) - 0.5f) * viewCone;// start at -viewCone * 0.5 and go up to viewCone * 0.5
        // calculate the offset that the camera should move
        float offset = cameraDistance * std::tan(offsetAngle);

        // modify the view matrix (position)
        holoCam[currentView].view = glm::translate(holoCam[currentView].view, glm::vec3(offset, 0.0f, cameraDistance));

        //The standard model Looking Glass screen is roughly 4.75" vertically. If we assume the average viewing distance for a user sitting at their desk is about 36", our field of view should be about 14°. There is no correct answer, as it all depends on your expected user's distance from the Looking Glass, but we've found the most success using this figure.
                                          // fov, aspect ratio, near, far
        holoCam[currentView].proj = glm::perspective(fov, aspectRatio, 20.0f, 700.0f);
        // modify the projection matrix, relative to the camera size and aspect ratio
        holoCam[currentView].proj[2][0] += offset / (cameraSize * aspectRatio);
        holoCam[currentView].position = glm::vec3(holoCam[currentView].view[3]);
    }
#endif

    //make a camera for each of the possible views, set up their position and angle
    cam = Camera(WIDTH, HEIGHT);;
    top = Camera(WIDTH / 2, HEIGHT / 2);
    side = Camera(WIDTH / 2, HEIGHT / 2);
    front = Camera(WIDTH / 2, HEIGHT / 2);

    top.position = glm::vec3(0.57f, 6.21f, 228.85f);
    top.altPosition = glm::vec3(1.57f, -40.93f, -228.85f);
    top.view = glm::lookAt(
        top.position, // position
        glm::vec3(1.52f, -33.28f, -80.23f), // camera center
        glm::vec3(0.0f, 0.0f, 1.0f) // up axis
    );
    front.position = glm::vec3(0.41f, 156.71f, 19.51f);
    front.altPosition = glm::vec3(0.41f, -220.92f, 19.51f);
    front.view = glm::lookAt(
        front.position, // position
        glm::vec3(1.52f, -33.28f, 6.23f), // camera center
        glm::vec3(0.0f, 0.0f, -1.0f) // up axis
    );
    side.position = glm::vec3(183.95f, -36.63f, -29.99f);
    side.altPosition = glm::vec3(-183.733f, -36.63f, -29.99f);
    side.view = glm::lookAt(
        side.position, // position
        glm::vec3(1.52f, -33.28f, 6.23f), // camera center
        glm::vec3(0.0f, 0.0f, -1.0f) // up axis
    );

    cam.position = front.position;

    //start the timer
    frameTimer.start();
}

//called on window resize
void GLWidget::resizeGL(int w, int h)
{

    //get back the found OpenGL functions and get the new window details (for looking glass the window will not change)
    QOpenGLFunctions_3_2_Core *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_2_Core>();
    WIDTH = w;
    HEIGHT = h;

#ifdef LOOKINGGLASS
    WIDTH = 2560;
    HEIGHT = 1600;
#endif
    //bind to our framebuffer and change the texture size
    f->glBindTexture(GL_TEXTURE_2D, renderedTexture);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    //do the same with the depth buffer
    f->glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    f->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WIDTH, HEIGHT);

    //reset cameras aspect ratios
    cam.Resize(WIDTH, HEIGHT);;
    top.Resize(WIDTH / 2, HEIGHT / 2);
    side.Resize(WIDTH / 2, HEIGHT / 2);
    front.Resize(WIDTH / 2, HEIGHT / 2);
}

//called every frame
void GLWidget::paintGL()
{
    //regrab openGL functions
    QOpenGLFunctions_3_2_Core *f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_2_Core>();

    //get mouse coords and the change in the coords
    float xpos = this->mapFromGlobal(QCursor::pos()).x();
    float ypos = this->mapFromGlobal(QCursor::pos()).y();
    float deltaX = lastXPos - xpos;
    float deltaY = lastYPos - ypos;
    lastXPos = xpos;
    lastYPos = ypos;

    //ensure timer is still going, and get deltaTime (this makes it so changes in framerate won't
    //affect how fast things move in the viewer
    float deltaTime = frameTimer.elapsed() / 1000.0f;
    frameTimer.restart();

    //set the node text in the sidebar
    if (nodeName != nullptr)
    {
        nodeName->setText(primaryBrain.nodeNames[selectedNode].c_str());
    }

    //make sure GL settings haven't changed
    f->glEnable(GL_BLEND);
    f->glEnable(GL_DEPTH_TEST);
    f->glDepthFunc(GL_LESS);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //////////////////////////////////////////////////////////////
    /// UPDATE BRAIN VARIBLES TO MATCH THE ONES IN THE SETTINGS///
    /////////////////////////////////////////////////////////////
    primaryBrain.colors = colors;
    secondaryBrain.colors = colors;

    primaryBrain.threshold = *threshold;
    secondaryBrain.threshold = *threshold;

    primaryBrain.textThreshold = *textThreshold;
    secondaryBrain.textThreshold = *textThreshold;

    primaryBrain.nodeSize = *nodeSize;
    secondaryBrain.nodeSize = *nodeSize;

    primaryBrain.connectionSize = *connectionSize;
    secondaryBrain.connectionSize = *connectionSize;

    primaryBrain.graphSignalSize = *graphSignalSize;
    secondaryBrain.graphSignalSize = *graphSignalSize;

    primaryBrain.displayHeatMap = *displayHeatmap;
    secondaryBrain.displayHeatMap = *displayHeatmap;

    primaryBrain.connectionStrengthColor = *connectionStrengthColor;
    secondaryBrain.connectionStrengthColor = *connectionStrengthColor;

    primaryBrain.mri.axialTrans = *axial;
    primaryBrain.mri.coronalTrans = *coronal;
    secondaryBrain.mri.axialTrans = *axial;
    secondaryBrain.mri.coronalTrans = *coronal;

    primaryBrain.isScaling = *isScaling;
    secondaryBrain.isScaling = *isScaling;

    primaryBrain.isConnectionScaling = *isConnectionScaling;
    secondaryBrain.isConnectionScaling = *isConnectionScaling;

    primaryBrain.displayUnusedNodes = *showUnusedNodes;
    secondaryBrain.displayUnusedNodes = *showUnusedNodes;

    primaryBrain.milisecondsPerFrame = msPerFrame;
    secondaryBrain.milisecondsPerFrame = msPerFrame;

    if (primaryShouldReload == 1)
    {
        primaryBrain.reloadBrain(primaryNodeName, primaryEdgeName);
        primaryBrain.mesh = BrainModel();
        primaryBrain.mesh.brain = &primaryBrain;
        if(primaryMeshName.substr(primaryMeshName.find_last_of(".") + 1) == "nv")
            primaryBrain.mesh.loadFromNV(f, primaryMeshName);
        else if(primaryMeshName.substr(primaryMeshName.find_last_of(".") + 1) == "vtk")
            primaryBrain.mesh.loadFromVTK(f, primaryMeshName, "assets/Atlas_Left_Destrieux.txt", "assets/LookUpTable_Destrieux.txt");
        else
            primaryBrain.mesh.loadFromObj(f, primaryMeshName, false);
        primaryShouldReload = 0;
    }
    else if (secondaryShouldReload == 1)
    {
        secondaryBrain.reloadBrain(secondaryNodeName, secondaryEdgeName);
        secondaryBrain.mesh = BrainModel();
        secondaryBrain.mesh.brain = &secondaryBrain;
        if(secondaryMeshName.substr(secondaryMeshName.find_last_of(".") + 1) == "nv")
            secondaryBrain.mesh.loadFromNV(f, secondaryMeshName);
        else if(secondaryMeshName.substr(secondaryMeshName.find_last_of(".") + 1) == "vtk")
            secondaryBrain.mesh.loadFromVTK(f, secondaryMeshName, "assets/Atlas_Left_Destrieux.txt", "assets/LookUpTable_Destrieux.txt");
        else
            secondaryBrain.mesh.loadFromObj(f, primaryMeshName, false);
        secondaryShouldReload = 0;
    }


    ///////////////////////////////////////////////////////////
    /// UPDATE CAMERA POSITION AND ANGLE BASED ON USER INPUT//
    /////////////////////////////////////////////////////////
    if (upKeyDown == 1)
    {
        pitch -= turnSpeed * deltaTime;
    }
    if (downKeyDown == 1)
    {
        pitch += turnSpeed * deltaTime;
    }
    if (rightKeyDown == 1)
    {
        yaw -= turnSpeed * deltaTime;
    }
    if (leftKeyDown == 1)
    {
        yaw += turnSpeed * deltaTime;
    }
    if(leftMouseDown == 1)
    {
        yaw += deltaX * deltaTime * mouseSensitivity;
        pitch += deltaY * deltaTime * mouseSensitivity;
    }

    //camera position is on a sphere that centers around the brain model
    cam.position.x = 1.52f + (cos(yaw)  * sin(pitch) * 220);
    cam.position.y = -33.28f + (sin(yaw) * sin(pitch) * 220);
    cam.position.z = 6.23f * (cos(pitch) * 50);

    //if we're in side by side view the aspect ratio changes and the camera has to reflect that
    if (viewingMode == 3)
        cam.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(WIDTH / 2) / HEIGHT, 1.0f, 2000.0f);
    else
        cam.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(WIDTH) / HEIGHT, 1.0f, 2000.0f);

    cam.view = glm::lookAt(
        cam.position,             // position
        glm::vec3(1.52f, -33.28f, 6.23f), // camera center
        glm::vec3(0.0f, 0.0f, -1.0f)                    // up axis
    );

    //prepare framebuffer for render
    f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, screenFramebuffer);
    f->glClearColor(colors[12].R / 255.0f, colors[12].G / 255.0f, colors[12].B / 255.0f, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    f->glEnable(GL_FRAMEBUFFER_SRGB);

#ifndef LOOKINGGLASS
    //////////////////////////////////////////////////////////////////////////////
    /// DEPENDING ON VIEW MODE RENDER THE BRAIN IN THE DIFFERENT CAMERA VIEWS////
    ////////////////////////////////////////////////////////////////////////////
    if (viewingMode == 1)
    {
        f->glViewport(0, 0, WIDTH / 2, HEIGHT / 2);
        primaryBrain.update(f, front, xpos, ypos, selectedNode, rightMouseDown);

        f->glViewport(WIDTH / 2, 0, WIDTH / 2, HEIGHT / 2);
        primaryBrain.update(f, top, xpos, ypos, selectedNode, rightMouseDown);

        f->glViewport(0, HEIGHT / 2, WIDTH / 2, HEIGHT / 2);
        primaryBrain.update(f, side, xpos, ypos, selectedNode, rightMouseDown);

        f->glViewport(WIDTH / 2, HEIGHT / 2, WIDTH / 2, HEIGHT / 2);
        primaryBrain.update(f, cam, xpos, ypos, selectedNode, rightMouseDown);
    }
    else if (viewingMode == 2)
    {
        f->glViewport(0, 0, WIDTH, HEIGHT);
        primaryBrain.update(f, cam, xpos, ypos, selectedNode, rightMouseDown);
    }
    else if (viewingMode == 3)
    {
        f->glViewport(0, 0, WIDTH / 2, HEIGHT);
        primaryBrain.update(f, cam, xpos, ypos, selectedNode, rightMouseDown);

        f->glViewport(WIDTH / 2, 0, WIDTH / 2, HEIGHT);
        secondaryBrain.update(f, cam, xpos, ypos, selectedNode, rightMouseDown);
    }

    //blit the framebuffer to the one bound to the Qt window
    f->glBindFramebuffer(GL_READ_FRAMEBUFFER, screenFramebuffer);
    f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->defaultFramebufferObject());
    f->glBlitFramebuffer(0, HEIGHT, WIDTH, 0,
        0, 0, WIDTH, HEIGHT,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);
    f->glBindFramebuffer(GL_READ_FRAMEBUFFER, this->defaultFramebufferObject());

    //do any QPainter Qt based rendering here (text rendering)
    //texts that should be rendered are put into a list that this reads, renders, and then clears
    //for the next frame
    painter.begin(this);
    painter.setPen(QColor(colors[14].R, colors[14].G, colors[14].B, colors[14].A));
    painter.setFont(QFont("Times", *textSize, QFont::Bold));
    for(NText text: nodeTexts)
    {
        painter.drawText(text.x, text.y, text.str);
    }
    if(primaryBrain.hasTime && *displayFrame)
    {
        painter.drawText(50.0f, HEIGHT - 50.0f, QString("Graph Signal: ") +
                         QString::fromStdString(boost::lexical_cast<std::string>(primaryBrain.currentFrame)));
    }
    nodeTexts.clear();
    painter.end();
#endif

#ifdef LOOKINGGLASS

    glm::quat brainRot = glm::quat(glm::vec3(0.0f, 0.0f, (1.5708f / 5) * deltaTime));
    primaryBrain.position = primaryBrain.position * glm::mat4_cast(brainRot);
    primaryBrain.updatePosition();

    f->glViewport(0, 0, WIDTH, HEIGHT);


    unsigned int totalViews = 32;
    for(unsigned int currentView = 0; currentView < totalViews; currentView++)
    {
        f->glBindFramebuffer(GL_FRAMEBUFFER, screenFramebuffer);
        f->glEnable(GL_DEPTH_TEST);
        f->glDepthFunc(GL_LESS);
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        primaryBrain.update(f, holoCam[currentView], xpos, ypos, selectedNode, rightMouseDown);

        //render to full screen quad to do post processing effects
        f->glBindTexture(GL_TEXTURE_2D, renderedTexture);
        fullscreenquad.render(f);

        f->glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
        f->glDisable(GL_DEPTH_TEST);
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        f->glBindTexture(GL_TEXTURE_2D, renderedTexture);
        //this is a workaround for a bug in holoplay, where it sends the wrong view
        //In order to work around this, I shifted the views until they lined up inside of the looking glass
        //hopefully this issue will be recognized by the holoplay devs soon
        int view2send = currentView - (totalViews / 2) + 8;
        if(view2send < 0)
            view2send = (totalViews - 1) + view2send;
        hp_copyViewToQuilt(view2send);
    }

    f->glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    hp_drawLightfield();
#endif
}

//this function finds which quadrent the cursor is in, and based on that will flip the camera
//to its alternate view
void GLWidget::flipView()
{
    float xpos = this->mapFromGlobal(QCursor::pos()).x();
    float ypos = this->mapFromGlobal(QCursor::pos()).y();
    if (xpos < (WIDTH / 2))
    {
        if (ypos < (HEIGHT / 2))
        {
            front.viewMode *= -1;
            if (front.viewMode == 1)
            {
                front.view = glm::lookAt(
                    front.position, // position
                    glm::vec3(1.52f, -33.28f, 6.23f), // camera center
                    glm::vec3(0.0f, 0.0f, -1.0f) // up axis
                );
            }
            else
            {
                front.view = glm::lookAt(
                    front.altPosition, // position
                    glm::vec3(1.52f, -33.28f, 6.23f), // camera center
                    glm::vec3(0.0f, 0.0f, -1.0f) // up axis
                );
            }
        }
        else
        {
            side.viewMode *= -1;
            if (side.viewMode == 1)
            {
                side.view = glm::lookAt(
                    side.position, // position
                    glm::vec3(1.52f, -33.28f, 6.23f), // camera center
                    glm::vec3(0.0f, 0.0f, -1.0f) // up axis
                );
            }
            else
            {
                side.view = glm::lookAt(
                    side.altPosition, // position
                    glm::vec3(1.52f, -33.28f, 6.23f), // camera center
                    glm::vec3(0.0f, 0.0f, -1.0f) // up axis
                );
            }
        }
    }
    else
    {
        if (ypos < (HEIGHT / 2))
        {
            top.viewMode *= -1;
            if (top.viewMode == 1)
            {
                top.view = glm::lookAt(
                    top.position, // position
                    glm::vec3(1.52f, -33.28f, -80.23f), // camera center
                    glm::vec3(0.0f, 0.0f, 1.0f) // up axis
                );
            }
            else
            {
                top.view = glm::lookAt(
                    top.altPosition, // position
                    glm::vec3(1.52f, -33.28f, -80.23f), // camera center
                    glm::vec3(0.0f, 0.0f, 1.0f) // up axis
                );
            }
        }
    }
}

//helper function that will render text at a 3D point using Qts text renderer
void GLWidget::renderText(glm::mat4 model, Camera cam, glm::vec4 viewport, const QString &str)
{
    // Identify x and y locations to render text within widget
    glm::vec3 textPos = glm::vec3(0,0,0);
    textPos = glm::project(glm::vec3(model[3]), cam.view, cam.proj, viewport);

    NText text;
    text.x = textPos.x;
    text.y = textPos.y;
    text.str = str;
    nodeTexts.push_back(text);
}

GLWidget::~GLWidget()
{
}
