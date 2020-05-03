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

    std::vector<float> distanceLeftEye, distanceRightEye;
    std::vector<glm::vec2> positionLeftEye, positionRightEye;

    //std::vector<float> leftEye, rightEye;
    int windowSize = 10;

    float avg(std::vector<float> q) {
        float sum = 0;
        for (auto i : q)
            sum += i;

        return sum / q.size();
    }

    float avg(std::vector<glm::vec2> q) {
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

    bool compareVec2(glm::vec2 a, glm::vec2 b) {
        return glm::length(a) < glm::length(b);
    }
    float med(std::vector<glm::vec2> q) {
        std::sort(q.begin(), q.end(), less_than_key());
        return glm::length(q[q.size() / 2]);
    }

    float getDistance(float orSize, float f, std::vector<float> q) {
        float d = orSize * f / avg(q);

        return d;
    }
    float getPosition(float center, std::vector<glm::vec2>& q) {
        float d = center - med(q);

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

        for (size_t i = 0; i < faces.size(); i++)
        {
            //Point center(faces[i].x + faces[i].width / 2, faces[i].y + faces[i].height / 2);
            //ellipse(frame, center, Size(faces[i].width / 2, faces[i].height / 2), 0, 0, 360, Scalar(255, 0, 255), 4);

            //std::cout << "Face: " << center.x << "," << center.y << ". ";

            Mat faceROI = frame_gray(faces[i]);

            //-- In each face, detect eyes
            std::vector<Rect> eyes;
            eyes_cascade.detectMultiScale(faceROI, eyes);

            for (size_t j = 0; j < eyes.size(); j++)
            {
                glm::vec2 eye_center(faces[i].x + eyes[j].x + eyes[j].width / 2, faces[i].y + eyes[j].y + eyes[j].height / 2);
                //int radius = cvRound((eyes[j].width + eyes[j].height) * 0.25);
                //circle(frame, eye_center, radius, Scalar(255, 0, 0), 4);

                //std::cout << "Eye: " << center.x << "," << center.y << " ";
                //std::cout << "Size: " << eyes[j].width << "," << eyes[j].height << " ";

                if (j == 0)
                {
                    distanceLeftEye.push_back(eyes[0].width);
                    if (distanceLeftEye.size() > windowSize)
                    {
                        distanceLeftEye.erase(distanceLeftEye.begin());
                        distanceLeft = getDistance(3, 900, distanceLeftEye);
                        //std::cout << "left: " << distanceLeft << " ";
                    }
                    //positionLeftEye.push_back(eye_center);
                    //if (positionLeftEye.size() > windowSize)
                    //{
                    //    positionLeftEye.erase(positionLeftEye.begin());
                    //    positionLeft = getPosition(frame.size[0] / 2, positionLeftEye);
                    //    //std::cout << "left: " << distanceLeft << " ";
                    //}
                }
                if (j == 1)
                {
                    distanceRightEye.push_back(eyes[1].width);
                    if (distanceRightEye.size() > windowSize)
                    {
                        distanceRightEye.erase(distanceRightEye.begin());
                        distanceRight = getDistance(3, 900, distanceRightEye);
                        //std::cout << "right: " << distanceRight << " ";

                    }
                    positionLeftEye.push_back(eye_center);
                    positionRightEye.push_back(eye_center);
                    if (positionLeftEye.size() > windowSize)
                    {
                        positionLeftEye.erase(positionLeftEye.begin());
                        positionLeft = getPosition(frame.size[0] / 2, positionLeftEye);
                        //std::cout << "left: " << distanceLeft << " ";
                    }
                    if (positionRightEye.size() > windowSize)
                    {
                        positionRightEye.erase(positionRightEye.begin());
                        positionRight = getPosition(frame.size[0] / 2, positionRightEye);
                        //std::cout << "left: " << distanceLeft << " ";
                    }
                    positionHorizontal = (positionLeft + positionRight) / 2.0;

                }
            }
        }

        //-- Show what you got
        //imshow("Capture - Face detection", frame);
    }


public:
    float 
        distanceLeft,
        distanceRight, 
        positionLeft,
        positionRight,
        positionHorizontal
        ;

    bool Init() {
       /* CommandLineParser parser(argc, argv,
            "{help h||}"
            "{face_cascade|data/haarcascades/haarcascade_frontalface_alt.xml|Path to face cascade.}"
            "{eyes_cascade|data/haarcascades/haarcascade_eye_tree_eyeglasses.xml|Path to eyes cascade.}"
            "{camera|0|Camera device number.}");

        parser.about("\nThis program demonstrates using the cv::CascadeClassifier class to detect objects (Face + eyes) in a video stream.\n"
            "You can use Haar or LBP features.\n\n");
        parser.printMessage();*/

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



