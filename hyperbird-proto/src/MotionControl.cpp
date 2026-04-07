#include "MotionControl.hpp"
#include <chrono>
#include <thread>

MotionControl::MotionControl()
{
    // Hardcoded for now
    max_posx_ = 0.0;
    max_posy_ = 0.0;
    max_posz_ = 0.0;
}

MotionControl::~MotionControl()
{
}

bool MotionControl::init(const char *port_path, int baudrate)
{
    std::string indata;

    try
    {
        port_.SetDevice(port_path);
        port_.SetBaudRate(baudrate);

        // Set read time out
        // -1 -> Block when reading until any data is received
        // 0  -> Non-blocking read,  does not throw exception if no data is read
        // 0 >= time in ms, timeout, does not throw exception if no data is read
        port_.SetTimeout(1000);
        // Try to open port
        port_.Open();
    }
    catch (Exception &e)
    {
        std::cerr << "Could not open serial port!" << e.what() << std::endl;
        return false;
    }

    // Wait until receiving MCB connection start data, or timeout
    /*auto start = std::chrono::high_resolution_clock::now();
    while(!port_.Available())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        auto end = std::chrono::high_resolution_clock::now();
        if(std::chrono::duration_cast<std::chrono::seconds>(end - start).count() > 5)
        {
            std::cerr << "[ERROR]: Timeout while trying to receive first data!" << std::endl;
            return false;
        }
    }

    // Once first data is received, print everything until it's done
    while(port_.Available())
    {
        port_.ReadLine(indata);
        #ifdef SERIALPORT_DEBUG
        std::cout << "READ: " << indata << std::endl;
        #endif
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }*/

    // set_steps_mm_Y(80);

    return true;
}

void MotionControl::close()
{
    try
    {
        port_.Close();
    }
    catch (Exception &e)
    {
        std::cerr << "Could not close serial port!" << e.what() << std::endl;
    }
}

bool MotionControl::find_zero(bool force)
{
    std::string cmd;
    try
    {
        if (force)
            cmd.assign("G28\n");
        else
            cmd.assign("G28 O\n");

        port_.Write(cmd);
#ifdef SERIALPORT_DEBUG
        std::cout << "WRITE: " << cmd;
#endif
    }
    catch (Exception &e)
    {
        std::cerr << "[ERROR]: Could not write on serial port!" << e.what() << std::endl;
        return false;
    }

    // ok = wait_ok(); // Can be deleted?
    bool ok = wait_done(40.0);

    // BE careful! When G28 is not done, Z limit is not correctly assigned,
    if (force)
    {
        get_motor_pos(&X_, &Y_, &Z_);
        // RCLCPP_INFO(this->get_logger(), "Z-end position is: %f",Z);
        // RCLCPP_INFO(this->get_logger(), "Z focal scan bounds: [MinZ: %f, MaxZ: %f]",Zmin,Zmax);
    }

    return ok;
}

bool MotionControl::move_to(double x, double y, double z, int f, bool rel, float to)
{
    std::string cmd = "G1";
    double fz = 0.0;

    if (f < 0)
        return false;

    // Chech whether 999 is the best "skip" number
    if (x != 999)
    {
        if (rel)
            cmd = cmd + " X" + std::to_string(x + X_);
        else
            cmd = cmd + " X" + std::to_string(x);
    }
    if (y != 999)
    {
        if (rel)
            cmd = cmd + " Y" + std::to_string(y + Y_);
        else
            cmd = cmd + " Y" + std::to_string(y);
    }
    if (z != 999)
    {
        // Make sure Z does not go less than min_
        if (rel)
        {
            if ((z + Z_) < min_posz)
            {
                fz = min_posz;
                std::cout << "[WARNING]: Z cannot go lower than: " << std::to_string(min_posz) << std::endl;
            }
            else
                fz = z + Z_;
        }
        else
        {
            if (z < min_posz)
            {
                fz = min_posz;
                std::cout << "[WARNING]: Z cannot go lower than: " << std::to_string(min_posz) << std::endl;
            }
            else
                fz = z;
        }

        cmd = cmd + " Z" + std::to_string(fz);
    }
    cmd = cmd + " F" + std::to_string(f) + "\n";

    try
    {
        port_.Write(cmd);
#ifdef SERIALPORT_DEBUG
        std::cout << "WRITE: " << cmd;
#endif
    }
    catch (Exception &e)
    {
        std::cerr << "[ERROR]: Could not write on serial port!" << e.what() << std::endl;
        return false;
    }

    // NOTE: Should it update this after wait_done?
    if (rel)
    {
        if (x != 999)
            X_ = X_ + x;
        if (y != 999)
            Y_ = Y_ + y;
    }
    else
    {
        if (x != 999)
            X_ = x;
        if (y != 999)
            Y_ = y;
        if (z != 999)
            Z_ = z;
    }

    if (z != 999)
        Z_ = fz;

    if (to)
        return wait_done(to);
    else
        return true;
}

bool MotionControl::move_to_sample(int ns, double avgZ, double offX, double offY)
{
    uint32_t x, y;
    double xt, yt, offX_tray;

    offX_tray = 0.0;
    x = ns;
    y = 1;
    uint8_t cells_in_row = w_sgrid;
    while (x > cells_in_row)
    {
        x -= cells_in_row;
        y++;
        if (y % 2)
            cells_in_row = w_sgrid;
        else
            cells_in_row = (w_sgrid - 1);
    }
    if (!(y % 2))
    {
        x = cells_in_row - x + 1;
        offX_tray = cell_sx / 2;
    }

    offX = -(0.5 * ((double)y - 0)) / (double)h_sgrid;

    // std::cout << "Sample " << std::to_string(ns) << " is at " << std::to_string(x) << ", " << std::to_string(y) << std::endl;

    xt = first_x - ((double)(x - 1) * cell_sx) + offX - offX_tray; // X goes down
    yt = first_y + ((double)(y - 1) * cell_sy) + offY;             // Y goes up

    std::cout << "[SCANNING]: Going to sample " << std::to_string(ns) << " at position [X: " << std::to_string(xt) << ", Y:";
    std::cout << std::to_string(yt) << ", Z: " << std::to_string(avgZ) << "]" << std::endl;

    return move_to(xt, yt, avgZ, 2500, false, 15.0f);
}

bool MotionControl::get_motor_pos(double *xpos, double *ypos, double *zpos)
{
    std::string str;
    size_t xi, yi, zi, ei;
    xi = 2;

    std::string cmd = "M114\n";

    try
    {
        port_.Write(cmd);
#ifdef SERIALPORT_DEBUG
        std::cout << "WRITE: " << cmd;
#endif
    }
    catch (Exception &e)
    {
        std::cerr << "[ERROR]: Could not write on serial port!" << e.what() << std::endl;
        return false;
    }

    port_.ReadLine(str);

    if (!str.length())
        return false;

#ifdef SERIALPORT_DEBUG
    std::cout << "READ: " << str << std::endl;
#endif

    yi = str.find_first_of('Y') + 2;
    zi = str.find_first_of('Z') + 2;
    ei = str.find_first_of('E') + 2;

    *xpos = atof(str.substr(xi, (yi - 4) - xi).c_str());
    *ypos = atof(str.substr(yi, (zi - 4) - yi).c_str());
    *zpos = atof(str.substr(zi, (ei - 4) - zi).c_str());
    wait_done(2.0);
    return true;
}

bool MotionControl::wait_done(float to)
{
    std::string data;
    std::time_t now, pre;
    double diffsec = 0;

    time(&pre);
    while (data.compare("Done."))
    {
        try
        {
            port_.ReadLine(data);
#ifdef SERIALPORT_DEBUG
            std::cout << "READ: " << data << std::endl;
#endif
        }
        catch (Exception &e)
        {
            std::cerr << "[ERROR]: Could not read on serial port!" << e.what() << std::endl;
            return false;
        }

        // log data?
        time(&now);
        diffsec = difftime(now, pre);
        if (diffsec > to)
        {
            std::cout << "[WARNING]: TIMEOUT: When waiting for done. This is not a good sign." << std::endl;
            return false;
            // WARNING! If a timeout is received during an experiment, it's probable the robot will have an inconsistent behavior!
            // Maybe there should be a Warning Flag to inform users!?
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return true;
}

bool MotionControl::disable_motors()
{
    std::string cmd = "M18\n";
    try
    {
        port_.Write(cmd);
#ifdef SERIALPORT_DEBUG
        std::cout << "WRITE: " << cmd;
#endif
    }
    catch (Exception &e)
    {
        std::cerr << "[ERROR]: Could not write on serial port!" << e.what() << std::endl;
        return false;
    }

    return wait_done(10.0f);
}

// scan accel
bool MotionControl::set_acceleration(const int ini_acc, const int max_acc)
{
    // Set Max acceleration
    std::string cmd = "M201 Y" + std::to_string(max_acc) + '\n';
    try
    {
        port_.Write(cmd);
#ifdef SERIALPORT_DEBUG
        std::cout << "WRITE: " << cmd;
#endif
    }
    catch (Exception &e)
    {
        std::cerr << "[ERROR]: Could not write on serial port!" << e.what() << std::endl;
        return false;
    }

    wait_done(10.0f);

    // Set starting acceleration
    cmd.clear();
    cmd = "M204 P" + std::to_string(ini_acc) + " T" + std::to_string(ini_acc) + '\n';
    try
    {
        port_.Write(cmd);
#ifdef SERIALPORT_DEBUG
        std::cout << "WRITE: " << cmd;
#endif
    }
    catch (Exception &e)
    {
        std::cerr << "[ERROR]: Could not write on serial port!" << e.what() << std::endl;
        return false;
    }

    return wait_done(10.0f);
}

bool MotionControl::set_steps_mm_Y(const int stepsmm)
{
    // Set Max acceleration
    std::string cmd = "M92 Y" + std::to_string(stepsmm) + '\n';
    try
    {
        port_.Write(cmd);
#ifdef SERIALPORT_DEBUG
        std::cout << "WRITE: " << cmd;
#endif
    }
    catch (Exception &e)
    {
        std::cerr << "[ERROR]: Could not write on serial port!" << e.what() << std::endl;
        return false;
    }

    return wait_done(5.0f);
}

bool MotionControl::set_lights(const bool on)
{
    // Set Max acceleration
    std::string cmd;
    if (on)
        cmd = "M42 P68 S255\n";
    else
        cmd = "M42 P68 S0\n";
    try
    {
        port_.Write(cmd);
#ifdef SERIALPORT_DEBUG
        std::cout << "WRITE: " << cmd;
#endif
    }
    catch (Exception &e)
    {
        std::cerr << "[ERROR]: Could not write on serial port!" << e.what() << std::endl;
        return false;
    }

    return wait_done(5.0f);
}