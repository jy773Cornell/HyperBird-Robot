#include "CameraControl.hpp"

#include <iostream>
#include <chrono>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cmath>

// Copied from example Andor SDK code
#ifndef RETURN_ON_FAILURE
#define RETURN_ON_FAILURE(command)                                                                   \
    {                                                                                                \
        int result = command;                                                                        \
        if (result != AT_SUCCESS)                                                                    \
        {                                                                                            \
            std::cout << #command << " returned error code: " << std::to_string(result) << std::endl \
                      << std::flush;                                                                 \
            return result;                                                                           \
        }                                                                                            \
    }
#endif
#ifndef WARN_ON_FAILURE
#define WARN_ON_FAILURE(command)                                                                     \
    {                                                                                                \
        int result = command;                                                                        \
        if (result != AT_SUCCESS)                                                                    \
        {                                                                                            \
            std::cout << #command << " returned error code: " << std::to_string(result) << std::endl \
                      << std::flush;                                                                 \
        }                                                                                            \
    }
#endif

// Extract 2 pixels from 3 consecutive bytes in 12bit packet pixel format
constexpr int EXTRACTLOWPACKED(const unsigned char *src_ptr) { return ((src_ptr[0] << 4) + (src_ptr[1] & 0xF)); }
constexpr int EXTRACTHIGHPACKED(const unsigned char *src_ptr) { return ((src_ptr[2] << 4) + (src_ptr[1] >> 4)); }

constexpr uint16_t EXTRACTLOWPACKED_U16(const unsigned char *src_ptr) { return ((src_ptr[0] << 4) + (src_ptr[1] & 0xF)); }
constexpr uint16_t EXTRACTHIGHPACKED_U16(const unsigned char *src_ptr) { return ((src_ptr[2] << 4) + (src_ptr[1] >> 4)); }

CameraControl::CameraControl()
{
    framebuffer_size_ = 0;

    hyperframe_bytes_ = 0;
    hyperimage_bytes_ = 0;

    im_width_ = 0;
    im_height_ = 0;
    is_open_ = false;
    is_streaming_ = false;
    got_error_ = false;
    global_shutter_ = false;

    spectral_binning_ = 1;
    spatial_binning_ = 1;
    exp_time_ = 0.01;
    bitspix_ = 12;

    min_wv_ = 400;
    max_wv_ = 1000;

    wv_map_.fill(0.0);
    // Added: toggles controlled by config
    ffc_enabled_ = false;
    processing_threads_enabled_ = false;
}

CameraControl::~CameraControl()
{
}

bool CameraControl::open(const int spatial_bin, const int spectral_bin, const double exp_time, const int pixelres, const bool is_gshutter)
{
    if (is_open_)
    {
        std::cout << "The camera is already open!" << std::endl;
    }

    spatial_binning_ = spatial_bin;
    spectral_binning_ = spectral_bin;
    exp_time_ = exp_time;
    bitspix_ = pixelres;
    global_shutter_ = is_gshutter;

    if (!read_wavelengths_file())
    {
        std::cout << "[ERROR]: Could not load wavelength map file!" << std::endl;
        return false;
    }

    if (AT_InitialiseLibrary() != AT_SUCCESS)
    {
        std::cout << "[ERROR]: Library Initialization failed!" << std::endl;
        return false;
    }

    if (open_camera() != AT_SUCCESS)
    {
        std::cout << "\n[ERROR]: Could not initialize the camera." << std::endl;
        std::cout << " - Is the camera plugged and powered on?" << std::endl;
        return false;
    }
    std::cout << "[SENSOR]: Camera opened successfully!" << std::endl;

    allocate_buffers();

    // Set up dump threads only if enabled (tied to DoFFC)
    if (processing_threads_enabled_) {
        for (int i = 0; i < MAX_DUMP_THREADS; ++i)
        {
            dump_threads_.emplace_back(&CameraControl::DumpWorker, this);
            // std::cout << "[INFO]: Launched DumpWorker thread #" << i << std::endl;
        }
    } else {
        // std::cout << "[INFO]: DumpWorker threads DISABLED";
    }
    return true;
}

// Private method
int CameraControl::open_camera()
{
    int device = 0; // Camera 0 by default
    AT_WC szValue[64];

    RETURN_ON_FAILURE(AT_Open(device, &h_cam_));

    std::cout << "*\r---------------------- CAMERA INFO -----------------------*" << std::endl;
    RETURN_ON_FAILURE(AT_GetString(h_cam_, L"SerialNumber", szValue, 64));
    std::wcout << L" Serial: " << szValue << std::endl;

    RETURN_ON_FAILURE(AT_GetString(h_cam_, L"CameraModel", szValue, 64));
    std::wcout << L" Model: " << szValue << std::endl;

    RETURN_ON_FAILURE(AT_GetString(h_cam_, L"FirmwareVersion", szValue, 64));
    std::wcout << L" FPGA: " << szValue << std::endl;

    is_open_ = true;

    int ret_code = config();
    std::cout << "*---------------------------------------------------------*\n"
              << std::endl;

    return ret_code;
}

bool CameraControl::close()
{
    if (!is_open_)
    {
        std::cout << "[ERROR]: Camera is not open!" << std::endl;
        return false;
    }

    // Close camera
    if (is_streaming_)
        WARN_ON_FAILURE(AT_Command(h_cam_, L"AcquisitionStop"));
    WARN_ON_FAILURE(AT_Flush(h_cam_));
    WARN_ON_FAILURE(AT_Close(h_cam_));

    AT_FinaliseLibrary();

    release_buffers();

    // Wait for all dump threads to finish
    stop_dump_threads();

    std::cout << "[SENSOR]: Camera closed successfully" << std::endl;
    is_open_ = false;

    return true;
}

// In example, they use 10 buffers!
int CameraControl::allocate_buffers()
{
    if (!framebuffer_size_)
        return -1;

    std::cout << "[INFO]: Allocating buffers...\r" << std::flush;

    for (int i = 0; i < num_buffers; i++)
    {
        framebuffers_.push_back(std::make_unique<unsigned char[]>(framebuffer_size_));
        RETURN_ON_FAILURE(AT_QueueBuffer(h_cam_, framebuffers_.back().get(), framebuffer_size_));
    }

    // Allocate data for an hyper-image
    hyperframe_bytes_ = pix_width_ * pix_height_ * sizeof(hyper_dtype);
    image_buffer_.resize(1.2 * scan_frame_);
    for (int i = 0; i < 1.2 * scan_frame_; ++i)
    {
        image_buffer_[i] = std::make_unique<hyper_dtype[]>(pix_width_ * pix_height_);
    }

    std::cout << "[INFO]: Allocated hyper-buffers\r" << std::endl;

    return 0;
}

void CameraControl::release_buffers()
{
    for (int i = 0; i < num_buffers; i++)
    {
        framebuffers_.at(i).release();
    }
}

int CameraControl::config()
{
    AT_64 ImageSizeBytes;
    double max_fps;

    // Not implemented
    // WaitForSensorInitialisation(h_cam_);

    // Enable cooling system
    WARN_ON_FAILURE(AT_SetBool(h_cam_, L"Sensor Cooling", true));
    WARN_ON_FAILURE(AT_SetEnumString(h_cam_, L"FanSpeed", L"On"));

    WARN_ON_FAILURE(AT_SetBool(h_cam_, L"FastAOIFrameRateEnable", true));

    // Try Overlap for faster acquisition, but more unstable????
    WARN_ON_FAILURE(AT_SetBool(h_cam_, L"Overlap", true));
    // WARN_ON_FAILURE(AT_SetBool(h_cam_,L"RollingShutterGlobalClear", true)); // Does no work with Overlapping

    // Set pixel formatting and Dynamic Range of Pixel representation
    switch (bitspix_)
    {
    case 12:
        WARN_ON_FAILURE(AT_SetEnumeratedString(h_cam_, L"Pixel Encoding", L"Mono12Packed"));
        WARN_ON_FAILURE(AT_SetEnumeratedString(h_cam_, L"SimplePreAmpGainControl", L"12-bit (low noise)"));
        // WARN_ON_FAILURE(AT_SetEnumeratedString(h_cam_, L"SimplePreAmpGainControl", L"12-bit (high well capacity)"));
        break;
    case 16:
        WARN_ON_FAILURE(AT_SetEnumeratedString(h_cam_, L"Pixel Encoding", L"Mono16"));
        WARN_ON_FAILURE(AT_SetEnumeratedString(h_cam_, L"SimplePreAmpGainControl", L"16-bit (low noise & high well capacity)"));
        break;
    default:
        std::cout << "[WARNING]: Pixel depth of " + std::to_string(bitspix_) + " not supported!" << std::endl;
        return -1;
    }
    std::cout << " Pixel Depth: " << std::to_string(bitspix_) + "-bits" << std::endl;

    // Vertical/Spectral binning to 4: 2560x540x1.5 (12bit)
    WARN_ON_FAILURE(AT_SetInt(h_cam_, L"AOIVBin", spectral_binning_));

    // Horizontal binning to 2 1280x540x1.5
    WARN_ON_FAILURE(AT_SetInt(h_cam_, L"AOIHBin", spatial_binning_));

    // Set horizontal ROI
    WARN_ON_FAILURE(AT_GetInt(h_cam_, L"AOIWidth", &pix_width_));
    pix_width_ = pix_width_ - left_padding / spatial_binning_ - right_padding / spatial_binning_;
    WARN_ON_FAILURE(AT_SetInt(h_cam_, L"AOIWidth", pix_width_));

    // NOTE: Even when using binning, padding does not need to be adjusted
    if (left_padding)
        WARN_ON_FAILURE(AT_SetInt(h_cam_, L"AOILeft", left_padding));

    // Set spectral ROI determined by spectral range
    // 1- Determine starting and ending positions in spectral axis
    // TODO: Support spectral binning!
    // top_pos_ = -1;
    // bottom_pos_ = -1;
    // for (long long i = 0; i < max_wv_pos; i++)
    // {
    //     if (wv_map_[i] >= min_wv_ && top_pos_ == -1)
    //         top_pos_ = i;
    //     if (wv_map_[i] >= max_wv_ && bottom_pos_ == -1)
    //         bottom_pos_ = i;
    // }

    // Scale positions regarging binning config
    WARN_ON_FAILURE(AT_SetInt(h_cam_, L"AOIHeight", (AT_64)std::round(((double)(bottom_pos_ - top_pos_)) / spectral_binning_)));
    WARN_ON_FAILURE(AT_SetInt(h_cam_, L"AOITop", top_pos_));

    std::cout << " Spectral range: " << std::to_string((int)min_wv_) << "nm to " << std::to_string((int)max_wv_)
              << "nm -> Table pos: [" << std::to_string(top_pos_) + ", " + std::to_string(bottom_pos_) << "]" << std::endl;

    // Configure external trigger
    // WARN_ON_FAILURE(AT_SetEnumString(h_cam, L"TriggerMode", L"Software"));
    WARN_ON_FAILURE(AT_SetEnumString(h_cam_, L"TriggerMode", L"Internal"));
    WARN_ON_FAILURE(AT_SetEnumString(h_cam_, L"CycleMode", L"Continuous"));

    // Set shuttering mode
    if (global_shutter_)
    {
        WARN_ON_FAILURE(AT_SetEnumString(h_cam_, L"ElectronicShutteringMode", L"Global"));
    }
    else
    {
        WARN_ON_FAILURE(AT_SetEnumString(h_cam_, L"ElectronicShutteringMode", L"Rolling"));
    }

    // Set Readout rate, 100Hz by default
    WARN_ON_FAILURE(AT_SetEnumeratedString(h_cam_, L"Pixel Readout Rate", L"280 MHz"));

    // Set the exposure time for this camera (FPS is affected)
    WARN_ON_FAILURE(AT_SetFloat(h_cam_, L"Exposure Time", exp_time_));

    // Validate exp time change
    double exp_curr;
    WARN_ON_FAILURE(AT_GetFloat(h_cam_, L"Exposure Time", &exp_curr));
    std::cout << " Current Exposition: " << std::to_string(exp_curr) << std::endl;

    // NOTE: Max FPS depends on Exposure Time!
    WARN_ON_FAILURE(AT_GetFloatMax(h_cam_, L"FrameRate", &max_fps));
    std::cout << " Max. FPS: " << std::to_string(max_fps) << std::endl;

    // NOTE: FPS needs to be manually set, otherwise unexpected frame rate behaviour will happen
    WARN_ON_FAILURE(AT_SetFloat(h_cam_, L"FrameRate", max_fps)); // set maximum FPS as current FPS

    // WARN_ON_FAILURE(AT_GetFloatMax(h_cam_, L"MaxInterfaceTransferRate", &max_fps));
    // std::cout << "Max. Interface Transfer rate (FPS): " << std::to_string(max_fps) << std::endl;

    // WARN_ON_FAILURE(AT_GetInt(h_cam_, L"AccumulateCount", &acc_count));
    // std::cout << "Accumulate Count: " << std::to_string(acc_count) << std::endl;

    // Get the number of bytes required to store one frame
    WARN_ON_FAILURE(AT_GetInt(h_cam_, L"Image Size Bytes", &ImageSizeBytes));
    framebuffer_size_ = (int)ImageSizeBytes;

    // Obtain Image size
    WARN_ON_FAILURE(AT_GetInt(h_cam_, L"Sensor Width", &im_width_));
    WARN_ON_FAILURE(AT_GetInt(h_cam_, L"Sensor Height", &im_height_));

    WARN_ON_FAILURE(AT_GetInt(h_cam_, L"AOIStride", &stride_));
    WARN_ON_FAILURE(AT_GetInt(h_cam_, L"AOIWidth", &pix_width_));
    WARN_ON_FAILURE(AT_GetInt(h_cam_, L"AOIHeight", &pix_height_));

    std::cout << " Sensor size: " << std::to_string(im_width_) + "x" + std::to_string(im_height_) << std::endl;
    std::cout << " Frame shape: " << std::to_string(pix_width_) + "x" + std::to_string(pix_height_) << "- stride " << std::to_string(stride_) << " bytes" << std::endl;
    std::cout << " Framebuffer size: " + std::to_string(framebuffer_size_) + " bytes" << std::endl;

    return AT_SUCCESS;
}

bool CameraControl::read_wavelengths_file()
{
    std::string line;
    std::ifstream infile(WVMAP_FILE);
    double wv;
    int i = 0;

    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        if (!(iss >> wv))
        {
            break;
        } // error
        wv_map_[i] = wv;
        i++;
    }
    if (!i)
    {
        std::cout << "[ERROR]: Wavelength map file not found!" << std::endl;
        return false;
    }
    if (i != max_wv_pos)
    {
        std::cout << "[WARNING]: Wavelengths map file contains diffent number of entries than expected!" << std::endl;
        return false;
    }
    return true;
}

// Not implemented in Zyla 5.5, not used here for now
void CameraControl::WaitForSensorInitialisation(AT_H H)
{
    AT_BOOL initialisedImplemented = AT_FALSE;
    AT_IsImplemented(H, L"SensorInitialised", &initialisedImplemented);
    if (initialisedImplemented == AT_TRUE)
    {
        AT_BOOL initialised = AT_FALSE;
        AT_GetBool(H, L"SensorInitialised", &initialised);
        if (initialised == AT_FALSE)
        {
            std::cout << "Waiting for sensor initialisation ";
            do
            {
                AT_GetBool(H, L"SensorInitialised", &initialised);
                std::cout << ".";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } while (initialised == AT_FALSE);
            std::cout << std::endl;
        }
    }
    else
        std::cout << "[INFO]: SensorInitialised  not implemented!" << std::endl;
}

bool CameraControl::set_wavelength_range(double min, double max)
{
    if (is_open_)
    {
        std::cout << "[ERROR]: Could not change wavelength range once camera has been opened! " << std::endl;
        return false;
    }

    if (min >= max || min < 0)
    {
        std::cout << "[ERROR]: Wavelength range not supported!" << std::endl;
        return false;
    }

    min_wv_ = min;
    max_wv_ = max;

    return true;
}

bool CameraControl::set_exposure_time(const double exp_time)
{
    double exp_max, exp_min, max_fps;

    double exp_curr;
    WARN_ON_FAILURE(AT_GetFloat(h_cam_, L"Exposure Time", &exp_curr));
    std::cout << "Current Exposition: " << std::to_string(exp_curr) << std::endl;

    // Set the exposure time for this camera to 20 milliseconds (FPS is affected)
    if (AT_SetFloat(h_cam_, L"Exposure Time", exp_time) != AT_SUCCESS)
    {
        std::cout << "[ERROR]: Could not set exposure time to " << std::to_string(exp_time) << std::endl;
        WARN_ON_FAILURE(AT_GetFloatMax(h_cam_, L"Exposure Time", &exp_max));
        WARN_ON_FAILURE(AT_GetFloatMin(h_cam_, L"Exposure Time", &exp_min));
        std::cout << "[INFO]: Exposure time range = [" << std::to_string(exp_min) << " to " << std::to_string(exp_max) << "]" << std::endl;
        return false;
    }
    exp_time_ = exp_time;

    // NOTE: Max FPS depends on Exposure Time!
    WARN_ON_FAILURE(AT_GetFloatMax(h_cam_, L"FrameRate", &max_fps));
    std::cout << "Exposure time set to " << std::to_string(exp_time);
    std::cout << " (Max. FPS: " << std::to_string(max_fps) << ")" << std::endl;

    return true;
}

bool CameraControl::start_acquisition(const std::string &base_filename_raw, const std::string &base_filename_ffc)
{
    int ret_code;
    ret_code = AT_Command(h_cam_, L"AcquisitionStart");
    if (ret_code != AT_SUCCESS)
    {
        std::cout << "[ERROR]: Start acquisition returned error code: " << std::to_string(ret_code) << std::endl;
        return false;
    }
    is_streaming_ = true;
    got_error_ = false;

    // start thread
    im_acq_thread_ = std::thread{&CameraControl::ImageAcquisitionThread, this, base_filename_raw, base_filename_ffc};
    return true;
}

bool CameraControl::stop_acquisition()
{
    int ret_code;

    is_streaming_ = false;
    im_acq_thread_.join(); // Join thread

    ret_code = AT_Command(h_cam_, L"AcquisitionStop");
    // finish and join thread if active
    if (ret_code != AT_SUCCESS)
    {
        std::cout << "[ERROR]: Stop acquisition returned error code: " << std::to_string(ret_code) << std::endl;
        return false;
    }

    return true;
}

//-------------------- IMAGE ACQUISITION THREAD --------------------------------
void CameraControl::ImageAcquisitionThread(const std::string &base_filename_raw, const std::string &base_filename_ffc)
{
    std::ofstream imfile;
    unsigned char *src_imdata;
    int recv_bytes;
    int ret_code;

    // Only for measuring FPS
    int n_frames = 0;
    int total_frames = 0;
    int min_fps, max_fps, current_fps;
    max_fps = 0;
    min_fps = 999;
    current_fps = 0;

    auto start = std::chrono::steady_clock::now();
    auto start2 = std::chrono::steady_clock::now();

    while (total_frames < scan_frame_)
    {
        is_streaming_ = true;
        ret_code = AT_WaitBuffer(h_cam_, &src_imdata, &recv_bytes, 1000000); // Timeout in example is 10s instead of 1s
        if (ret_code != AT_SUCCESS)
        {
            got_error_ = true;
            std::cout << "[ERROR]: Error acquiring frame: " << std::to_string(ret_code) << std::endl; // 13 = Timeout
            auto end = std::chrono::steady_clock::now();
            std::cout << "Time alive: " << std::to_string(std::chrono::duration_cast<std::chrono::seconds>(end - start2).count()) << "s\n"
                      << std::flush;
            break;
        }

        // Check if there is enough memory for one more frame, break otherwise
        if (total_frames >= (int)image_buffer_.size())
        {
            std::cout << "[WARNING]: Not enough memory for more hyper-frames. Skipping frames..." << std::endl;
            break;
        }

        if (bitspix_ == 12)
            build_12bit_frame_uint16(src_imdata, image_buffer_.at(total_frames), pix_width_, pix_height_);
        else
            build_16bit_frame_uint16(src_imdata, image_buffer_.at(total_frames), pix_width_, pix_height_);

        total_frames++;
        n_frames++;

        // Reallocate the frame buffer
        WARN_ON_FAILURE(AT_QueueBuffer(h_cam_, src_imdata, framebuffer_size_));

        auto end = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() > 1000)
        {
            if (n_frames > max_fps)
                max_fps = n_frames;
            if (n_frames < min_fps)
                min_fps = n_frames;
            current_fps = n_frames;

            std::cout << "\rFramerate: " << n_frames << "FPS; Progress: " << total_frames << "|" << scan_frame_ << std::flush;
            start = std::chrono::steady_clock::now();
            n_frames = 0;
        }
    }

    is_streaming_ = false;

    if (processing_threads_enabled_) {
        // Push to queue for multi-threaded dump/FFC
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            buffer_queue_.push({std::move(image_buffer_), base_filename_raw, base_filename_ffc});
            queue_cv_.notify_one();
        }
    } else {
        // Synchronous, single-threaded save of RAW only (no FFC)
        if (!save_raw_image(base_filename_raw, image_buffer_, 12)) {
            std::cerr << "[ERROR]: Failed to save raw image (single-thread path)";
        } else {
            std::cout << "[SAVE]: Saved " << base_filename_raw << ".raw (single-thread)";
        }
    }

    // Reallocate the image_buffer_
    image_buffer_.resize(scan_frame_);
    for (int i = 0; i < scan_frame_; ++i)
    {
        image_buffer_[i] = std::make_unique<hyper_dtype[]>(pix_width_ * pix_height_);
    }

    std::cout << "\r[SCANNING]: Sample Scanning Completed. FPS. Min: " << current_fps
              << ", Max: " << current_fps << std::endl;
}

//-------------------- Dump Worker THREAD --------------------------------
std::string to_lower(const std::string &str)
{
    std::string lower;
    lower.reserve(str.size());
    for (char c : str)
    {
        lower += std::tolower(static_cast<unsigned char>(c));
    }
    return lower;
}

void CameraControl::DumpWorker()
{
    while (true)
    {
        ImageTask task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [&]()
                           { return !buffer_queue_.empty() || acquisition_is_done_; });

            if (buffer_queue_.empty() && acquisition_is_done_)
                break;

            task = std::move(buffer_queue_.front());
            buffer_queue_.pop();
        }

        // auto start = std::chrono::steady_clock::now();

        // save raw image
        std::filesystem::path raw_path(task.base_filename_raw);
        std::string filename = raw_path.filename().string();
        std::string &base_raw = task.base_filename_raw;
        if (!save_raw_image(base_raw, task.image, 12))
        {
            continue;
        }
        std::cout << "[DUMP]: Saved " << base_raw << ".raw" << std::endl;

        // Detect and compute reference frame
        if (to_lower(filename).find("white") != std::string::npos)
        {
            white_ref_frame_ = compute_reference_frame(task.image);
            std::cout << "[INFO]: Updated white reference frame from: " << base_raw << std::endl;
        }
        else if (to_lower(filename).find("black") != std::string::npos)
        {
            black_ref_frame_ = compute_reference_frame(task.image);
            std::cout << "[INFO]: Updated black reference frame from: " << base_raw << std::endl;
        }
        else
        {
            if (ffc_enabled_) {
            // Apply FFC
            auto corrected = flat_field_correction(task.image);
            if (corrected.empty())
            {
                std::cerr << "[WARNING]: FFC skipped due to invalid reference frames for " << base_raw << "";
                continue;
            }

            // Save corrected image
            const std::string &base_ffc = task.base_filename_ffc;
            if (!save_raw_image(base_ffc, corrected, 12))
            {
                continue;
            }
            std::cout << "[DUMP]: Saved corrected (FFC) image: " << base_ffc << ".raw";
        } else {
            // FFC disabled: nothing else to do
        }}

        // auto end = std::chrono::steady_clock::now();
        // using seconds_f = std::chrono::duration<double>; // double-precision seconds
        // double seconds = std::chrono::duration_cast<seconds_f>(end - start).count();
        // std::cout << "\nDUMP RUNNING TIME: " << seconds << " seconds\n";
    }

    std::cout << "[THREAD]: DumpWorker exiting\n";
}

void CameraControl::stop_dump_threads()
{
    if (!processing_threads_enabled_) return;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        acquisition_is_done_ = true;
    }
    queue_cv_.notify_all(); // Wake up all dump threads

    for (auto &t : dump_threads_)
    {
        if (t.joinable())
            t.join();
    }
    dump_threads_.clear();

    std::cout << "[INFO]: All DumpWorker threads joined and cleared.\n";
}

// Compute the reference frame for flat field correction
std::vector<float> CameraControl::compute_reference_frame(const hyper_image &img)
{
    int npixels = pix_width_ * pix_height_;
    std::vector<float> avg_frame(npixels, 0.0f);

    for (int i = 0; i < scan_frame_; ++i)
    {
        for (int j = 0; j < npixels; ++j)
        {
            avg_frame[j] += static_cast<float>(img[i][j]);
        }
    }

    for (float &val : avg_frame)
        val /= static_cast<float>(scan_frame_);

    return avg_frame;
}
// Flat Field Correction (stored as scaled int16)
CameraControl::hyper_image CameraControl::flat_field_correction(const hyper_image &raw)
{
    const size_t expected_size = pix_width_ * pix_height_;

    // Sanity checks
    if (black_ref_frame_.empty() || white_ref_frame_.empty())
    {
        std::cerr << "[WARNING]: Reference frames are not initialized.\n";
        return {};
    }

    if (black_ref_frame_.size() != expected_size || white_ref_frame_.size() != expected_size)
    {
        std::cerr << "[WARNING]: Reference frame dimensions do not match image frame size.\n";
        return {};
    }

    hyper_image corrected(scan_frame_);

    for (int i = 0; i < scan_frame_; ++i)
    {
        corrected[i] = std::make_unique<hyper_dtype[]>(pix_width_ * pix_height_);
        for (int j = 0; j < pix_width_ * pix_height_; ++j)
        {
            float I_raw = static_cast<float>(raw[i][j]);
            float I_dark = black_ref_frame_[j];
            float I_white = white_ref_frame_[j];

            float denom = I_white - I_dark;
            if (denom < 1.0f)
                denom = 1.0f; // avoid divide by near-zero

            float I_corr = (I_raw - I_dark) / denom;
            I_corr = std::clamp(I_corr, 0.0f, 1.0f); // normalized reflectance

            // Scale to uint16
            corrected[i][j] = static_cast<hyper_dtype>(std::round(I_corr * 65535.0f));
        }
    }

    return corrected;
}

// Save raw image
bool CameraControl::save_raw_image(const std::string &base, const hyper_image &img, int dtype)
{
    // Ensure directory has enough space
    auto dir = base.substr(0, base.find_last_of('/') + 1);
    std::filesystem::space_info si = std::filesystem::space(dir);
    uintmax_t needed = static_cast<uintmax_t>(scan_frame_) * static_cast<uintmax_t>(hyperframe_bytes_);
    if (si.available < needed)
    {
        std::cerr << "[ERROR]: Not enough free disk space at " << dir << "!\n";
        return false;
    }

    // Write binary .raw
    std::ofstream out(base + ".raw", std::ios::binary);
    for (int i = 0; i < scan_frame_; ++i)
        out.write(reinterpret_cast<const char *>(img[i].get()), hyperframe_bytes_);
    out.close();

    // Write .hdr
    save_ENVI_hdr_file((base + ".hdr").c_str(), scan_frame_, dtype);

    return true;
}

// ENVI Save Function
void CameraControl::save_ENVI_hdr_file(const char *fname, const int nframes, const int data_type)
{
    std::ofstream iminfo;
    iminfo.open(fname);
    iminfo << "ENVI" << std::endl;
    iminfo << "camera model = " << "ZYLA5.5USB33J1" << std::endl;
    iminfo << "serial number = " << "VSC-14736" << std::endl;
    iminfo << "tint = " << std::to_string(exp_time_) << std::endl;
    iminfo << "trigger mode = " << "Internal" << std::endl;
    iminfo << "roi left = " << std::to_string(left_padding) << std::endl;
    iminfo << "roi width = " << std::to_string(im_width_ - right_padding - left_padding) << std::endl;
    iminfo << "roi top = " << std::to_string(top_pos_) << std::endl;
    iminfo << "roi height = " << std::to_string(bottom_pos_ - top_pos_) << std::endl;
    iminfo << "hbin = " << "x" + std::to_string(spatial_binning_) << std::endl;
    iminfo << "vbin = " << "x" + std::to_string(spectral_binning_) << std::endl;

    if (bitspix_ == 12)
        iminfo << "gain mode = " << "12-bit (low noise)" << std::endl;
    else
        iminfo << "gain mode = " << "16-bit (low noise & high well capacity)" << std::endl;

    if (global_shutter_)
        iminfo << "shutter mode = " << "Global" << std::endl;
    else
        iminfo << "shutter mode = " << "Rolling" << std::endl;

    iminfo << "interleave = bip" << std::endl;
    iminfo << "samples = " << std::to_string(pix_width_) << std::endl;
    iminfo << "lines = " << std::to_string(nframes) << std::endl;
    iminfo << "bands = " << std::to_string(pix_height_) << std::endl;
    iminfo << "data type = " << std::to_string(data_type) << std::endl;
    iminfo << "byte order = 0" << std::endl;
    iminfo << "header offset = 0" << std::endl
           << std::endl;

    iminfo << "wlcal units = nm" << std::endl;
    iminfo << "Wavelength = {" << std::endl;
    for (long long i = 0; i < pix_height_; i++)
    {
        iminfo << std::to_string(wv_map_[(i * spectral_binning_)]);
        if (i < pix_height_ - 1)
            iminfo << ',' << std::endl;
    }
    iminfo << "\n}" << std::endl
           << std::endl;

    iminfo << "fwhm = {" << std::endl;
    for (long long i = 0; i < pix_height_; i++)
    {
        // CAUTION: index can get out of bounds if index => im_height_  !!!!!!!!!!!!
        iminfo << std::to_string(wv_map_[((i + 1) * spectral_binning_)] - wv_map_[(i * spectral_binning_)]);
        if (i < pix_height_ - 1)
            iminfo << ',' << std::endl;
    }
    iminfo << "\n}" << std::endl;

    iminfo.close();
}

bool CameraControl::check_stream_errors()
{
    return got_error_;
}

// TODO: In order to save space, it might be better so save values as int32 instead of double64!
void CameraControl::build_frame(const unsigned char *pBuf, std::unique_ptr<double[]> &out, const int width, const int height)
{
    int LowPixel, HighPixel;
    const unsigned char *__restrict__ ptr = pBuf;
    double *__restrict__ outp = out.get();

    for (int j = 0; j < height; j++)
    {
        // std::cout << std::to_string(j) << ": ";
        for (int i = 0; i < width; i += 2)
        {
            LowPixel = EXTRACTLOWPACKED(ptr);
            HighPixel = EXTRACTHIGHPACKED(ptr);
            ptr += 3;

            // std::cout << std::to_string(i) << " " << std::flush;

            outp[(i * height) + j] = (double)LowPixel / 4095;
            outp[((i + 1) * height) + j] = (double)HighPixel / 4095;
        }
    }
}

// TODO: In order to save space, it might be better so save values as int32 instead of double64!
void CameraControl::build_12bit_frame_int32(const unsigned char *pBuf, std::unique_ptr<int[]> &out, const int width, const int height)
{
    int LowPixel, HighPixel;
    const unsigned char *__restrict__ ptr = pBuf;
    int *__restrict__ outp = out.get();

    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i += 2)
        {
            LowPixel = EXTRACTLOWPACKED(ptr);
            HighPixel = EXTRACTHIGHPACKED(ptr);
            ptr += 3;

            outp[(i * height) + j] = LowPixel;
            outp[((i + 1) * height) + j] = HighPixel;
        }
    }
}

void CameraControl::build_16bit_frame_int32(unsigned char *pBuf, std::unique_ptr<int[]> &out, const int width, const int height)
{
    unsigned char *__restrict__ ptr = pBuf;
    int *__restrict__ outp = out.get();

    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            outp[(i * height) + j] = (int)*reinterpret_cast<uint16_t *>(ptr);
            ptr += 2;
        }
    }
}

void CameraControl::build_12bit_frame_uint16(const unsigned char *pBuf, std::unique_ptr<uint16_t[]> &out, const int width, const int height)
{
    uint16_t LowPixel, HighPixel;
    const unsigned char *__restrict__ ptr = pBuf;
    uint16_t *__restrict__ outp = out.get();

    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i += 2)
        {
            LowPixel = EXTRACTLOWPACKED_U16(ptr);
            HighPixel = EXTRACTHIGHPACKED_U16(ptr);
            ptr += 3;

            outp[(i * height) + j] = LowPixel;
            outp[((i + 1) * height) + j] = HighPixel;
        }
    }
}

void CameraControl::build_16bit_frame_uint16(const unsigned char *pBuf, std::unique_ptr<uint16_t[]> &out, const int width, const int height)
{
    const unsigned char *__restrict__ ptr = pBuf;
    uint16_t *__restrict__ outp = out.get();

    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            outp[(i * height) + j] = *const_cast<uint16_t *>(reinterpret_cast<const uint16_t *>(ptr));
            ptr += 2;
        }
    }
}

void CameraControl::build_frame_uint8(const unsigned char *pBuf, std::unique_ptr<uint8_t[]> &out, const int width, const int height)
{
    int LowPixel, HighPixel;
    const unsigned char *__restrict__ ptr = pBuf;
    uint8_t *__restrict__ outp = out.get();

    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i += 2)
        {
            LowPixel = EXTRACTLOWPACKED(ptr);
            HighPixel = EXTRACTHIGHPACKED(ptr);
            ptr += 3;

            outp[(i * height) + j] = (uint8_t)((LowPixel * 255) / 4095);
            outp[((i + 1) * height) + j] = (uint8_t)((HighPixel * 255) / 4095);
        }
    }
}