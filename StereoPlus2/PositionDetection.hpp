#pragma once

#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>
#include "include/glm/glm.hpp"
#include <queue>
#include <future>
#include "InfrastructureTypes.hpp"

using namespace std;
using namespace cv;

class PositionDetector {
    struct less_than_keyX {
        inline bool operator() (const glm::vec2& a, const glm::vec2& b) {
            return a.x < b.x;
        }
    };
    struct less_than_keyY {
        inline bool operator() (const glm::vec2& a, const glm::vec2& b) {
            return a.y < b.y;
        }
    };

    const Log log = Log::For<PositionDetector>();

    VideoCapture capture;
    CascadeClassifier face_cascade;
    CascadeClassifier eyes_cascade;
    
    int windowSize = 10;
    std::list<float> distanceLeftEye, distanceRightEye;
    std::list<glm::vec2> positionLeftEye, positionRightEye;

    std::atomic<bool> mustStopPositionProcessing;
    std::thread distanceProcessThread;


    std::list<float> faceSize;
    std::list<glm::vec2> facePosition;
    std::list<float> distanceToCameraBuffer;


    float avg(std::list<float> q) {
        float sum = 0;
        for (auto i : q)
            sum += i;

        return sum / q.size();
    }

    template<typename T>
    const T& at(const std::list<T>& q, int pos) {
        auto i = q.cbegin();

        while (pos-- > 0) i++;

        return i._Ptr->_Myval;
    }

    float medX(std::list<glm::vec2> q) {
        q.sort(less_than_keyX());
        return at(q, q.size() / 2).x;
    }
    float medY(std::list<glm::vec2> q) {
        q.sort(less_than_keyY());
        return at(q, q.size() / 2).y;
    }


    float getPosX(float center, const std::list<glm::vec2>& q, float distance) {
        auto pixelToAngle = divide(cameraResolution, cameraViewAngle);
        auto angleFaceSize = (center - medX(q)) / pixelToAngle.x;
        auto posX = distance * tan(angleFaceSize * degreeToRadian);
        return posX;
    }
    float getPosY(float center, const std::list<glm::vec2>& q, float distance) {
        auto pixelToAngle = divide(cameraResolution, cameraViewAngle);
        auto angleFaceSize = (center - medY(q)) / pixelToAngle.x;
        auto posX = distance * tan(angleFaceSize * degreeToRadian);
        return posX;
    }

    float getPosY(float center, const std::list<glm::vec2>& q) {
        float d = center - medY(q);

        return d;
    }

    glm::vec2 divide(const glm::vec2& v1, const glm::vec2& v2) {
        return glm::vec2(v1.x / v2.x, v1.y / v2.y);
    }
    glm::vec2 multiply(const glm::vec2& v1, const glm::vec2& v2) {
        return glm::vec2(v1.x * v2.x, v1.y * v2.y);
    }

    float getDistanceToCamera(const std::list<float>& pixelFaceSizesY) {
        auto pixelToAngle = divide(cameraResolution, cameraViewAngle);
        auto angleFaceSize = avg(pixelFaceSizesY) / pixelToAngle.y;
        auto distance = faceSizeRealY / (tan(angleFaceSize / 2.f * degreeToRadian) * 2.f);
        return distance;
    }

    void detectAndDisplay(Mat frame)
    {
        Mat frame_gray;
        cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
        equalizeHist(frame_gray, frame_gray);

        //-- Detect faces
        std::vector<Rect> faces;
        face_cascade.detectMultiScale(frame_gray, faces);

        for (size_t i = 0; i < faces.size(); i++)
        {
            glm::vec2 center(faces[i].x + faces[i].width / 2, faces[i].y + faces[i].height / 2);

            faceSize.push_back(faces[i].width);
            facePosition.push_back(center);

            if (faceSize.size() > windowSize)
                faceSize.pop_front();
            if (facePosition.size() > windowSize)
                facePosition.pop_front();

            auto distanceToCamera = getDistanceToCamera(faceSize);
            distanceToCameraBuffer.push_back(distanceToCamera);
            if (distanceToCameraBuffer.size() > windowSize)
                distanceToCameraBuffer.pop_front();
            distanceToCamera = avg(distanceToCameraBuffer);

            auto pixelToAngle = divide(cameraResolution, cameraViewAngle);

            auto facePositionMedianY = medY(facePosition);
            auto angleFaceCenterY = (frame.size[0] / 2.f - facePositionMedianY) / pixelToAngle.y;
            auto alpha = cameraAngle.y - angleFaceCenterY;
            auto distanceToScreen = distanceToCamera * sin(alpha * degreeToRadian) + screenCenterToCameraDistance.z;
            distance = distanceToScreen * 1.2;
            auto posHorizontal = getPosX(frame.size[1] / 2.f, facePosition, distanceToCamera);
            positionHorizontal = posHorizontal;


            auto posVerticalRelativeToCamera = distanceToCamera * cos(alpha * degreeToRadian);
            auto posVertical = posVerticalRelativeToCamera - screenCenterToCameraDistance.y;
            positionVertical = posVertical;


        }

        //-- Show what you got
        //imshow("Capture - Face detection", frame);
    }

    void distanceProcess() {
        while (!mustStopPositionProcessing)
        {
            if (!ProcessFrame())
                break;
        }
    }

    bool ProcessFrame() {
        Mat frame;
        if (capture.read(frame))
        {
            if (frame.empty())
            {
                log.Error("No captured frame\n");
                return false;
            }

            detectAndDisplay(frame);
            return true;
        }

        return false;
    }


public:
    std::atomic<float>
        distanceLeft,
        distanceRight, 
        distance,

        positionLeft,
        positionRight,
        positionHorizontal,

        heightLeft,
        heightRight,
        positionVertical
        ;

    bool isPositionProcessingWorking;

    std::function<void()> onStartProcess = [] {};
    std::function<void()> onStopProcess = [] {};


    const float degreeToRadian = 3.1415926f * 2 / 360;

    glm::vec2 cameraResolution = glm::vec2(640, 480);
    // Degrees
    // These angles aren't correct. 
    // Just found the values, that produce the best results.
    // Need to find out the real values.
    glm::vec2 cameraViewAngle = glm::vec2(47, 35);
    glm::vec2 cameraAngle = glm::vec2(0, 65);

    // Millimeters
    float faceSizeRealY = 165;
    glm::vec3 screenCenterToCameraDistance = glm::vec3(0, 170, 30);


    bool Init() {
        String face_cascade_name = samples::findFile("data/haarcascades/haarcascade_frontalface_alt.xml");
        String eyes_cascade_name = samples::findFile("data/haarcascades/haarcascade_eye_tree_eyeglasses.xml");

        //-- 1. Load the cascades
        if (!face_cascade.load(face_cascade_name))
        {
            log.Error("Error loading face cascade");
            return false;
        };
        if (!eyes_cascade.load(eyes_cascade_name))
        {
            log.Error("Error loading eyes cascade");
            return false;
        };

        int camera_device = 0;
        //-- 2. Read the video stream
        capture.open(camera_device);
        if (!capture.isOpened())
        {
            log.Error("Error opening video capture\n");
            return false;
        }

        setUseOptimized(true);

        return true;
    }

    void StartPositionDetection() {
        if (isPositionProcessingWorking)
            return;

        isPositionProcessingWorking = true;
        mustStopPositionProcessing = false;

        // A separate thread for position detection
        distanceProcessThread = std::thread([=]() {
            onStartProcess();
            distanceProcess();
            onStopProcess();
        });
    }

    void StopPositionDetection() {
        if (!isPositionProcessingWorking)
            return;

        isPositionProcessingWorking = false;
        mustStopPositionProcessing = true;

        // Join the thread to clean it
        distanceProcessThread.join();
    }
};



