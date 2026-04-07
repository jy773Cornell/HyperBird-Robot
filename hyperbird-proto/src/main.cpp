#include "Hyperbird.hpp"
#include <iostream>

// Name of serial port device
static constexpr const char *port_name = "/dev/ttyACM0";

void imaging_process(Hyperbird &robot)
{
    // double xpos,ypos,zpos;
    bool is_finished = false;

    is_finished = !robot.goto_first_sample();

    while (!robot.is_interrupted() && !is_finished)
    {

        robot.scan_current_sample();
        if (robot.is_interrupted())
            break;

        is_finished = !robot.goto_next_sample();
    }

    robot.goto_end_pos();

    return;
}

int main(int argc, char *argv[])
{
    auto start = std::chrono::steady_clock::now();

    Hyperbird robot;

    std::string excel_file_path, config_file_path, root_dir;
    double z_pos = 0.0;
    std::string answ;

    // Check program inputs
    if (argc < 3 || argc > 5)
    {
        std::cout << "Usage: program <excel_file> <config_file> [RootDir] [Zpos]" << std::endl;
        return -1;
    }

    excel_file_path.assign(argv[1]);
    config_file_path.assign(argv[2]);
    
   // Optional arguments
    if (argc >= 4)
        root_dir.assign(argv[3]);

    if (argc == 5)
        z_pos = std::stod(argv[4]);

    // Attempt to load the provided Excel file
    if (!robot.read_excel_file(excel_file_path))
    {
        std::cout << "Could not load experiment info!" << std::endl;
        return -1;
    }

    // Make terminal 130 chars wide, 35 rows
    std::cout << "\e[8;35;135t";

    // Show tray information to verify
    robot.print_tray_info();

    // Ask the user whether he/she wants to proceed with imaging this tray.
    std::cout << "- Proceed to start imaging this tray? (Y/N) ";
    std::cin >> answ;
    std::cout << std::endl;

    // Check if user response was other than 'Y'; then finish program, continue otherwise.
    if (answ.compare("Y") != 0 && answ.compare("y") != 0)
    {
        std::cout << "Finishing program..." << std::endl;
        return 0;
    }

    // Attempt to load the provided Excel file
    if (!robot.read_config(config_file_path, root_dir, z_pos))
    {
        std::cout << "Loading configuration file was not sucessful!" << std::endl;
        return -1;
    }

    // Try to connect and initialize the Hyperbird robot
    if (!robot.connect(port_name))
    {
        // If it was not possible, stop program.
        return -1;
    }

    imaging_process(robot);

    // Disconnect and "close" the communication with the robot before finishing the program.
    robot.close();

    auto end = std::chrono::steady_clock::now();
    using minutes_f = std::chrono::duration<double, std::ratio<60>>;
    double minutes = std::chrono::duration_cast<minutes_f>(end - start).count();
    std::cout << "\nTOTAL RUNNING TIME: " << minutes << " minutes\n";
    std::cout << "\n *** You can now retrieve the tray ***\n"
              << std::endl;

    return 0;
}
