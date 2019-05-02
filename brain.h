#ifndef BRAIN_H
#define BRAIN_H

#define GLM_ENABLE_EXPERIMENTAL

#include <string>
#include <vector>
#include <fstream>

#include <qopenglfunctions_3_2_core.h>
#include <qopenglwidget.h>
#include <qmessagebox.h>
#include <qdatetime.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/intersect.hpp>

#include "model.h"
#include "camera.h"
#include "mri.h"
#include "colorsettings.h"
#include <boost/algorithm/string.hpp>

class GLWidget;

class Brain
{
    Model sphere;
    Model connector;
    std::vector<glm::mat4> nodePositions;
    std::vector<int> nodeColors;
    std::vector<float> nodeSizes;
    std::vector<std::vector<std::vector<float>>> connections;

    glm::mat4 position;

    std::vector<std::vector<float>> appendedNodeData;
public:
    Brain(QOpenGLFunctions_3_2_Core *f, std::string nodePath, QStringList connectionPath);
    Brain();
    void reloadBrain(std::string nodePath, QStringList connectionPath);
    void update(QOpenGLFunctions_3_2_Core *f, Camera &camera, float xpos, float ypos, int &selectedNode, int mouseDown);
    void setPosition(glm::vec3 position);
    void loadAppendedNodeData(std::string filepath);

    Model mesh;

    GLWidget *screen;
    std::vector<std::string> nodeNames;
    float threshold = 0.5f;
    float textThreshold = 1.0f;

    NColor *colors;
    float nodeSize = 1.5f;
    bool isScaling = false;
    float connectionSize = 0.3f;
    float graphSignalSize = 20.0f;

    int displayShell = 1;
    int displayMri = -1;
    MRI mri;

    bool hasAppendedData = false;
    bool hasTime = false;
    int currentFrame = 0;
    int numFrames = 0;
    int *milisecondsPerFrame;
    quint64 nextFrameTime = 0;
};



#endif // BRAIN_H
