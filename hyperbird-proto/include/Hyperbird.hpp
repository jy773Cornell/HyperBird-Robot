#pragma once

#include <ctime>
#include <csignal>
#include <vector>

#include <xlnt/xlnt.hpp>
#include "MotionControl.hpp"
#include "CameraControl.hpp"

class Hyperbird
{
private:
    // Hyperbird Interface Version, Provisional
    static constexpr const int MAJOR = 2;
    static constexpr const int MINOR = 1;
    static constexpr const int PATCH = 0;

    // Constant information
    static constexpr const int MAX_SAMPLES = 351;
    static constexpr const int TRAY_GRID_W = 20;
    static constexpr const int TRAY_GRID_H = 18;
    static constexpr const double scan_dist = 13.0;

    static volatile std::sig_atomic_t finish;

    // Data structure to store the sample tray data from its Excel file
    struct TrayInfo
    {
        std::string exp_name;
        std::string tray_name;
        std::tm innoc_date;
        std::vector<std::string> slabels;
        int nsamples;
        std::string date_str;
    };

    // Data structure to store scan configuration
    struct ScanConfig
    {
        // Camera settings
        int spectral_binning = 1; // Spectral binning (h)
        int spatial_binning = 1;  // Spatial binning (w)
        float exp_time = 0.01;    // Integration time in SECONDS
        bool global_shutter = 0;  // 0 - Rolling, 1- Global
        int pixelres = 12;        // Pixel Resolution: 12bits or 16bits

        // Motion settings
        int scan_speed = 30; // Must be integer (mm/min)
        double zpos = 12.0;  // Z position in which all samples will be imaged

        // Misc. settings
        double lens_aperture = 5.6; // Not usable, informative only

        bool do_ffc = false; // Whether to perform FFC or not
    };

    ScanConfig config_;

    std::vector<std::string>::iterator sample_it;

    MotionControl motion_;
    CameraControl camera_;
    bool connected_;

    TrayInfo tray_info;
    std::string root_data_dir1_; // For original raw data
    std::string root_data_dir2_; // For FFC data

    int current_sample_;

    // NOTE: This function should have less code as possible. Use flags to finish other threads/tasks
    static void interrupt_funct(int s)
    {
        std::cout << "\n Interruption detected! Stopping... (Wait until current action is done)" << std::endl;
        Hyperbird::finish = s;
    };

    static const std::string bold(std::string str) { return ("\e[1m" + str + "\e[0m").c_str(); }
    static const std::string red(std::string str) { return ("\033[1;31m" + str + "\033[0m").c_str(); }

    // store any scan‐dump threads
    std::vector<std::thread> dump_threads_;

public:
    Hyperbird();
    ~Hyperbird();

    bool read_config(const std::string cfg_file, const std::string root_dir, double z_pos);

    bool connect(const char *port_name);
    bool close();
    bool is_interrupted();

    // Data management methods:
    bool read_excel_file(const std::string path);
    void print_tray_info();

    // Motion related methods:
    bool get_position(double &xpos, double &ypos, double &zpos);
    bool goto_first_sample();
    bool goto_sample(const int nsample);
    bool goto_next_sample();
    bool goto_end_pos();
    void set_Zpos(double zpos);
    double get_Zpos();

    // Camera related methods
    bool scan_current_sample();
};