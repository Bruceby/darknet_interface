#include "detector.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#ifdef OPENCV
#include "opencv2/core/types_c.h"
#include <opencv2/highgui/highgui.hpp>  
#include <opencv2/imgproc/imgproc.hpp>  
#include <opencv2/core/core.hpp> 
#include <thread>
int FRAME_WINDOW_WIDTH = 320;
int FRAME_WINDOW_HEIGHT = 240;
using namespace std;
using namespace cv;  

void detect_mat(Mat frame_detect, float* detections_output, int* num_output_class, double* time_consumed, float thresh, float hier_thresh)
{
    double time = what_is_the_time_now();
    IplImage input = IplImage(frame_detect);// convert image format from mat to iplimage

    // do detect in an iplimage converted from mat
    float* detections = test_detector_cv(&input, thresh, hier_thresh, num_output_class);
    for(int i = 0; i < *num_output_class; i++)
    {
        printf("%.0f: %.0f%%", detections[i*6+0], detections[i*6+1] * 100);
        printf("\t(left_x: %4.0f   top_y: %4.0f   width: %4.0f   height: %4.0f)\n",
        detections[i*6+2], detections[i*6+3], detections[i*6+4], detections[i*6+5]);
        detections_output[i*6+0] = detections[i*6+0];// ith detection's category
        detections_output[i*6+1] = detections[i*6+1];// ith detection's confidence score
        detections_output[i*6+2] = detections[i*6+2];// ith detection's top-left x-coordinate of bbox
        detections_output[i*6+3] = detections[i*6+3];// ith detection's top-left y-coordinate of bbox
        detections_output[i*6+4] = detections[i*6+4];// ith detection's width of bbox
        detections_output[i*6+5] = detections[i*6+5];// ith detection's height of bbox
    }
    if(time_consumed)
        *time_consumed = (what_is_the_time_now() - time);
}

void show_detection(Mat frame, float* detections, int num_output_class, Rect detections_rect, bool is_show_image, bool is_show_detections)
{
    if(is_show_image)
    {
        if(is_show_detections)
        {
            for(int i = 0; i < num_output_class; i++)
            {
                detections_rect.x = detections[i*6+2];
                detections_rect.y = detections[i*6+3];
                detections_rect.width = detections[i*6+4];
                detections_rect.height = detections[i*6+5];
                if(detections[i*6+0] == 0) // person
                    rectangle(frame, detections_rect, CV_RGB(255, 0, 0), 4, 8, 0);
                else if(detections[i*6+0] == 62 || detections[i*6+0] == 63) // tvmonitor && laptop
                    rectangle(frame, detections_rect, CV_RGB(0, 0, 255), 4, 8, 0);
                else if(detections[i*6+0] == 28 || detections[i*6+0] == 56)//  suitcase && chair
                    rectangle(frame, detections_rect, CV_RGB(0, 255, 0), 4, 8, 0);
                else
                    rectangle(frame, detections_rect, CV_RGB(0, 0, 0), 4, 8, 0);
                String info = "cls:"+ to_string(int(detections[i*6+0])) +", conf:"+ to_string(int(detections[i*6+1]*100)) +"%";
                putText(frame, info, Point(detections_rect.x, detections_rect.y-10), FONT_HERSHEY_SIMPLEX, 1, Scalar(0,255,0), 2, 8);
            }
        }
        namedWindow("detections", cv::WINDOW_AUTOSIZE);
        imshow("detections", frame);
        waitKey(1);
    }
}
#endif

int main(int argc, char** argv)
{
    if (argc < 5)
    {
        cout << "Usage: " << endl
        << "  detect image :" << endl << "    $ "
        << argv[0] << " image cfg_path weight_path thresh image_path" << endl
        << "  detect video :" << endl << "    $ "
        << argv[0] << " video cfg_path weight_path thresh video_path" << endl
        << "  detect webcam:" << endl << "    $ "
        << argv[0] << " webcam cfg_path weight_path thresh webcam_index" << endl
        << endl;
        return -1;
    }
    // ====== init ======
    char *type = argv[1];
    char *cfgfile = argv[2];
    char *weightfile = argv[3];
    float thresh = atof(argv[4]);
    float hier_thresh = 0.5;
    int num_output_class = 0;
    double time_consumed = 0;
    VideoCapture cap;
    float *detections;
    Rect detections_rect;
#ifdef OPENCV
    bool is_show_image = true;
    bool is_show_detections = true;
    bool is_detect_in_thread = false;
#endif

    detector_init(cfgfile, weightfile);

    // ====== detect in image ======
    if(0==strcmp(type, "image"))
    {
        cout << "detect image" << endl;
        char *image_path = argv[5];
        double time;
        for(int i = 0; i < 10; i++)
        {
            time = what_is_the_time_now();
            detections = test_detector(image_path, thresh, hier_thresh, &num_output_class);
            printf("Predicted in %f seconds.\n", what_is_the_time_now() - time);
            printf("num_output_class:%d\n", num_output_class);
            for(int i = 0; i < num_output_class; i++)
            {
                printf("%.0f: %.0f%%", detections[i*6+0],	detections[i*6+1] * 100);
                printf("\t(left_x: %4.0f   top_y: %4.0f   width: %4.0f   height: %4.0f)\n",
                    detections[i*6+2], detections[i*6+3], detections[i*6+4], detections[i*6+5]);
            }
#ifdef OPENCV
            // show detections using opencv
            show_detection(imread(image_path), detections, num_output_class, 
                            detections_rect, is_show_image, is_show_detections);
#endif
        }
        return 0;
    }
    // ====== detect in video or webcam ======
    else if(0==strcmp(type, "video"))
    {
        cout << "detect video" << endl;
        cap = VideoCapture(argv[5]);// load video from file
    }
    else if(0==strcmp(type, "webcam"))
    {
        cout << "detect webcam" << endl;
        cap = VideoCapture(atoi(argv[5]));// init camera
    }
#ifndef OPENCV
    cout << "need OpenCV!" << endl;
    return -1;
#endif
    bool isOpened = true;
    if(!cap.isOpened())  
        isOpened = false;
    if(!isOpened)
    {
        printf("No video stream captured \n");
        return -1;
    }
    Mat frame;
    detections = (float*)calloc(255*6, sizeof(float));
    bool stop = false;  
    while(cap.read(frame))  
    {
        if(isOpened)
        {
            // --- detect objects ---
            if(!is_detect_in_thread)
            {
                detect_mat(frame, detections, &num_output_class, &time_consumed, thresh, hier_thresh);
                cout << "time_consumed: " << (float)time_consumed 
                    << ", frame rate: " << 1/(float)time_consumed << endl;
            }
            else// do detect in a background thread
            {
                if(time_consumed >= 0)
                {
                    thread t(detect_mat, frame, detections, &num_output_class, &time_consumed, thresh, hier_thresh);
                    t.detach();
                    printf("time_consumed: %f \n", (float)time_consumed);
                    cout << "time_consumed: " << (float)time_consumed 
                        << ", frame rate: " << 1/(float)time_consumed << endl;
                    time_consumed = -1;
                }
            }
            
            // --- show detections ---
            show_detection(frame, detections, num_output_class, detections_rect, is_show_image, is_show_detections);
            if(waitKey(1) >=0)
                stop = true;
        }
    }

    // ====== uninit ======
    detector_uninit();

    return 0;
}

