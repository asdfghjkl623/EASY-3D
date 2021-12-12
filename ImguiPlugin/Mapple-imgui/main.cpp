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

#include <easy3d/util/logging.h>

#include "../window/viewer.h"
#include "../window/panel.h"
#include "plugin_polyfit.h"

using namespace easy3d;

int main(int argc, char **argv) {
    // Initialize logging.
    logging::initialize();

    ViewerImGui app;

    auto panel = new Panel(&app, "Rendering Panel");
    auto plugin = new PluginPolyFit(panel, "PolyFit");
    panel->add_plugin(plugin);

    app.add_panel(panel);

    app.resize(800, 600);

    return app.run();
}
