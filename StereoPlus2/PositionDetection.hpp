#pragma once

#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <iostream>
#include "include/glm/glm.hpp"
#include <queue>

using namespace std;
using namespace cv;

class PositionDetector {
    VideoCapture capture;
    /** Global variables */
    CascadeClassifier face_cascade;
    CascadeClassifier eyes_cascade;

    std::list<float> distanceLeftEye, distanceRightEye;
    std::list<glm::vec2> 
        positionLeftEye, 
        positionRightEye;

    //std::vector<float> leftEye, rightEye;
    int windowSize = 10;

    float avg(std::list<float> q) {
        float sum = 0;
        for (auto i : q)
            sum += i;

        return sum / q.size();
    }

    float avg(std::list<glm::vec2> q) {
        glm::vec2 sum = glm::vec2();
        for (auto i : q)
            sum += i;

        return glm::length(sum / (float)q.size());
    }

    struct less_than_key
    {
        inline bool operator() (const glm::vec2& a, const glm::vec2& b)
        {
            return glm::length(a) < glm::length(b);
        }
    };

    struct less_than_keyX
    {
        inline bool operator() (const glm::vec2& a, const glm::vec2& b)
        {
            return a.x < b.x;
        }
    };
    struct less_than_keyY
    {
        inline bool operator() (const glm::vec2& a, const glm::vec2& b)
        {
            return a.y < b.y;
        }
    };
    template<typename T>
    const T& at(const std::list<T>& q, int pos) {
        auto i = q.cbegin();

        while (pos-- > 0) i++;

        return i._Ptr->_Myval;
    }

    float med(std::list<glm::vec2> q) {
        q.sort(less_than_key());
        return glm::length(at(q, q.size() / 2));
    }

    float medX(std::list<glm::vec2> q) {
        q.sort(less_than_keyX());
        return at(q, q.size() / 2).x;
    }
    float medY(std::list<glm::vec2> q) {
        q.sort(less_than_keyY());
        return at(q, q.size() / 2).y;
    }

    float getDistance(float orSize, float f, const std::list<float>& q) {
        float d = orSize * f / avg(q);

        return d;
    }
    float getPosX(float center, const std::list<glm::vec2>& q) {
        float d = center - medX(q);

        return d;
    }
    float getPosY(float center, const std::list<glm::vec2>& q) {
        float d = center - medY(q);

        return d;
    }

    void detectAndDisplay(Mat frame)
    {
        Mat frame_gray;
        cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
        equalizeHist(frame_gray, frame_gray);

        //-- Detect faces
        std::vector<Rect> faces;
        face_cascade.detectMultiScale(frame_gray, faces);
        
        //bool j = useOptimized();

        for (size_t i = 0; i < faces.size(); i++)
        {
            //Point center(faces[i].x + faces[i].width / 2, faces[i].y + faces[i].height / 2);
            //ellipse(frame, center, Size(faces[i].width / 2, faces[i].height / 2), 0, 0, 360, Scalar(255, 0, 255), 4);

            //std::cout << "Face: " << center.x << "," << center.y << ". ";

            Mat faceROI = frame_gray(faces[i]);

            //-- In each face, detect eyes
            std::vector<Rect> eyes;
            eyes_cascade.detectMultiScale(faceROI, eyes);

            if (eyes.size() == 1)
            {
                glm::vec2 eye_center(faces[i].x + eyes[0].x + eyes[0].width / 2, faces[i].y + eyes[0].y + eyes[0].height / 2);

                // if is left eye
                if (eye_center.x < (faces[i].x + faces[i].width / 2)) {
                    distanceLeftEye.push_back(eyes[0].width);
                    positionLeftEye.push_back(eye_center);

                    if (distanceLeftEye.size() > windowSize)
                        distanceLeftEye.pop_front();
                    if (positionLeftEye.size() > windowSize)
                        positionLeftEye.pop_front();

                    distanceLeft = getDistance(3, 900, distanceLeftEye);
                    distance = (distanceLeft + distanceRight) / 2;

                    positionLeft = getPosX(frame.size[1] / 2, positionLeftEye);
                    positionHorizontal = (positionLeft + positionRight) / 2;
                    heightLeft = getPosY(frame.size[0] / 2, positionLeftEye);
                    positionVertical = (heightLeft + heightRight) / 2;
                }
                else
                {
                    distanceRightEye.push_back(eyes[0].width);
                    positionRightEye.push_back(eye_center);

                    if (distanceRightEye.size() > windowSize)
                        distanceRightEye.pop_front();
                    if (positionRightEye.size() > windowSize)
                        positionRightEye.pop_front();

                    distanceRight = getDistance(3, 900, distanceRightEye);
                    distance = (distanceLeft + distanceRight) / 2;

                    positionRight = getPosX(frame.size[1] / 2, positionRightEye);
                    positionHorizontal = (positionLeft + positionRight) / 2;
                    heightRight = getPosY(frame.size[0] / 2, positionRightEye);
                    positionVertical = (heightLeft + heightRight) / 2;
                }
            }
            else if (eyes.size() > 1)
            {
                glm::vec2 eye_centerLeft(faces[i].x + eyes[0].x + eyes[0].width / 2, faces[i].y + eyes[0].y + eyes[0].height / 2);
                glm::vec2 eye_centerRight(faces[i].x + eyes[1].x + eyes[1].width / 2, faces[i].y + eyes[1].y + eyes[1].height / 2);

                distanceLeftEye.push_back(eyes[0].width);
                distanceRightEye.push_back(eyes[1].width);
                positionLeftEye.push_back(eye_centerLeft);
                positionRightEye.push_back(eye_centerRight);

                if (distanceLeftEye.size() > windowSize)
                    distanceLeftEye.pop_front();
                if (distanceRightEye.size() > windowSize)
                    distanceRightEye.pop_front();
                if (positionLeftEye.size() > windowSize)
                    positionLeftEye.pop_front();
                if (positionRightEye.size() > windowSize)
                    positionRightEye.pop_front();

                distanceLeft = getDistance(3, 900, distanceLeftEye);
                distanceRight = getDistance(3, 900, distanceRightEye);
                distance = (distanceLeft + distanceRight) / 2;

                positionLeft = getPosX(frame.size[1] / 2, positionLeftEye);
                positionRight = getPosX(frame.size[1] / 2, positionRightEye);
                positionHorizontal = (positionLeft + positionRight) / 2;

                heightLeft = getPosY(frame.size[0] / 2, positionLeftEye);
                heightRight = getPosY(frame.size[0] / 2, positionRightEye);
                positionVertical = (heightLeft + heightRight) / 2;
            }

        }

        //-- Show what you got
        //imshow("Capture - Face detection", frame);
    }


public:
    float 
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

    bool Init() {
        String face_cascade_name = samples::findFile("data/haarcascades/haarcascade_frontalface_alt.xml");
        String eyes_cascade_name = samples::findFile("data/haarcascades/haarcascade_eye_tree_eyeglasses.xml");

        //-- 1. Load the cascades
        if (!face_cascade.load(face_cascade_name))
        {
            cout << "--(!)Error loading face cascade\n";
            return false;
        };
        if (!eyes_cascade.load(eyes_cascade_name))
        {
            cout << "--(!)Error loading eyes cascade\n";
            return false;
        };

        int camera_device = 0;
        //-- 2. Read the video stream
        capture.open(camera_device);
        if (!capture.isOpened())
        {
            cout << "--(!)Error opening video capture\n";
            return false;
        }

        setUseOptimized(true);

        return true;
    }

    bool ProcessFrame() {
       /* if (!capture.isOpened())
        {
            cout << "--(!)Error opening video capture\n";
            return false;
        }*/
        Mat frame;
        if (capture.read(frame))
        {
            if (frame.empty())
            {
                cout << "--(!) No captured frame -- Break!\n";
                return false;
            }

            //-- 3. Apply the classifier to the frame
            detectAndDisplay(frame);
            return true;
        }

        return false;
    }
};



