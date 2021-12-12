/********************************************************************
 * Copyright (C) 2015 Liangliang Nan <liangliang.nan@gmail.com>
 * https://3d.bk.tudelft.nl/liangliang/
 *
 * This file is part of Easy3D. If it is useful in your research/work,
 * I would be grateful if you show your appreciation by citing it:
 * ------------------------------------------------------------------
 *      Liangliang Nan.
 *      Easy3D: a lightweight, easy-to-use, and efficient C++ library
 *      for processing and rendering 3D data.
 *      Journal of Open Source Software, 6(64), 3255, 2021.
 * ------------------------------------------------------------------
 *
 * Easy3D is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 3
 * as published by the Free Software Foundation.
 *
 * Easy3D is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ********************************************************************/

#include "panel.h"
#include "plugin.h"

#include <3rd_party/imgui/imgui.h>
#include <3rd_party/imgui/backends/imgui_impl_glfw.h>
#include <3rd_party/glfw/include/GLFW/glfw3.h>

#include "viewer.h"


namespace easy3d {


	Panel::Panel(ViewerImGui* viewer, const std::string& title)
		: name_(title)
		, visible_(true)
	{
		viewer_ = viewer;
	}


    bool Panel::add_plugin(Plugin *plugin) {
        if (!plugin)
            return false;

        for (auto p : plugins_) {
            if (p == plugin) {
                LOG(WARNING) << "plugin '" << plugin->name_ << "' already added to the panel";
                return false;
            }
        }

        plugins_.push_back(plugin);
        return true;
    }


    void Panel::cleanup()
	{
		for (auto p : plugins_) {
            p->cleanup();
            delete p;
        }
	}

	// Mouse IO
	bool Panel::mouse_press(int button, int modifier)
	{
		ImGui_ImplGlfw_MouseButtonCallback(viewer_->window_, button, GLFW_PRESS, modifier);
		return ImGui::GetIO().WantCaptureMouse;
	}

	bool Panel::mouse_release(int button, int modifier)
	{
		return ImGui::GetIO().WantCaptureMouse;
	}

	bool Panel::mouse_move(int mouse_x, int mouse_y)
	{
		return ImGui::GetIO().WantCaptureMouse;
	}

	bool Panel::mouse_scroll(double delta_y)
	{
		ImGui_ImplGlfw_ScrollCallback(viewer_->window_, 0.f, delta_y);
		return ImGui::GetIO().WantCaptureMouse;
	}

	// Keyboard IO
	bool Panel::char_input(unsigned int key)
	{
		ImGui_ImplGlfw_CharCallback(nullptr, key);
		return ImGui::GetIO().WantCaptureKeyboard;
	}

	bool Panel::key_press(int key, int modifiers)
	{
		ImGui_ImplGlfw_KeyCallback(viewer_->window_, key, 0, GLFW_PRESS, modifiers);
		return ImGui::GetIO().WantCaptureKeyboard;
	}

	bool Panel::key_release(int key, int modifiers)
	{
		ImGui_ImplGlfw_KeyCallback(viewer_->window_, key, 0, GLFW_RELEASE, modifiers);
		return ImGui::GetIO().WantCaptureKeyboard;
	}


	bool Panel::draw()
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
        flags |= ImGuiWindowFlags_NoMove;

		ImGui::Begin(name_.c_str(), &visible_, flags);
		ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.4f);

		draw_widgets();
		for (auto p : plugins_)
			p->draw();

		ImGui::PopItemWidth();
		ImGui::End();

		return false;
	}


	void Panel::draw_widgets()
	{
		// Mesh
		if (ImGui::CollapsingHeader("File", ImGuiTreeNodeFlags_DefaultOpen))
		{
			float w = ImGui::GetContentRegionAvail().x;
			float p = ImGui::GetStyle().FramePadding.x;
			if (ImGui::Button("Load##Model", ImVec2((w - p) / 2.f, 0)))
			{
				viewer_->open();
			}
			ImGui::SameLine(0, p);
			if (ImGui::Button("Save##Model", ImVec2((w - p) / 2.f, 0)))
			{
				viewer_->save();
			}
		}
	}

}
