#include "Hyperbird.hpp"
#include <filesystem>
#include <cctype>

// Due to this member variable being static, it needs to be declared where is used.
volatile std::sig_atomic_t Hyperbird::finish = 0;

Hyperbird::Hyperbird()
{
	tray_info.date_str = "";
	tray_info.exp_name = "";
	tray_info.tray_name = "";
	tray_info.innoc_date = {};
	tray_info.nsamples = 0;
	root_data_dir1_.assign("");
	current_sample_ = 0;

	connected_ = false;

	// Show version, provisional
	std::cout << "** " << bold("Hyperbird Control Interface") << " v" << std::to_string(MAJOR) << "." << std::to_string(MINOR) << "." << std::to_string(PATCH) << " **" << std::endl;

	// Register signal handler function to stop the imaging process.
	std::signal(SIGINT, Hyperbird::interrupt_funct);
}

Hyperbird::~Hyperbird()
{
}

bool Hyperbird::is_interrupted()
{
	return Hyperbird::finish != 0;
}

std::string trim(const std::string &str)
{
	auto begin = std::find_if_not(str.begin(), str.end(), [](int ch)
								  { return std::isspace(ch); });

	auto end = std::find_if_not(str.rbegin(), str.rend(), [](int ch)
								{ return std::isspace(ch); })
				   .base();

	if (begin >= end)
		return "";

	return std::string(begin, end);
}

bool Hyperbird::read_config(const std::string cfg_file, const std::string root_dir, double z_pos)
{
	std::string entry, parameter, value;
	std::size_t equal_pos;

	std::ifstream cfgfile(cfg_file);
	if (!cfgfile.is_open())
	{
		std::cout << red("[Hyperbird]: ") << "Config file not found!" << std::endl;
		return false;
	}

	std::cout << bold("[Hyperbird]: ") << "Config file: " << cfg_file << std::endl;
	while (std::getline(cfgfile, entry))
	{
		// Skip comment lines
		if (entry[0] == '#' || entry[0] == '\0')
			continue;

		equal_pos = entry.find_first_of('=');
		parameter = trim(entry.substr(0, equal_pos));
		value = trim(entry.substr(equal_pos + 1));

		// Assign configurationss
		if (!parameter.compare("SpectralBinning"))
			config_.spectral_binning = std::stoi(value);
		else if (!parameter.compare("SpatialBinning"))
			config_.spatial_binning = std::stoi(value);
		else if (!parameter.compare("ExpTime"))
			config_.exp_time = std::stod(value);
		else if (!parameter.compare("GlobalShutter"))
			config_.global_shutter = std::stoi(value);
		else if (!parameter.compare("ScanSpeed"))
			config_.scan_speed = std::stoi(value);
		else if (!parameter.compare("Aperture"))
			config_.lens_aperture = std::stod(value);
		else if (!parameter.compare("Zpos"))
		{
			if (z_pos != 0.0) {
				value = std::to_string(z_pos);
				config_.zpos = std::stod(value);
			} else {
				config_.zpos = std::stod(value);
			}
		}
		else if (!parameter.compare("PixelRes"))
			config_.pixelres = std::stoi(value);
        else if (!parameter.compare("FlatFieldCorrection"))
            config_.do_ffc = (std::stoi(value) != 0);
		else if (!parameter.compare("RootDir1"))
		{
			if (!root_dir.empty()) {
				value = root_dir;
				root_data_dir1_.assign(value);
			} else {
				root_data_dir1_.assign(value);
			}
			// Check if exists
			if (!std::filesystem::exists(root_data_dir1_))
			{
				std::cout << red("[ERROR]") << "; Root path '" << root_data_dir1_ << "' does not exist!" << std::endl;
				std::cout << "** Make sure to mount the drive if using a secondary hard drive." << std::endl;
				return false;
			}
		}
		else if (!parameter.compare("RootDir2"))
		{
			root_data_dir2_.assign(value);
			// Check if exists
			if (!std::filesystem::exists(root_data_dir2_))
			{
				std::cout << red("[ERROR]") << "; Root path '" << root_data_dir2_ << "' does not exist!" << std::endl;
				std::cout << "** Make sure to mount the drive if using a secondary hard drive." << std::endl;
				return false;
			}
		}
		else
		{
			std::cout << parameter << " configuration entry not recognized!" << std::endl;
			continue;
		}
		std::cout << "  --> " << parameter << " set to " << value << std::endl;
	}

	// Check if available space for current experiment
	const std::filesystem::space_info si = std::filesystem::space(root_data_dir1_);
	if (static_cast<std::intmax_t>(si.available) < (long int)((2190 * 2000) / config_.spatial_binning * (950 / config_.spectral_binning) * 2) * tray_info.nsamples)
	{
		std::cout << std::endl
				  << bold("[WARNING]:") << " There might be not enough free disk space for raw data!" << std::endl;
	}

	if (config_.do_ffc) {
        const std::filesystem::space_info si2 = std::filesystem::space(root_data_dir2_);
        if (static_cast<std::intmax_t>(si2.available) < (long int)((2190 * 2000) / config_.spatial_binning * (950 / config_.spectral_binning) * 2) * tray_info.nsamples)
        {
            std::cout << std::endl
                      << bold("[WARNING]:") << " There might be not enough free disk space for the FFC data!" << std::endl;
        }
    }return true;
}

bool Hyperbird::connect(const char *port_name)
{

	if (!motion_.init(port_name, 115200))
	{
		std::cout << red("[Hyperbird]: ") << "Could not connect to motor board:" << std::endl;
		return false;
	}
	std::cout << bold("[Hyperbird]: ") << "Motion connected." << std::endl;

	camera_.set_wavelength_range(400.0, 1000.0);

	if (!camera_.open(config_.spatial_binning, config_.spectral_binning, config_.exp_time, config_.pixelres, config_.global_shutter))
		connected_ = false;
	else
		connected_ = true;

	
    // Set processing toggles based on config
    camera_.set_processing_threads_enabled(config_.do_ffc);
    camera_.set_ffc_enabled(config_.do_ffc);
if (connected_)
	{
		std::cout << "[Hyperbird]: Calibrating...\r" << std::flush;
		motion_.find_zero(true); // force = true first time?
		std::cout << "[Hyperbird]: Calibration done." << std::endl;
	}

	return connected_;
}

bool Hyperbird::close()
{
	motion_.disable_motors();
	motion_.close();
	camera_.close();
	return true;
}

bool Hyperbird::read_excel_file(const std::string path)
{
	xlnt::workbook wb;
	xlnt::worksheet ws;
	xlnt::row_t row;
	xlnt::column_t col;

	xlnt::cell_reference act_cell = xlnt::cell_reference("B3"); // Sample 1 position

	uint32_t s_ind = 0;
	std::tm *date_now;
	time_t t_innoc, t_now;

	// Clear tray info data
	tray_info.date_str = "";
	tray_info.exp_name = "";
	tray_info.tray_name = "";
	tray_info.innoc_date = {};
	tray_info.slabels.clear();
	tray_info.slabels.reserve(MAX_SAMPLES);

	try
	{
		wb.load(path);
		ws = wb.active_sheet();
	}
	catch (const std::exception &e)
	{
		std::cerr << "File not supported: " << e.what() << std::endl;
		return false;
	}

	row = ws.highest_row();
	col = ws.highest_column();

	if (col.index != 8 || row != 102)
	{
		std::cout << "Error on Excel sheet format! Expected 8x102 cells. Found " + std::to_string(col.index) + "x" + std::to_string(row) + " cells." << std::endl;
		return false;
	}

	tray_info.exp_name = ws.cell("D1").value<std::string>();

	if (ws.cell("H1").data_type() == xlnt::cell_type::number)
		tray_info.tray_name = std::to_string(ws.cell("H1").value<int>());
	else
		tray_info.tray_name = ws.cell("H1").value<std::string>();

	tray_info.innoc_date.tm_mon = ws.cell("D2").value<int>() - 1;
	tray_info.innoc_date.tm_mday = ws.cell("F2").value<int>();
	tray_info.innoc_date.tm_year = ws.cell("H2").value<int>() - 1900;
	tray_info.innoc_date.tm_sec = 0;
	tray_info.innoc_date.tm_min = 0;
	tray_info.innoc_date.tm_hour = 0;
	t_innoc = mktime(&tray_info.innoc_date);
	if (t_innoc == -1)
	{
		std::cout << L"Incorrect inoculation date!" << std::endl;
		return false;
	}

	while (s_ind < MAX_SAMPLES)
	{

		if (ws.cell(act_cell).data_type() == xlnt::cell_type::number)
		{
			// tray_info.slabels.at(s_ind) = std::to_string(ws.cell(act_cell).value<int>());
			tray_info.slabels.emplace_back(std::to_string(ws.cell(act_cell).value<int>()));
		}
		else
		{
			// tray_info.slabels.at(s_ind) = ws.cell(act_cell).value<std::string>();
			tray_info.slabels.emplace_back(ws.cell(act_cell).value<std::string>());
		}

		// Increment linear index
		s_ind++;

		// Next sample label from Excel file
		if (!(s_ind % 100))
		{
			act_cell = act_cell.make_offset(2, -99);
		}
		else
			act_cell = act_cell.make_offset(0, 1);
	}

	t_now = time(0);

	date_now = localtime(&t_now);

	date_now->tm_sec = 0;
	date_now->tm_min = 0;
	date_now->tm_hour = 0;
	double diffsec = difftime(mktime(date_now), t_innoc);
	int dpi = round(diffsec / 86400.0);

	tray_info.date_str.assign(std::to_string(date_now->tm_mon + 1) + '-' + std::to_string(date_now->tm_mday) + '-' + std::to_string(date_now->tm_year + 1900) + '_' + std::to_string(dpi) + "dpi");

	return true;
}

std::string to_lower_info(const std::string &str)
{
	std::string lower;
	lower.reserve(str.size());
	for (char c : str)
	{
		lower += std::tolower(static_cast<unsigned char>(c));
	}
	return lower;
}

void Hyperbird::print_tray_info()
{
	int cw;
	int offsetx = 0;
	int i_sample = 0;
	int n_samples = 0;
	int index_table[MAX_SAMPLES];

	int n_white = 0;
	int n_black = 0;
	int n_other = 0;
	int n_empty = 0;

	int x = 0;
	int y = 0;

	if (tray_info.exp_name.empty())
	{
		std::cout << "No tray data was loaded!" << std::endl;
		return;
	}

	// Initialize index table
	for (int i = 0; i < MAX_SAMPLES; i++)
	{
		index_table[x + (y * TRAY_GRID_W) - (y / 2)] = i;
		if (y % 2) // odd, 1 cell less
		{
			x--;
			if (x < 0)
			{
				y++;
				x = 0;
			}
		}
		else
		{
			x++;
			if (x == TRAY_GRID_W)
			{
				y++;
				x = TRAY_GRID_W - 2;
			}
		}
	}

	std::cout << "\n+------------------ " + bold("TRAY INFO") + " ------------------+" << std::endl;
	std::cout << bold("* Experiment ID: ") << tray_info.exp_name << std::endl;
	std::cout << bold("* Tray ID: ") << tray_info.tray_name << std::endl;
	std::cout << bold("* Innoc. Date: ") + std::to_string(tray_info.innoc_date.tm_mon + 1) + "/" + std::to_string(tray_info.innoc_date.tm_mday) + "/" + std::to_string(tray_info.innoc_date.tm_year + 1900) << std::endl;
	std::cout << bold("* Load Date: ") << tray_info.date_str << std::endl;

	std::cout << " ┌";
	for (int x = 0; x < TRAY_GRID_W; x++)
		std::cout << "\e[1m──\e[0m";
	std::cout << "┐" << std::endl;

	for (int y = 0; y < TRAY_GRID_H; y++)
	{
		std::cout << "\e[1m │\e[0m";
		offsetx = y % 2;
		if (offsetx)
		{
			cw = TRAY_GRID_W - 1;
			std::cout << " ";
		}
		else
			cw = TRAY_GRID_W;

		for (int x = 0; x < cw; x++)
		{
			const std::string &label = tray_info.slabels.at(index_table[i_sample]);
			if (label.empty())
			{
				std::cout << "\033[0;90m⬤ \033[0m"; // gray
				n_empty++;
			}
			else
			{
				std::string label_lower = to_lower_info(label);
				if (label_lower.find("black") != std::string::npos)
				{
					std::cout << "\033[1;33m⬤ \033[0m"; // yellow
					n_black++;
				}
				else if (label_lower.find("white") != std::string::npos)
				{
					std::cout << "\033[1;34m⬤ \033[0m"; // blue
					n_white++;
				}
				else
				{
					std::cout << "\033[1;32m⬤ \033[0m"; // green
					n_other++;
				}
				n_samples++;
			}
			i_sample++;
		}

		if (offsetx)
			std::cout << "\e[1m │\e[0m" << std::endl;
		else
			std::cout << "\e[1m│\e[0m" << std::endl;
	}

	std::cout << " └";
	for (int x = 0; x < TRAY_GRID_W; x++)
		std::cout << "\e[1m──\e[0m";
	std::cout << "┘" << std::endl;
	std::cout << bold("* Tray Summary:\n");
	std::cout << "  \033[1;32m⬤\033[0m Sample   : " << n_other << std::endl;
	std::cout << "  \033[1;34m⬤\033[0m White   : " << n_white << std::endl;
	std::cout << "  \033[1;33m⬤\033[0m Black   : " << n_black << std::endl;
	std::cout << "  \033[0;90m⬤\033[0m Empty   : " << n_empty << std::endl;
	std::cout << bold("* Total labeled  : ") << n_samples << std::endl;
	std::cout << "+------------------------------------------------+" << std::endl;
}

bool Hyperbird::get_position(double &xpos, double &ypos, double &zpos)
{
	return motion_.get_motor_pos(&xpos, &ypos, &zpos);
}

void Hyperbird::set_Zpos(double zpos)
{
	config_.zpos = zpos;
}

double Hyperbird::get_Zpos()
{
	return config_.zpos;
}

bool Hyperbird::goto_first_sample()
{
	// Check if robot is connected?
	if (!connected_)
	{
		std::cout << red("[ERROR]") << ": Hyperbird not connected!" << std::endl;
		return false;
	}

	current_sample_ = 0;

	// Keep incrementing sample index until a sample label is found
	while (tray_info.slabels.at(current_sample_).empty())
	{
		if ((current_sample_ + 1) >= (int)tray_info.slabels.size())
		{
			std::cout << "[WARNING]: No samples found in loaded tray!" << std::endl;
			return false;
		}
		current_sample_++;
	}

	return motion_.move_to_sample(current_sample_ + 1, config_.zpos, 0.0, 0.0);
}

// Goto first sample position with Z at its lowest height
bool Hyperbird::goto_end_pos()
{
	motion_.move_to_sample(1, 24.0, 0.0, 0.0);
	return true;
}

// returns false if is finished or motion failed
bool Hyperbird::goto_next_sample()
{
	// Check if robot is connected?
	if (!connected_)
	{
		std::cout << red("[ERROR]") << ": Hyperbird not connected!" << std::endl;
		return false;
	}

	// Check if next label is out of bounds
	if (current_sample_ + 1 >= static_cast<int>(tray_info.slabels.size()))
		return false;

	// Add 1 to sample index
	current_sample_++;

	// Continue incrementing sample index until a sample label is found
	while (tray_info.slabels.at(current_sample_).empty())
	{
		if ((current_sample_ + 1) >= (int)tray_info.slabels.size())
			return false;
		current_sample_++;
	}

	return motion_.move_to_sample(current_sample_ + 1, config_.zpos, 0.0, 0.0);
}

bool Hyperbird::goto_sample(const int nsample)
{
	// Check if robot is connected?
	if (!connected_)
	{
		std::cout << red("[ERROR]") << ": Hyperbird not connected!" << std::endl;
		return false;
	}

	return motion_.move_to_sample(nsample, config_.zpos, 0.0, 0.0);
}

bool Hyperbird::scan_current_sample()
{
	bool ok = true;
	std::string fpath1 = root_data_dir1_;
	std::string fpath2 = root_data_dir2_;
	std::error_code ec;
	std::stringstream idstr;

	// Create main output folder (raw)
	fpath1 += "/" + tray_info.exp_name;

	if (!std::filesystem::create_directories(fpath1, ec))
	{
		if (ec)
		{
			std::cout << red("[ERROR]") << ": Could not create directories to store raw images! " << ec.message() << std::endl;
			return false;
		}
	}

	// Create secondary output folder (corrected) only if FFC is enabled
    if (config_.do_ffc) {
        fpath2 += "/" + tray_info.exp_name + "_Processed";
        if (!std::filesystem::create_directories(fpath2, ec))
        {
            if (ec)
            {
                std::cout << red("[ERROR]") << ": Could not create directories to store FFC images! " << ec.message() << std::endl;
                return false;
            }
        }
    }// Get sample index string
	idstr << std::setw(3) << std::setfill('0') << current_sample_ + 1;

	// Capture base filenames
	std::string base_raw = fpath1 + "/" + idstr.str() + "-" + tray_info.slabels.at(current_sample_);
	std::string base_ffc = config_.do_ffc ? (fpath2 + "/" + idstr.str() + "-" + tray_info.slabels.at(current_sample_) + "_ffc") : std::string();

	// Change acceleration
	motion_.set_acceleration(10, 10);
	motion_.set_lights(true);
	motion_.move_to(999, scan_dist, 999, config_.scan_speed, true, 0); // Non-blocking

	// Begin acquisition
	ok = camera_.start_acquisition(base_raw, base_ffc);

	if (!ok)
		return false;

	while (camera_.is_streaming())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// Stop acquisition
	camera_.stop_acquisition();

	// Wait here until the robot finishes moving
	motion_.wait_done(60000);
	motion_.set_acceleration(100, 100);

	if (camera_.check_stream_errors())
		ok = false;

	// If this was the BLACK sample, pause and ask user to proceed
	{
		std::string lbl = tray_info.slabels.at(current_sample_);
		for (char &c : lbl) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		if (lbl.find("black") != std::string::npos)
		{
			std::cout << "\n============================================================\n";
			std::cout << " Black reference scanning is completed.\n";
			std::cout << " Please REMOVE the camera cap before continuing.\n";
			std::cout << " Ready to move forward? (Y to continue / any other key to exit): ";
			std::string ans;
			std::cin >> ans;

			if (ans != "Y" && ans != "y")
			{
				std::cout << "\n[INFO]: Exiting program per user choice after black reference.\n";
				Hyperbird::finish = 1; // request termination; imaging loop will stop
				return false;          // stop this scan call
			}
			std::cout << "[INFO]: Continuing. Camera cap should now be removed.\n";
		}
	}

	return ok;
}