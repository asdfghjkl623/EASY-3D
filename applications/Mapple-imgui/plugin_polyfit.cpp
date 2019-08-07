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

#include "plugin_polyfit.h"

#include <3rd_party/imgui/imgui.h>
#include <3rd_party/imgui/impl/imgui_impl_glfw.h>
#include <3rd_party/imgui/impl/imgui_impl_opengl3.h>
#include <3rd_party/glfw/include/GLFW/glfw3.h>


namespace easy3d {


    bool PluginPolyFit::draw() const {

		static double doubleVariable = 0.1;

		// Add new group
		if (ImGui::CollapsingHeader(title_.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID(title_.c_str());	//  to avoid ID conflict
			// Expose variable directly ...
            //ImGui::InputDouble("double", &doubleVariable, 0, 0, "%.4f");
            ImGui::DragScalar("double", ImGuiDataType_Double, &doubleVariable, 0.1, 0, 0, "%.4f");
			ImGui::PopID();
		}
		return false;
	}


}
