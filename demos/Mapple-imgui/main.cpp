/*
*	Copyright (C) 2015 by Liangliang Nan (liangliang.nan@gmail.com)
*	https://3d.bk.tudelft.nl/liangliang/
*
*	This file is part of Easy3D. If it is useful in your research/work,
*   I would be grateful if you show your appreciation by citing it:
*   ------------------------------------------------------------------
*           Liangliang Nan.
*           Easy3D: a lightweight, easy-to-use, and efficient C++
*           library for processing and rendering 3D data. 2018.
*   ------------------------------------------------------------------
*
*	EasyGUI is free software; you can redistribute it and/or modify
*	it under the terms of the GNU General Public License Version 3
*	as published by the Free Software Foundation.
*
*	EasyGUI is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <algorithm>

#include <easy3d/util/file.h>
#include <easy3d/util/logging.h>

#include "../window/panel.h"
#include "mapple.h"
#include "plugin_polyfit.h"


using namespace easy3d;

int main(int argc, char** argv) {
    try {
        // Initialize Google's logging library.
        const std::string& dir = file::get_current_working_directory();
        const std::string log_dir = dir + "/logs/";
        if (!easy3d::file::is_directory(log_dir))
            easy3d::file::create_directory(log_dir);
        google::SetLogDestination(google::GLOG_INFO, log_dir.c_str());
        google::SetLogFilenameExtension("Mapple_log-");
    #ifndef NDEBUG
        FLAGS_stderrthreshold = google::GLOG_INFO;   // requires gflags
    #else
        FLAGS_stderrthreshold = google::GLOG_WARNING;   // requires gflags
    #endif
        FLAGS_colorlogtostderr = true;                  // requires gflags
        google::InitGoogleLogging(argv[0]);
        LOG(INFO) << "Current working directory: " << dir;

        Mapple app(4, 3, 2);

 		Panel pan(&app, "Rendering Panel");
 		PluginPolyFit  polyfit(&pan);

#if 0
		Panel win2(&app, "Model Panel");
		PluginPolyFit polyfit2(&win2);

		Panel win3(&app, "Model Panel##3");
		PluginPolyFit polyfit3(&win3);
#endif

        app.resize(800, 600);
		app.run();

    } catch (const std::runtime_error &e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
		std::cerr << error_msg << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
