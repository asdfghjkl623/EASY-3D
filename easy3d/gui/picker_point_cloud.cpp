/**
 * Copyright (C) 2015 by Liangliang Nan (liangliang.nan@gmail.com)
 * https://3d.bk.tudelft.nl/liangliang/
 *
 * This file is part of Easy3D. If it is useful in your research/work,
 * I would be grateful if you show your appreciation by citing it:
 * ------------------------------------------------------------------
 *      Liangliang Nan.
 *      Easy3D: a lightweight, easy-to-use, and efficient C++
 *      library for processing and rendering 3D data. 2018.
 * ------------------------------------------------------------------
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
 */


#include <easy3d/gui/picker_point_cloud.h>
#include <easy3d/core/point_cloud.h>
#include <easy3d/viewer/shader_program.h>
#include <easy3d/viewer/shader_manager.h>
#include <easy3d/viewer/framebuffer_object.h>
#include <easy3d/viewer/opengl_error.h>
#include <easy3d/viewer/drawable_points.h>
#include <easy3d/viewer/opengl_info.h>
#include <easy3d/util/logging.h>


namespace easy3d {


    PointCloudPicker::PointCloudPicker(Camera *cam)
            : Picker(cam) {
        use_gpu_ = false;
    }


    int PointCloudPicker::pick_vertices(PointCloud *model, int min_x, int max_x, int min_y, int max_y, bool deselect) {
        if (!model)
            return 0;

        if (use_gpu_ && OpenglInfo::gl_version_number() >= 4.3) {
            auto program = ShaderManager::get_program("selection/selection_pointcloud_rect");
            if (!program) {
                std::vector<ShaderProgram::Attribute> attributes;
                attributes.push_back(ShaderProgram::Attribute(ShaderProgram::POSITION, "vtx_position"));
                program = ShaderManager::create_program_from_files("selection/selection_pointcloud_rect", attributes);
            }
            if (program)
                return pick_vertices_gpu(model, min_x, max_x, min_y, max_y, deselect, program);
            else
                LOG(WARNING) << "shader program not found, default to CPU implementation";
        }

        LOG_IF(WARNING, use_gpu_ && OpenglInfo::gl_version_number() < 4.3) << "GPU implementation requires OpenGL >= 4.3. Default to CPU implementation";

        // CPU with OpenMP (if supported)
        return pick_vertices_cpu(model, min_x, max_x, min_y, max_y, deselect);
    }


    int PointCloudPicker::pick_vertices(PointCloud *model, const std::vector<vec2> &plg, bool deselect) {
        if (!model)
            return 0;

        if (!model)
            return 0;

        if (use_gpu_ && OpenglInfo::gl_version_number() >= 4.3) {
            auto program = ShaderManager::get_program("selection/selection_pointcloud_lasso");
            if (!program) {
                std::vector<ShaderProgram::Attribute> attributes;
                attributes.push_back(ShaderProgram::Attribute(ShaderProgram::POSITION, "vtx_position"));
                program = ShaderManager::create_program_from_files("selection/selection_pointcloud_lasso", attributes);
            }
            if (program)
                return pick_vertices_gpu(model, plg, deselect, program);
            else
                LOG(WARNING) << "shader program not found, default to CPU implementation";
        }

        LOG_IF(WARNING, use_gpu_ && OpenglInfo::gl_version_number() < 4.3) << "GPU implementation requires OpenGL >= 4.3. Default to CPU implementation";

        // CPU with OpenMP (if supported)
        return pick_vertices_cpu(model, plg, deselect);
    }


    int PointCloudPicker::pick_vertices_cpu(PointCloud *model, int xmin, int xmax, int ymin, int ymax, bool deselect) {
        if (!model)
            return 0;

        int win_width = camera()->screenWidth();
        int win_height = camera()->screenHeight();

        float minX = xmin / float(win_width - 1);
        float minY = 1.0f - ymin / float(win_height - 1);
        float maxX = xmax / float(win_width - 1);
        float maxY = 1.0f - ymax / float(win_height - 1);
        if (minX > maxX) std::swap(minX, maxX);
        if (minY > maxY) std::swap(minY, maxY);

        const auto& points = model->get_vertex_property<vec3>("v:point").vector();
        int num = static_cast<int>(points.size());
        const mat4 &m = camera()->modelViewProjectionMatrix();

        auto& select = model->vertex_property<bool>("v:select").vector();

#pragma omp parallel for
        for (int i = 0; i < num; ++i) {
            const vec3 &p = points[i];
            float x = m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12];
            float y = m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13];
            //float z = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]; // I don't need z
            float w = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
            x /= w;
            y /= w;
            x = 0.5f * x + 0.5f;
            y = 0.5f * y + 0.5f;

            if (x >= minX && x <= maxX && y >= minY && y <= maxY) {
                select[i] = true;
            }
        }

        return 0;
    }


    int PointCloudPicker::pick_vertices_gpu(PointCloud *model, int xmin, int xmax, int ymin, int ymax, bool deselect, ShaderProgram *program) {
        if (!model)
            return 0;

        LOG(WARNING) << "not implemented yet";
        return 0;

        auto drawable = model->points_drawable("vertices");
//        int num_before = drawable->num_selected();

        int viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        int width = viewport[2];
        int height = viewport[3];
        vec4 rect((float) xmin, (float) xmax, (float) ymin, (float) ymax);
        const mat4 &MVP = camera()->modelViewProjectionMatrix();

        program->bind();
        program->set_uniform("viewport", viewport);
        program->set_uniform("MVP", MVP);
        program->set_uniform("rect", rect);
        program->set_uniform("deselect", deselect);
        drawable->gl_draw(true);
        program->release();

        //-----------------------------------------------------------------
        // Liangliang: this is not needed for rendering purpose, but just to keep the CPU data up to date. A better
        //             choice could be retrieving the data only if needed.
        drawable->fetch_selection_buffer();
        easy3d_debug_gl_error;
        easy3d_debug_frame_buffer_error;
        //-----------------------------------------------------------------

//        int num_changed = std::abs(drawable->num_selected() - num_before);
//        if (num_changed > 0) {
//            // the data in the GPU memory has already been updated by my shader
//            //model->notify_selection_change();
//        }
//        return num_changed;
        return 0;
    }


    int PointCloudPicker::pick_vertices_cpu(PointCloud *model, const std::vector<vec2> &plg, bool deselect) {
        if (!model)
            return 0;

        int win_width = camera()->screenWidth();
        int win_height = camera()->screenHeight();

        float minX = FLT_MAX;
        float minY = FLT_MAX;
        float maxX = -FLT_MAX;
        float maxY = -FLT_MAX;
        std::vector<vec2> region; // the transformed selection region
        for (std::size_t i = 0; i < plg.size(); ++i) {
            const vec2 &p = plg[i];
            minX = std::min(minX, p.x);
            minY = std::min(minY, p.y);
            maxX = std::max(maxX, p.x);
            maxY = std::max(maxY, p.y);

            float x = p.x / float(win_width - 1);
            float y = 1.0f - p.y / float(win_height - 1);
            region.push_back(vec2(x, y));
        }

        minX = minX / float(win_width - 1);
        minY = 1.0f - minY / float(win_height - 1);
        maxX = maxX / float(win_width - 1);
        maxY = 1.0f - maxY / float(win_height - 1);
        if (minX > maxX)
            std::swap(minX, maxX);
        if (minY > maxY)
            std::swap(minY, maxY);

        const auto& points = model->get_vertex_property<vec3>("v:point").vector();
        int num = static_cast<int>(points.size());
        const mat4 &m = camera()->modelViewProjectionMatrix();

        auto& select = model->vertex_property<bool>("v:select").vector();

#pragma omp parallel for
        for (int i = 0; i < num; ++i) {
            const vec3 &p = points[i];
            float x = m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12];
            float y = m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13];
            //float z = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]; // I don't need z
            float w = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
            x /= w;
            y /= w;
            x = 0.5f * x + 0.5f;
            y = 0.5f * y + 0.5f;

            if (x >= minX && x <= maxX && y >= minY && y <= maxY) {
                if (geom::point_in_polygon(vec2(x, y), region))
                    select[i] = true;
            }
        }

//        int num_changed = std::abs(drawable->num_selected() - num_before);
//        if (num_changed > 0)
//            model->notify_selection_change();
//        return num_changed;

        return 0;
    }


    int PointCloudPicker::pick_vertices_gpu(PointCloud *model, const std::vector<vec2> &plg, bool deselect, ShaderProgram *program) {
        if (!model)
            return 0;

        LOG(WARNING) << "not implemented yet";
        return 0;

        auto drawable = model->points_drawable("vertices");
//        int num_before = drawable->num_selected();

//        // make sure the vertex buffer holds the right data.
//        if (drawable->num_primitives() != model->n_vertices()) {
//            model->notify_vertex_change();
//            model->active_renderer()->ensure_buffers(drawable);
//        }
//        drawable->update_storage_buffer(plg.data(), plg.size() * sizeof(vec2));

        int viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        int width = viewport[2];
        int height = viewport[3];

        program->bind();
        program->set_uniform("viewport", viewport);
        program->set_uniform("MVP", camera()->modelViewProjectionMatrix());
        program->set_uniform("deselect", deselect);
        drawable->gl_draw(true);
        program->release();

        //-----------------------------------------------------------------
        // Liangliang: this is not needed for rendering purpose, but just to keep the CPU data up to date. A better
        //             choice could be retrieving the data only if needed.
        drawable->fetch_selection_buffer();
        easy3d_debug_gl_error;
        easy3d_debug_frame_buffer_error;
        //-----------------------------------------------------------------

//        int num_changed = std::abs(drawable->num_selected() - num_before);
//        if (num_changed > 0) {
//            // the data in the GPU memory has already been updated by my shader
//            //model->notify_selection_change();
//        }
//        return num_changed;
        return 0;
    }

}