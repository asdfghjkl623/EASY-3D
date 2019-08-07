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


#include "mapple.h"

#include <easy3d/viewer/drawable.h>
#include <easy3d/core/surface_mesh.h>
#include <easy3d/core/point_cloud.h>
#include <easy3d/algo/point_cloud_poisson_reconstruction.h>
#include <easy3d/algo/point_cloud_ransac.h>
#include <easy3d/algo/surface_mesh_sampler.h>
#include <easy3d/core/random.h>

#include <3rd_party/imgui/imgui.h>
#include <3rd_party/imgui/impl/imgui_impl_glfw.h>
#include <3rd_party/imgui/impl/imgui_impl_opengl3.h>
#include <3rd_party/glfw/include/GLFW/glfw3.h>

using namespace easy3d;

Mapple::Mapple(
	int num_samples /* = 4 */,
	int gl_major /* = 3 */,
	int gl_minor /* = 2 */ 
) : MainWindow("Mapple", num_samples, gl_major, gl_minor)
{

}


void Mapple::draw_menu_algorithm() {
    static bool show_RANSAC_dialog = false;
    if (ImGui::BeginMenu("Algorithm")) {
        if (ImGui::MenuItem("Poisson Reconstruction"))
            poisson_surface_reconstruction();

        if (ImGui::MenuItem("Primitive Extraction"))
            show_RANSAC_dialog = true;

        if (ImGui::MenuItem("Sampling Surface"))
            sampling_surface();
         ImGui::EndMenu();
    }

    if (show_RANSAC_dialog) {
        ImGui::SetNextWindowPos(ImVec2(width() * 0.5f, height() * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::Begin("Extract Primitives", &show_RANSAC_dialog, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
        static int min_support = 1000;
        static float distance_threshold = 0.005f;

        ImGui::SliderInt("Minimum support", &min_support, 5, 10000);
        ImGui::SliderFloat("Distance threshold", &distance_threshold, 0.0001f, 0.01f);
        if (ImGui::Button("Extract")) {
            primitive_extraction_ransac(min_support, distance_threshold);
        }
        ImGui::End();
    }

}


void Mapple::poisson_surface_reconstruction() {
    if (!current_model())
        return;
    PointCloud* cloud = dynamic_cast<PointCloud*>(current_model());
    if (!cloud)
        return;

    PoissonReconstruction rec;
    rec.set_depth(6);
    std::cout << "I need a dialog for the parameters" << std::endl;
    SurfaceMesh* mesh = rec.apply(cloud);
    if (mesh) {
        create_drawables(mesh);
        add_model(mesh);
    }
}


void Mapple::primitive_extraction_ransac(int min_support, float dist_threshold) {
    if (!current_model()) {
        std::cerr << "no point cloud exists" << std::endl;
        return;
    }

    PointCloud* cloud = dynamic_cast<PointCloud*>(current_model());
    if (!cloud)
        return;

    auto normals = cloud->get_vertex_property<vec3>("v:normal");
    if (!normals) {
        std::cerr << "RANSAC requires point normals" << std::endl;
        return;
    }

    PrimitivesRansac ransac;
    ransac.add_primitive_type(PrimitivesRansac::PLANE);
    int num = ransac.detect(cloud, min_support, dist_threshold);
    std::vector<vec3> random_colors(num);
    for (auto& c : random_colors)
        c = random_color();

    auto indices = cloud->get_vertex_property<int>("v:segment_index");
    std::vector<vec3> colors(cloud->n_vertices());
    for (auto v : cloud->vertices()) {
        int segment_idx = indices[v];
        if (segment_idx == -1)
            colors[v.idx()] = vec3(0.5,0.5,0.5);
        else
            colors[v.idx()] = random_colors[segment_idx];
    }
    cloud->points_drawable("vertices")->update_color_buffer(colors);
}


void Mapple::sampling_surface() {
    if (!current_model())
        return;

    SurfaceMesh* mesh = dynamic_cast<SurfaceMesh*>(current_model());
    if (!mesh)
        return;

    SurfaceMeshSampler sampler;
    PointCloud* cloud = sampler.apply(mesh);
    if (cloud) {
        create_drawables(cloud);
        add_model(cloud);
        delete_model(mesh);
    }
}

