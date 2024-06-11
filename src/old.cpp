/*
 *********************************************************
 * Copyright (c) 2008-2024, FlatFrog Laboratories AB
 * All rights reserved.
 *********************************************************
 */

#include <iostream>
#include <fstream>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include "argparse.hpp"
#include "findcam.hpp"
#include "virtual_output.h"

struct Args
{
    bool user_specified_camera_index;
    int camera_devide_index;
    int camera_width;
    int camera_height;
    int camera_fps;
    bool debug_prints;
};

Args parse_args(int argc, char** argv)
{
    argparse::ArgumentParser parser("FF Virtual Camera");

    parser.add_argument("-c", "--camera_device_index").default_value(0).scan<'i', int>().help("Camera device index.");
    parser.add_argument("-cw", "--camera_width").default_value(3840).scan<'i', int>().help("Target camera width.");
    parser.add_argument("-ch", "--camera_height").default_value(2160).scan<'i', int>().help("Target camera height.");
    parser.add_argument("-cf", "--camera_fps").default_value(30).scan<'i', int>().help("Target camera fps.");
    parser.add_argument("--4k").default_value(false).implicit_value(true).help("Request 4k camera resolution");
    parser.add_argument("--fullhd").default_value(false).implicit_value(true).help("Request full HD camera resolution");
    parser.add_argument("--debug_prints").default_value(false).implicit_value(true).help("Print debug messages");
 

    try 
    {
        parser.parse_args(argc, argv);
    }
    catch (const std::exception& err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }

    int camera_width = parser.get<int>("--camera_width");
    int camera_height = parser.get<int>("--camera_height");

    if (parser.get<bool>("--fullhd"))
    {
        camera_width = 1920;
        camera_height = 1080;
    }

    if (parser.get<bool>("--4k"))
    {
        camera_width = 3840;
        camera_height = 2160;
    }

    return Args
    {
        parser.is_used("--camera_device_index"),
        parser.get<int>("--camera_device_index"),
        camera_width,
        camera_height,
        parser.get<int>("--camera_fps"),
        parser.get<bool>("--debug_prints")
    };
}

int main(int argc, char** argv) 
{
    Args args = parse_args(argc, argv);
    std::cout << "OpenCV version : " << CV_VERSION << std::endl;

    wchar_t buffer[MAX_PATH], drive[MAX_PATH] ,directory[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH); 
    _wsplitpath(buffer, drive, directory, NULL, NULL);
    std::wstring s_drive(drive), s_dir(directory);

    std::ofstream errorFile(s_drive + s_dir + L"ErrorFile.log");
    std::cerr.rdbuf(errorFile.rdbuf());

    if (!args.user_specified_camera_index) {
        std::wifstream camera_name_file(s_drive + s_dir + L"camera_name.txt");
        if(camera_name_file.good()){
            wchar_t camera_name[256];
            camera_name_file.getline(camera_name, 256);
            int camera_index = FindDeviceByName(camera_name);
            if (camera_index >= 0) {
                std::wcout << "Camera " << camera_name << " identified as device " << camera_index << std::endl;
                args.camera_devide_index = camera_index;
            } else {
                std::wcout << "Camera with name " << camera_name <<
                " specified in file camera_name.txt not found, using device " <<
                args.camera_devide_index  << std::endl;
            }
        } else {
            std::cout << "camera_name.txt file not found" << std::endl;
        }
        camera_name_file.close();
    }
restart:
    cv::VideoCapture cap;

    size_t fail_count = 0;
    bool camera_connected = false;
    while (!camera_connected) {
        std::cout << "Trying to capture device " << args.camera_devide_index << " ...     ";
        cap = cv::VideoCapture(args.camera_devide_index, cv::CAP_MSMF);
        cap.set(cv::CAP_PROP_FRAME_WIDTH, args.camera_width);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, args.camera_height);
        cap.set(cv::CAP_PROP_FPS, args.camera_fps);
        cap.set(cv::CAP_PROP_CONVERT_RGB, 0.0);
        if (cap.isOpened()) {
            camera_connected = true;
            std::cout << std::endl << "Successfully captured device "<< args.camera_devide_index << std::endl;
        } else {
            fail_count++;
            std::cout << "Failed " << fail_count << " time(s), make sure its connected, trying again...\r";
            cap.release();
            cv::waitKey(2000);
        }
    }

    size_t image_size = args.camera_width * args.camera_height * 3 / 2;

    cv::Mat raw_frame(1, image_size, CV_8UC1);
    VirtualOutput *virtual_output;
    try {
        virtual_output = new VirtualOutput(args.camera_width, args.camera_height, args.camera_fps);
    } catch (std::runtime_error err) {
        std::cout << err.what() << std::endl;
        return EXIT_FAILURE;
    }

    bool successful_camera_grab = false;

    while (true) {
        bool read_success = true;
        int fail_count = 0;
        auto start = std::chrono::steady_clock::now();
        do {
            try {
                read_success = cap.read(raw_frame);
            } catch (const std::exception &exc) {
                cap.release();
                virtual_output->stop();
                delete virtual_output;
                std::cerr << "ERROR: " << exc.what() << std::endl;
                goto restart;
            } catch(...) {
                cap.release();
                virtual_output->stop();
                delete virtual_output;
                std::cerr << "Unknown error reading from camera" << std::endl;
                goto restart;
            }

            if (!successful_camera_grab && read_success) {
                successful_camera_grab = true;
                if (!args.debug_prints) {
                    FreeConsole();
                }
            }

            if (!read_success) {
                fail_count++;
                std::cout << "Failed reading from camera " << fail_count << " time(s), it is probaby in use by another application, retrying...\r";
                cv::waitKey(2000);
                if (fail_count > 5) {
                    std::cout << "Failed reading camera too many times, it is probably in use by another application, recapturing camera..." << std::endl;
                    cap.release();
                    virtual_output->stop();
                    delete virtual_output;
                    goto restart;
                }
            }
        } while (!read_success);

        auto finish = std::chrono::steady_clock::now();
        std::cout << "cam read" << std::chrono::duration_cast<std::chrono::duration<double>>(finish - start).count();
        start = finish;
        virtual_output->send(static_cast<uint8_t*>(raw_frame.data));
        auto finish2 = std::chrono::steady_clock::now();
        std::cout << "cam put" << std::chrono::duration_cast<std::chrono::duration<double>>(finish2 - start).count();
    }

    virtual_output->stop();
    cap.release();
    delete virtual_output;

    return 0;
}

