#pragma once

#include "atcore.h"
#include <memory>
#include <vector>
#include <string>
#include <array>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <algorithm>
#include <condition_variable>

class CameraControl
{

public:
    CameraControl();
    ~CameraControl();

    bool open(const int spatial_bin, const int spectral_bin, const double exp_time, const int pixelres, const bool is_gshutter);
    bool close();

    bool set_wavelength_range(double min, double max);
    bool set_exposure_time(double exp_time);

    bool start_acquisition(const std::string &base_filename_raw, const std::string &base_filename_ffc);
    bool stop_acquisition();

    /// Returns true if the acquisition thread is still running.
    bool check_stream_errors();
    bool is_streaming() const { return is_streaming_; }

    void set_ffc_enabled(bool enable) { ffc_enabled_ = enable; }
    void set_processing_threads_enabled(bool enable) { processing_threads_enabled_ = enable; }

private:
    // Camera Info variables
    AT_H h_cam_;
    AT_64 im_width_; // Sensor size
    AT_64 im_height_;
    AT_64 pix_width_; // Frame size
    AT_64 pix_height_;
    AT_64 stride_;
    bool is_open_;
    volatile std::atomic<bool> got_error_;
    volatile std::atomic<bool> is_streaming_;

    // Camera settings
    int spectral_binning_; // Spectral binning (h)
    int spatial_binning_;  // Spatial binning (w)
    float exp_time_;       // Integration time in SECONDS
    int bitspix_;
    bool global_shutter_;

    // Frame Buffer
    static constexpr const int num_buffers = 10;
    std::vector<std::unique_ptr<unsigned char[]>> framebuffers_;
    int framebuffer_size_;

    // int16 Hyper-Image Type
    typedef uint16_t hyper_dtype;
    typedef std::unique_ptr<hyper_dtype[]> hyper_frame;
    typedef std::vector<hyper_frame> hyper_image;

    // Hyper-image buffer
    std::vector<std::unique_ptr<hyper_dtype[]>> image_buffer_;

    // float32 Hyper-Image Type
    typedef float ffc_dtype;
    typedef std::unique_ptr<ffc_dtype[]> ffc_frame;
    typedef std::vector<ffc_frame> ffc_image;

    // References for FFC
    std::vector<float> white_ref_frame_;
    std::vector<float> black_ref_frame_;

    // Sturctures for hyper-image buffer
    struct ImageTask
    {
        hyper_image image;
        std::string base_filename_raw;
        std::string base_filename_ffc;
    };

    // Thread-safe queue for hyper_image buffers
    std::queue<ImageTask> buffer_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    bool acquisition_is_done_ = false;

    // Dump thread management
    std::vector<std::thread> dump_threads_;
    static constexpr int MAX_DUMP_THREADS = 2;

    // Image size in bytes
    size_t hyperframe_bytes_; // bytes for each frame scan
    size_t hyperimage_bytes_; // total bytes for all sample scans

    // Spatial padding
    static constexpr const AT_64 left_padding = 220;
    static constexpr const AT_64 right_padding = 150;

    // Spectral padding
    static constexpr const AT_64 top_pos_ = 630;
    static constexpr const AT_64 bottom_pos_ = 1580;

    // Scanned frames/lines
    static constexpr const int scan_frame_ = 2000;

    // Spectrum ROI
    double min_wv_;
    double max_wv_;

    // Flags for FFC and multi-threaded processing
    bool ffc_enabled_;
    bool processing_threads_enabled_;

    // Maximum spectral position
    static constexpr const AT_64 max_wv_pos = 950;

    static constexpr const char *WVMAP_FILE = "data/spectral_axis_roi.txt";
    std::array<double, max_wv_pos> wv_map_;

    std::thread im_acq_thread_;

    int open_camera();
    int allocate_buffers();
    void release_buffers();

    int config();
    void WaitForSensorInitialisation(AT_H H);

    void ImageAcquisitionThread(const std::string &base_filename_raw, const std::string &base_filename_ffc);
    void DumpWorker();
    void stop_dump_threads();

    std::vector<float> compute_reference_frame(const hyper_image &img);
    hyper_image flat_field_correction(const hyper_image &raw);
    bool read_wavelengths_file();
    bool save_raw_image(const std::string &base, const hyper_image &img, int dtype);
    void save_ENVI_hdr_file(const char *fname, const int nframes, const int data_type);

    void build_frame(const unsigned char *pBuf, std::unique_ptr<double[]> &out, const int width, const int height);
    void build_12bit_frame_uint16(const unsigned char *pBuf, std::unique_ptr<uint16_t[]> &out, const int width, const int height);
    void build_12bit_frame_int32(const unsigned char *pBuf, std::unique_ptr<int[]> &out, const int width, const int height);
    void build_frame_uint8(const unsigned char *pBuf, std::unique_ptr<uint8_t[]> &out, const int width, const int height);
    void build_16bit_frame_int32(unsigned char *pBuf, std::unique_ptr<int[]> &out, const int width, const int height);
    void build_16bit_frame_uint16(const unsigned char *pBuf, std::unique_ptr<uint16_t[]> &out, const int width, const int height);
};