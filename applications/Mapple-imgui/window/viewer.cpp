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

#include "viewer.h"

#include "plugin.h"
#include "panel.h"

#include <easy3d/renderer/camera.h>

#include <3rd_party/imgui/misc/fonts/imgui_fonts_droid_sans.h>
#include <3rd_party/imgui/imgui.h>
#include <3rd_party/imgui/backends/imgui_impl_glfw.h>
#include <3rd_party/imgui/backends/imgui_impl_opengl3.h>
#include <3rd_party/glfw/include/GLFW/glfw3.h>


namespace easy3d {

    ImGuiContext* ViewerImGui::context_ = nullptr;

    ViewerImGui::ViewerImGui(
        const std::string& title /* = "Easy3D ImGui Viewer" */,
		int samples /* = 4 */,
		int gl_major /* = 3 */,
		int gl_minor /* = 2 */)
    : Viewer(title, samples, gl_major, gl_minor)
	{
        background_color_ = vec4(1.0f, 1.0f, 1.0f, 1.0f);

        camera()->setUpVector(vec3(0, 1, 0));
        camera()->setViewDirection(vec3(0, 0, -1));
        camera_->showEntireScene();
	}


    bool ViewerImGui::add_panel(Panel* panel) {
        if (!panel)
            return false;

        for (auto p : panels_) {
            if (p == panel) {
                LOG(WARNING) << "panel '" << panel->name_ << "' already added to the viewer";
                return false;
            }
        }

        panels_.push_back(panel);
        return true;
    }


    void ViewerImGui::init() {
        Viewer::init();

		if (!context_) {
			// Setup ImGui binding
			IMGUI_CHECKVERSION();

			context_ = ImGui::CreateContext();

			const char* glsl_version = "#version 150";
			ImGui_ImplGlfw_InitForOpenGL(window_, false);
			ImGui_ImplOpenGL3_Init(glsl_version);
			ImGuiIO& io = ImGui::GetIO();
            io.WantCaptureKeyboard = true;
            io.WantTextInput = true;
            io.IniFilename = nullptr;
			ImGui::StyleColorsDark();
			ImGuiStyle& style = ImGui::GetStyle();
			style.FrameRounding = 5.0f;

			// load font

            auto reload_font = [this](int font_size) -> void {
                ImGuiIO& io = ImGui::GetIO();
                io.Fonts->Clear();
                io.Fonts->AddFontFromMemoryCompressedTTF(droid_sans_compressed_data, droid_sans_compressed_size, font_size * dpi_scaling());

                // Computes pixel ratio for hidpi devices
                io.FontGlobalScale = 1.0f / dpi_scaling();
                ImGui_ImplOpenGL3_DestroyDeviceObjects();
            };
            reload_font(16);
		}
	}


    void ViewerImGui::post_resize(int w, int h) {
        Viewer::post_resize(w, h);
		if (context_) {
			ImGui::GetIO().DisplaySize.x = float(w);
			ImGui::GetIO().DisplaySize.y = float(h);
		}
	}


    bool ViewerImGui::callback_event_cursor_pos(double x, double y) {
		if (ImGui::GetIO().WantCaptureMouse)
			return true;
		else
			return Viewer::callback_event_cursor_pos(x, y);
	}


    bool ViewerImGui::callback_event_mouse_button(int button, int action, int modifiers) {
		if (ImGui::GetIO().WantCaptureMouse)
			return true;
		else
			return Viewer::callback_event_mouse_button(button, action, modifiers);
	}


    bool ViewerImGui::callback_event_keyboard(int key, int action, int modifiers) {
		if (ImGui::GetIO().WantCaptureKeyboard)
            return true;
		else
			return Viewer::callback_event_keyboard(key, action, modifiers);
	}


    bool ViewerImGui::callback_event_character(unsigned int codepoint) {
		if (ImGui::GetIO().WantCaptureKeyboard)
			return true;
		else
			return Viewer::callback_event_character(codepoint);
	}


    bool ViewerImGui::callback_event_scroll(double dx, double dy) {
		if (ImGui::GetIO().WantCaptureMouse)
			return true;
		else
			return Viewer::callback_event_scroll(dx, dy);
	}


    void ViewerImGui::cleanup() {
        for (auto p : panels_) {
            p->cleanup();
            delete p;
        }

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();

		ImGui::DestroyContext(context_);
        Viewer::cleanup();
	}


    void ViewerImGui::pre_draw() {
        ImGui_ImplOpenGL3_NewFrame();  
        ImGui_ImplGlfw_NewFrame();    
        ImGui::NewFrame();
        Viewer::pre_draw(); 
	}


    void ViewerImGui::post_draw() {
        for (std::size_t i = 0; i < panels_.size(); ++i) {
            Panel* win = panels_[i];
            if (!win->visible_)
                continue;
            const float panel_width = 180.f;
            const float offset = 10.0f;
            ImGui::SetNextWindowPos(ImVec2((panel_width + offset) * i, offset * i), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(0.0f, 100), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSizeConstraints(ImVec2(panel_width, -1.0f), ImVec2(panel_width, -1.0f));
            ImGui::SetNextWindowBgAlpha(1.0f);
            win->draw();
        }

		ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        Viewer::draw_corner_axes();
	}
}
