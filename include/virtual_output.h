/*
* This file is originally from pyvirtualcam and was modified on 2024-03-01
*/


#pragma once

#include <stdio.h>
#include <stdexcept>
#include <Windows.h>
#include <vector>
#include <cmath>
#include "shared-memory-queue.h"

class VirtualOutput {
  private:
    bool _output_running = false;
    video_queue_t *_vq;
    uint32_t _frame_width;
    uint32_t _frame_height;
    std::vector<uint8_t> _buffer_tmp;
    std::vector<uint8_t> _buffer_output;
    bool _have_clockfreq = false;
    LARGE_INTEGER _clock_freq;

    uint64_t get_timestamp_ns()
    {
        if (!_have_clockfreq) {
            QueryPerformanceFrequency(&_clock_freq);
            _have_clockfreq = true;
        }

        LARGE_INTEGER current_time;
        QueryPerformanceCounter(&current_time);
        double time_val = (double)current_time.QuadPart;
        time_val *= 1000000000.0;
        time_val /= (double)_clock_freq.QuadPart;

        return static_cast<uint64_t>(time_val);
    }

  public:
    VirtualOutput(uint32_t width, uint32_t height, double fps) {
        LPCWSTR guid = L"CLSID\\{689685A0-B481-11EE-9EC1-0800200C9A66}";
        HKEY key = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, guid, 0, KEY_READ, &key) != ERROR_SUCCESS) {
            throw std::runtime_error(
                "Open Virtual Camera device not found! "
                "Did you install Open Virtual Camera?"
            );
        }

        _frame_width = width;
        _frame_height = height;

        uint64_t interval = (uint64_t)(10000000.0 / fps);

        _vq = video_queue_create(width, height, interval);

        if (!_vq) {
            throw std::runtime_error("virtual camera output could not be started");
        }

        _output_running = true;
    }

    void stop()
    {
        if (!_output_running) {
            return;
        }	
        video_queue_close(_vq);
        _output_running = false;
    }

    void send(const uint8_t *frame)
    {
        if (!_output_running)
            return;

        uint8_t* tmp = _buffer_tmp.data();
        uint8_t* out_frame;

        out_frame = const_cast<uint8_t*>(frame);
        // NV12 has two planes
        uint8_t* y = out_frame;
        uint8_t* uv = out_frame + _frame_width * _frame_height;

        // One entry per plane
        uint32_t linesize[2] = { _frame_width, _frame_width / 2 };
        uint8_t* data[2] = { y, uv };

        uint64_t timestamp = get_timestamp_ns();

        video_queue_write(_vq, data, linesize, timestamp);
    }

};
