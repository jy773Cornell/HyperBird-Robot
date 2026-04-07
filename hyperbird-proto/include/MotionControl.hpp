#pragma once

#include "SerialPort.hpp"

class MotionControl
{
private:
    SerialPort port_; // Serial port object

    static constexpr int w_sgrid = 20; // Tray grid cells width
    static constexpr int h_sgrid = 18; // Tray grid cells height

    static constexpr double cell_sx = 13.521; // X Displacement in mm between each sample cell
    static constexpr double cell_sy = 11.63;  // Y

    // WARNING: Must be updated! Read from file?
    static constexpr double first_x = 269.7; // X position of centered first sample cell
    static constexpr double first_y = 12.3;  // Y
    static constexpr double first_z = 14.0;  // Z NOT USED, only fyi

    double X_, Y_, Z_;                      // Current coordinates
    double max_posx_, max_posy_, max_posz_; // Maximum frame position (obtained from MCB firmware), NOT USED
    bool first_calib_;                      // Whether is the first calibration upon starting this program

    static constexpr double min_posz = 2; // Z should never go less than this, otherwise it will touch the LEDs

public:
    MotionControl();
    ~MotionControl();

    /**
     * Initializes and opens serial communication with the motor control board (MCB)
     *
     *
     * @param port_path Serial port name or device path Ex: "/dev/ttyUSB0" or "COM3"
     * @param baudrate Baud rate of the serial communication
     * @return true if successfully connected, false otherwise
     */
    bool init(const char *port_path, int baudrate);

    /**
     * Close currently active serial port. Ends communications.
     */
    void close();

    /**
     * Calibrates all axes by finding the zero points / boundaries.
     *
     * This process is blocking.
     *
     * @param force whether the device must perform this process regardless it has been already calibrated
     * @return true if successful, false otherwise
     */
    bool find_zero(bool force);

    /**
     * Move camera to a specific XYZ coordinate.
     * This process is blocking if "to" is not zero.
     *
     * @param x,y,z Target euclidean position to move to. Use value 999 to any axis to skip.
     * @param f force or speed of movement in mm/s
     * @param rel whether the provided target position is relative or absolute
     * @param to timeout in milliseconds. Function returns false when 'to' time has passed.
     *           This function is non-blocking when "to" is zero.
     * @return true if successfully connected, false otherwise
     */
    bool move_to(double x, double y, double z, int f, bool rel, float to);

    /**
     * Move camera to a specific sample cell number.
     * This process is blocking.
     *
     * @param ns Target sample number in which the camera will displace to. Starts from 1 (not 0)
     * @param avgZ target Z coordinate
     * @param offX,offY XY offsets that will be added to the target position. Useful for correcting stacked positional errors.
     * @return true if successfully connected, false otherwise
     */
    bool move_to_sample(int ns, double avgZ, double offX, double offY);

    /**
     * Move camera to a specific XYZ coordinate.
     *
     * @param xpos,ypos,zpos pointers to double where the current positions will be stored.
     * @return true if successfully connected, false otherwise
     */
    bool get_motor_pos(double *xpos, double *ypos, double *zpos);

    /**
     * All stepper motors are powered off. Users may manually interact with the system after this.
     *
     * @return true if successful, false otherwise
     */
    bool disable_motors();

    /**
     * Set initial and maximum accelerations in mm/s
     *
     * @param ini_acc initial acceleration (default 100.0)
     * @param max_acc maximum acceleration (default 100.0)
     * @return true if successful, false otherwise
     */
    bool set_acceleration(const int ini_acc, const int max_acc);

    /**
     * Change steps per mm of Y axis.
     *  CAUTION: Use only when very slow speed is required (for 1x binning)
     *           Make sure you know what you are doing when using this
     * @param stepsmm steps per mm of Y axis (default 80.0)
     * @return true if successful, false otherwise
     */
    bool set_steps_mm_Y(const int stepsmm);

    /**
     * Turn the lights ON or OFF.
     * @param stepsmm steps per mm of Y axis (default 80.0)
     * @return true if successful, false otherwise
     */
    bool set_lights(const bool on);

    // setLEDs or lights or whatever

    /**
     * This function blocks the program until a "done" message is received from the MCB.
     * Always use this method after calling "move_to(...)" if its timeout/'to' parameter was set to 0.
     *
     * @param to timeout in milliseconds. Function returns false when 'to' time has passed.
     * @return true if "done" is received, false otherwise
     */
    bool wait_done(float to);
};
