#include "paint_canvas.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QTimer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <easy3d/core/surface_mesh.h>
#include <easy3d/core/point_cloud.h>
#include <easy3d/viewer/drawable.h>
#include <easy3d/viewer/shader_program.h>
#include <easy3d/viewer/primitives.h>
#include <easy3d/viewer/transform.h>
#include <easy3d/viewer/camera.h>
#include <easy3d/viewer/manipulated_camera_frame.h>
#include <easy3d/viewer/shader_manager.h>
#include <easy3d/viewer/ambient_occlusion_hbao.h>
#include <easy3d/viewer/soft_shadow.h>
#include <easy3d/viewer/dual_depth_peeling.h>
#include <easy3d/viewer/eye_dome_lighting.h>
#include <easy3d/viewer/read_pixel.h>
#include <easy3d/viewer/opengl_info.h>
#include <easy3d/viewer/opengl_error.h>
#include <easy3d/viewer/opengl_timer.h>
#include <easy3d/viewer/setting.h>
#include <easy3d/util/logging.h>


using namespace easy3d;


PaintCanvas::PaintCanvas(QWidget* parent /* = nullptr*/)
    : QOpenGLWidget (parent)
	, func_(nullptr)
    , gpu_timer_(nullptr)
    , gpu_time_(0.0)
    , camera_(nullptr)
    , pressed_button_(Qt::NoButton)
    , mouse_pressed_pos_(0, 0)
    , mouse_previous_pos_(0, 0)
    , show_corner_axes_(true)
    , axes_(nullptr)
	, show_pivot_point_(false)
    , model_idx_(-1)
    , ssao_(nullptr)
    , transparency_(nullptr)
    , transparency_enabled_(false)
    , shadow_(nullptr)
    , shadow_enabled_(false)
    , edl_(nullptr)
    , edl_enabled_(false)
{
	// like Qt::StrongFocus plus the widget accepts focus by using the mouse wheel.
    setFocusPolicy(Qt::WheelFocus);
    setMouseTracking(true);

    camera_ = new Camera;
    camera_->setType(Camera::PERSPECTIVE);
    camera_->setScreenWidthAndHeight(width(), height());
    camera_->setViewDirection(vec3(0.0, 1.0, 0.0));
    camera_->showEntireScene();
}


PaintCanvas::~PaintCanvas() {
    // Make sure the context is current and then explicitly
    // destroy all underlying OpenGL resources.
    makeCurrent();

    cleanup();

    doneCurrent();
}


void PaintCanvas::cleanup() {
    if (camera_)
        delete camera_;

    if (axes_)
        delete axes_;

    for (auto m : models_)
        delete m;

    if (ssao_)
        delete ssao_;

    if (shadow_)
        delete shadow_;

    if (transparency_)
        delete transparency_;

    if (edl_)
        delete edl_;

    ShaderManager::terminate();
}


void PaintCanvas::init() {
}


void PaintCanvas::initializeGL()
{
    QOpenGLWidget::initializeGL();
	func_ = context()->functions();
	func_->initializeOpenGLFunctions();

    OpenglInfo::init();
#ifndef NDEBUG
    opengl::setup_gl_debug_callback();
#endif

    if (!func_->hasOpenGLFeature(QOpenGLFunctions::Multisample))
        LOG(WARNING) << "Multisample not supported on this machine!!! Mapple may not run properly";
    if (!func_->hasOpenGLFeature(QOpenGLFunctions::Framebuffers))
        LOG(WARNING) << "Framebuffer Object is not supported on this machine!!! Mapple may not run properly";

    gpu_timer_ = new OpenGLTimer(false);

    background_color_ = vec4(1.0f, 1.0f, 1.0f, 1.0f);

    func_->glEnable(GL_DEPTH_TEST);

    func_->glClearDepthf(1.0f);
    func_->glClearColor(background_color_[0], background_color_[1], background_color_[2], background_color_[3]);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    dpi_scaling_ = devicePixelRatioF();
#else
    dpi_scaling_ = devicePixelRatio();
#endif

#ifndef NDEBUG
    int major_requested = QSurfaceFormat::defaultFormat().majorVersion();
    int minor_requested = QSurfaceFormat::defaultFormat().minorVersion();
    LOG(INFO) << "OpenGL version requested: " << major_requested << "." << minor_requested;
    LOG(INFO) << "Supported OpenGL version: " << func_->glGetString(GL_VERSION);
    LOG(INFO) << "Supported GLSL version:   " << func_->glGetString(GL_SHADING_LANGUAGE_VERSION);

    int major = 0;  func_->glGetIntegerv(GL_MAJOR_VERSION, &major);
    int minor = 0;  func_->glGetIntegerv(GL_MINOR_VERSION, &minor);
    if (major * 10 + minor < 32) {
        throw std::runtime_error("Mapple requires at least OpenGL 3.2");
    }

    // This won't work because QOpenGLWidget draws everying in framebuffer and
    // the framebuffer has not been created in the initializeGL() method. We
    // will query the actual samples in the paintGL() method.
//    //int samples_received = 0;
//    //func_->glGetIntegerv(GL_SAMPLES, &samples_received);
#endif

    std::cout << usage() << std::endl;

    // Calls user defined method.
    init();
}


void PaintCanvas::resizeGL(int w, int h)
{
    QOpenGLWidget::resizeGL(w, h);

    // The viewport is set up by QOpenGLWidget before drawing.
    // So I don't need to set up.
    // func_->glViewport(0, 0, static_cast<int>(w * highdpi_), static_cast<int>(h * highdpi_));

    camera()->setScreenWidthAndHeight(w, h);
}


void PaintCanvas::setBackgroundColor(const vec4 &c) {
    background_color_ = c;

    makeCurrent();
    func_->glClearColor(background_color_[0], background_color_[1], background_color_[2], background_color_[3]);
    doneCurrent();
}


void PaintCanvas::mousePressEvent(QMouseEvent* e) {
    pressed_button_ = e->button();
    mouse_previous_pos_ = e->pos();
    mouse_pressed_pos_ = e->pos();

    camera_->frame()->action_start();
    if (e->button() == Qt::RightButton && e->modifiers() == Qt::ShiftModifier) {
        bool found = false;
        const vec3& p = pointUnderPixel(e->pos(), found);
		if (found) {
			camera_->setPivotPoint(p);

			// show, but hide the visual hint of pivot point after \p delay milliseconds.
			show_pivot_point_ = true;
			const vec3& proj = camera()->projectedCoordinatesOf(camera()->pivotPoint());
            pivot_point_ = QPointF(static_cast<double>(proj.x), static_cast<double>(proj.y));

            const int delay = 2000;
            QTimer::singleShot(delay, [&](){
                show_pivot_point_ = false;
                pivot_point_ = QPointF(0, 0);
                update();
            });
		}
        else {
            camera_->setPivotPoint(camera_->sceneCenter());
            show_pivot_point_ = false;
        }
    }

    QOpenGLWidget::mousePressEvent(e);
    update();
}


void PaintCanvas::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && e->modifiers() == Qt::ControlModifier) { // ZOOM_ON_REGION
        int xmin = std::min(mouse_pressed_pos_.x(), e->pos().x());
        int xmax = std::max(mouse_pressed_pos_.x(), e->pos().x());
        int ymin = std::min(mouse_pressed_pos_.y(), e->pos().y());
        int ymax = std::max(mouse_pressed_pos_.y(), e->pos().y());
        camera_->fitScreenRegion(xmin, ymin, xmax, ymax);
    }
    else
        camera_->frame()->action_end();

    pressed_button_ = Qt::NoButton;
    mouse_pressed_pos_ = QPoint(0, 0);

    QOpenGLWidget::mouseReleaseEvent(e);
    update();
}


void PaintCanvas::mouseMoveEvent(QMouseEvent* e) {
    int x = e->pos().x(), y = e->pos().y();
    if (x < 0 || x > width() || y < 0 || y > height()) {
        e->ignore();
        return;
    }

    if (pressed_button_ != Qt::NoButton) { // button pressed
        // Restrict the cursor to be within the client area during dragging
        if (e->modifiers() == Qt::ControlModifier) {
            // zoom on region
        }
        else {
            int x = e->pos().x();
            int y = e->pos().y();
            int dx = x - mouse_previous_pos_.x();
            int dy = y - mouse_previous_pos_.y();
            if (pressed_button_ == Qt::LeftButton)
                camera_->frame()->action_rotate(x, y, dx, dy, camera_, e->modifiers() == Qt::AltModifier);
            else if (pressed_button_ == Qt::RightButton)
                camera_->frame()->action_translate(x, y, dx, dy, camera_, e->modifiers() == Qt::AltModifier);
            else if (pressed_button_ == Qt::MidButton) {
                if (dy != 0)
                    camera_->frame()->action_zoom(dy > 0 ? 1 : -1, camera_);
            }
        }
    }

    mouse_previous_pos_ = e->pos();
    QOpenGLWidget::mouseMoveEvent(e);
    update();
}


void PaintCanvas::mouseDoubleClickEvent(QMouseEvent* e) {
     QOpenGLWidget::mouseDoubleClickEvent(e);
     update();
}


void PaintCanvas::wheelEvent(QWheelEvent* e) {
    if (e->delta() == 0)
        e->ignore();
    int dy = e->delta() > 0 ? 1 : -1;

     camera_->frame()->action_zoom(dy , camera_);
     update();
}


Model* PaintCanvas::currentModel() const {
    if (models_.empty())
        return nullptr;
    if (model_idx_ < models_.size())
        return models_[model_idx_];
    return nullptr;
}

void PaintCanvas::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_A && e->modifiers() == Qt::NoModifier) {
        show_corner_axes_ = !show_corner_axes_;
    }
    else if (e->key() == Qt::Key_C && e->modifiers() == Qt::NoModifier) {
        if (currentModel())
			fitScreen(currentModel());
    }
    else if (e->key() == Qt::Key_F && e->modifiers() == Qt::NoModifier) {
		fitScreen();
    }
    else if (e->key() == Qt::Key_Left && e->modifiers() == Qt::NoModifier) {
        float angle = static_cast<float>(1 * M_PI / 180.0); // turn left, 1 degrees each step
        camera_->frame()->action_turn(angle, camera_);
    }
    else if (e->key() == Qt::Key_Right && e->modifiers() == Qt::NoModifier) {
        float angle = static_cast<float>(1 * M_PI / 180.0); // turn right, 1 degrees each step
        camera_->frame()->action_turn(-angle, camera_);
    }
    else if (e->key() == Qt::Key_Up && e->modifiers() == Qt::NoModifier) {	// move camera forward
        float step = 0.02f * camera_->sceneRadius();
        camera_->frame()->translate(camera_->frame()->inverseTransformOf(vec3(0.0, 0.0, -step)));
    }
    else if (e->key() == Qt::Key_Down && e->modifiers() == Qt::NoModifier) {// move camera backward
        float step = 0.02f * camera_->sceneRadius();
        camera_->frame()->translate(camera_->frame()->inverseTransformOf(vec3(0.0, 0.0, step)));
    }
    else if (e->key() == Qt::Key_Up && e->modifiers() == Qt::ControlModifier) {	// move camera up
        float step = 0.02f * camera_->sceneRadius();
        camera_->frame()->translate(camera_->frame()->inverseTransformOf(vec3(0.0, step, 0.0)));
    }
    else if (e->key() == Qt::Key_Down && e->modifiers() == Qt::ControlModifier) {	// move camera down
        float step = 0.02f * camera_->sceneRadius();
        camera_->frame()->translate(camera_->frame()->inverseTransformOf(vec3(0.0, -step, 0.0)));
    }

    else if (e->key() == Qt::Key_Minus && e->modifiers() == Qt::ControlModifier)
        camera_->frame()->action_zoom(-1, camera_);
    else if (e->key() == Qt::Key_Equal && e->modifiers() == Qt::ControlModifier)
        camera_->frame()->action_zoom(1, camera_);

    else if (e->key() == Qt::Key_F1 && e->modifiers() == Qt::NoModifier)
        std::cout << usage() << std::endl;
    else if (e->key() == Qt::Key_P && e->modifiers() == Qt::NoModifier) {
        if (camera_->type() == Camera::PERSPECTIVE)
            camera_->setType(Camera::ORTHOGRAPHIC);
        else
            camera_->setType(Camera::PERSPECTIVE);
    }
    else if (e->key() == Qt::Key_Space && e->modifiers() == Qt::NoModifier) {
        // Aligns camera
        Frame frame;
        frame.setTranslation(camera_->pivotPoint());
        camera_->frame()->alignWithFrame(&frame, true);

        // Aligns frame
        //if (manipulatedFrame())
        //	manipulatedFrame()->alignWithFrame(camera_->frame());
    }

    else if (e->key() == Qt::Key_Minus && e->modifiers() == Qt::NoModifier) {
        for (auto m : models_) {
            for (auto d : m->points_drawables()) {
                float size = d->point_size() - 1.0f;
                if (size < 1)
                    size = 1;
                d->set_point_size(size);
            }
        }
    }
    else if (e->key() == Qt::Key_Equal && e->modifiers() == Qt::NoModifier) {
        for (auto m : models_) {
            for (auto d : m->points_drawables()) {
                float size = d->point_size() + 1.0f;
                if (size > 20)
                    size = 20;
                d->set_point_size(size);
            }
        }
    }

    else if (e->key() == Qt::Key_Comma && e->modifiers() == Qt::NoModifier) {
        int pre_idx = model_idx_;
        if (models_.empty())
            model_idx_ = -1;
        else
            model_idx_ = int((model_idx_ - 1 + models_.size()) % models_.size());
        if (model_idx_ != pre_idx) {
            emit currentModelChanged();
            if (model_idx_ >= 0)
                LOG(INFO) << "current model: " << model_idx_ << ", " << models_[model_idx_]->name() << std::endl;
        }
    }
    else if (e->key() == Qt::Key_Period && e->modifiers() == Qt::NoModifier) {
        int pre_idx = model_idx_;
        if (models_.empty())
            model_idx_ = -1;
        else
            model_idx_ = int((model_idx_ + 1) % models_.size());
        if (model_idx_ != pre_idx) {
            emit currentModelChanged();
            if (model_idx_ >= 0)
                LOG(INFO) << "current model: " << model_idx_ << ", " << models_[model_idx_]->name() << std::endl;
        }
    }
    else if (e->key() == Qt::Key_Delete && e->modifiers() == Qt::NoModifier) {
        if (currentModel())
            deleteModel(currentModel());
    }
    else if (e->key() == Qt::Key_W && e->modifiers() == Qt::NoModifier) {
        if (currentModel()) {
            SurfaceMesh* m = dynamic_cast<SurfaceMesh*>(currentModel());
            if (m) {
                LinesDrawable* wireframe = m->lines_drawable("wireframe");
                if (!wireframe) {
                    makeCurrent();
                    wireframe = m->add_lines_drawable("wireframe");
                    std::vector<unsigned int> indices;
                    for (auto e : m->edges()) {
                        SurfaceMesh::Vertex s = m->vertex(e, 0);
                        SurfaceMesh::Vertex t = m->vertex(e, 1);
                        indices.push_back(s.idx());
                        indices.push_back(t.idx());
                    }
                    auto points = m->get_vertex_property<vec3>("v:point");
                    wireframe->update_vertex_buffer(points.vector());
                    wireframe->update_index_buffer(indices);
					wireframe->set_default_color(vec3(0.0f, 0.0f, 0.0f));
					wireframe->set_per_vertex_color(false);
					wireframe->set_visible(true);
                    doneCurrent();
                }
                else
                    wireframe->set_visible(!wireframe->is_visible());
            }
        }
    }
    else if (e->key() == Qt::Key_R && e->modifiers() == Qt::NoModifier) {
        ShaderManager::reload();
    }

    QOpenGLWidget::keyPressEvent(e);
    update();
}


void PaintCanvas::keyReleaseEvent(QKeyEvent* e) {
    QOpenGLWidget::keyReleaseEvent(e);
    update();
}


void PaintCanvas::timerEvent(QTimerEvent* e) {
    QOpenGLWidget::timerEvent(e);
    update();
}


void PaintCanvas::closeEvent(QCloseEvent* e) {
    cleanup();
    QOpenGLWidget::closeEvent(e);
}


std::string PaintCanvas::usage() const {
    return std::string(
        "Mapple viewer usage:												\n"
        "  F1:              Help											\n"
        "  Ctrl + O:        Open file										\n"
        "  Ctrl + S:        Save file										\n"
        "  Left:            Orbit-rotate the camera							\n"
        "  Right:           Move up/down/left/right							\n"
        "  Alt + Left:      Orbit-rotate the camera (screen based)			\n"
        "  Alt + Right:     Move up/down/left/right (screen based)			\n"
        "  Middle/Wheel:    Zoom out/in										\n"
        "  Ctrl + '-'/'+':  Zoom out/in										\n"
        "  F:               Fit screen (all models)                         \n"
        "  C:               Fit screen (current model only)					\n"
        "  Shift + Right:   Set/unset anchor point							\n"
        "  P:               Toggle perspective/orthographic projection)		\n"
        "  A:               Toggle axes										\n"
        "  W:               Toggle wireframe								\n"
        "  < or >:          Switch between models							\n"
    );
}


void PaintCanvas::addModel(Model* model) {
    if (!model)
        return;

    int pre_idx = model_idx_;

    unsigned int num = model->n_vertices();
    if (num == 0) {
        LOG(WARNING) << "Warning: model does not have vertices. Only complete model can be added to the viewer." << std::endl;
        return;
    }

    Box3 box;
    if (dynamic_cast<PointCloud*>(model)) {
        PointCloud* cloud = dynamic_cast<PointCloud*>(model);
        auto points = cloud->get_vertex_property<vec3>("v:point");
        for (auto v : cloud->vertices())
            box.add_point(points[v]);
        model->set_bounding_box(box);
    }
    else if (dynamic_cast<SurfaceMesh*>(model)) {
        SurfaceMesh* mesh = dynamic_cast<SurfaceMesh*>(model);
        auto points = mesh->get_vertex_property<vec3>("v:point");
        for (auto v : mesh->vertices())
            box.add_point(points[v]);
    }
    model->set_bounding_box(box);

    models_.push_back(model);
    model_idx_ = static_cast<int>(models_.size()) - 1; // make the last one current

    if (model_idx_ != pre_idx) {
        emit currentModelChanged();
        if (model_idx_ > 0)
            LOG(INFO) << "current model: " << model_idx_ << ", " << models_[model_idx_]->name() << std::endl;
    }
}


void PaintCanvas::deleteModel(Model* model) {
    if (!model)
        return;

    int pre_idx = model_idx_;

    auto pos = std::find(models_.begin(), models_.end(), model);
    if (pos != models_.end()) {
        const std::string name = model->name();

        models_.erase(pos);
        delete model;
        model_idx_ = static_cast<int>(models_.size()) - 1; // make the last one current

        LOG(INFO) << "model deleted: " << name << std::endl;
    }
    else
        LOG(WARNING) << "no such model: " << model->name() << std::endl;

    if (model_idx_ != pre_idx) {
        emit currentModelChanged();
        if (model_idx_ > 0)
            LOG(INFO) << "current model: " << model_idx_ << ", " << models_[model_idx_]->name() << std::endl;
    }
}


void PaintCanvas::fitScreen(const easy3d::Model *model) {
	if (!model && models_.empty())
		return;

	Box3 box;
	if (model)
		box = model->bounding_box();
	else {
		for (auto m : models_)
			box.add_box(m->bounding_box());
	}
	camera_->setSceneBoundingBox(box.min(), box.max());
	camera_->showEntireScene();
	update();
}


vec3 PaintCanvas::pointUnderPixel(const QPoint& p, bool &found) const
{
    const_cast<PaintCanvas*>(this)->makeCurrent();

    // Qt (same as GLFW) uses upper corner for its origin while GL uses the lower corner.
    int glx = p.x();
    int gly = height() - 1 - p.y();

    // NOTE: when dealing with OpenGL, all positions are relative to the viewer port.
    //       So we have to handle highdpi desplays.
    glx = static_cast<int>(glx * dpi_scaling());
    gly = static_cast<int>(gly * dpi_scaling());

    int samples = 0;
    func_->glGetIntegerv(GL_SAMPLES, &samples); easy3d_debug_gl_error;

    float depth = 1.0f;
    if (samples > 0) {
        opengl::read_depth_ms(depth, glx, gly); easy3d_debug_gl_error;
    }
    else {
        opengl::read_depth(depth, glx, gly); easy3d_debug_gl_error;
    }

    const_cast<PaintCanvas*>(this)->doneCurrent();
    // here the glGetError() won't work because the OpenGL context is not current.
    // easy3d_debug_gl_error;

    found = depth < 1.0f;
    if (found) {
        // The input to unprojectedCoordinatesOf() is defined in the screen coordinate system
        vec3 point(p.x(), p.y(), depth);
        point = camera_->unprojectedCoordinatesOf(point);
        return point;
    }

    return vec3();
}


void PaintCanvas::paintGL() {
    easy3d_debug_gl_error;

#if 1
    // QOpenGLWidget renders everything into a FBO. Internally it changes
    // QSurfaceFormat to always have samples = 0 and the OpenGL context is
    // not a multisample context. So we have to query the render-buffer
    // to know if it is using multisampling. At initializeGL() we were not
    // able to query the actual samples because the internal FBO has not
    // been created yet, so we do it here.
    static bool queried = false;
    if (!queried) {
#if 1
        func_->glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples_);    easy3d_debug_frame_buffer_error;
#else   // the samples can also be retrieved using glGetIntegerv()
        func_->glGetIntegerv(GL_SAMPLES, &samples_); easy3d_debug_gl_error;
#endif
        // warn the user if the expected request was not satisfied
        int samples_requested = QSurfaceFormat::defaultFormat().samples();
        int max_num = 0;
        func_->glGetIntegerv(GL_MAX_SAMPLES, &max_num);
        if (samples_requested > 0 && samples_ != samples_requested) {
            if (samples_ == 0)
                LOG(WARNING) << "MSAA is not available (" << samples_requested << " samples requested)";
            else
                LOG(WARNING) << "MSAA is available with " << samples_ << " samples (" << samples_requested << " requested but max support is "<< max_num << ")";
        }
        else
            LOG(INFO) << "Samples: " << samples_ << " (" << samples_requested << " requested, max support is "<< max_num << ")";
        queried = true;
    }
#endif

    preDraw();

    gpu_timer_->start();
    draw();
    gpu_timer_->stop();
    gpu_time_ = gpu_timer_->time();

    // Add visual hints: axis, camera, grid...
    postDraw();
}


void PaintCanvas::drawCornerAxes() {
    ShaderProgram* program = ShaderManager::get_program("surface_color");
    if (!program) {
        std::vector<ShaderProgram::Attribute> attributes;
        attributes.push_back(ShaderProgram::Attribute(ShaderProgram::POSITION, "vtx_position"));
        attributes.push_back(ShaderProgram::Attribute(ShaderProgram::COLOR, "vtx_color"));
        attributes.push_back(ShaderProgram::Attribute(ShaderProgram::NORMAL, "vtx_normal"));
        program = ShaderManager::create_program_from_files("surface_color", attributes);
    }
    if (!program)
        return;

    if (!axes_) {
        const float base = 0.5f;   // the cylinder length, relative to the allowed region
        const float head = 0.2f;   // the cone length, relative to the allowed region
		std::vector<vec3> points, normals, colors;
		opengl::prepare_cylinder(0.03, 10, vec3(0, 0, 0), vec3(base, 0, 0), vec3(1, 0, 0), points, normals, colors);
		opengl::prepare_cylinder(0.03, 10, vec3(0, 0, 0), vec3(0, base, 0), vec3(0, 1, 0), points, normals, colors);
		opengl::prepare_cylinder(0.03, 10, vec3(0, 0, 0), vec3(0, 0, base), vec3(0, 0, 1), points, normals, colors);
		opengl::prepare_cone(0.06, 20, vec3(base, 0, 0), vec3(base + head, 0, 0), vec3(1, 0, 0), points, normals, colors);
		opengl::prepare_cone(0.06, 20, vec3(0, base, 0), vec3(0, base + head, 0), vec3(0, 1, 0), points, normals, colors);
		opengl::prepare_cone(0.06, 20, vec3(0, 0, base), vec3(0, 0, base + head), vec3(0, 0, 1), points, normals, colors);
		opengl::prepare_sphere(vec3(0, 0, 0), 0.06, 20, 20, vec3(0, 1, 1), points, normals, colors);
        axes_ = new TrianglesDrawable("corner_axes");
        axes_->update_vertex_buffer(points);
        axes_->update_normal_buffer(normals);
        axes_->update_color_buffer(colors);
        axes_->set_per_vertex_color(true);
    }
    // The viewport and the scissor are changed to fit the lower left corner.
    int viewport[4], scissor[4];
    func_->glGetIntegerv(GL_VIEWPORT, viewport);
    func_->glGetIntegerv(GL_SCISSOR_BOX, scissor);

    static const int corner_frame_size = static_cast<int>(100 * dpi_scaling());
    func_->glViewport(0, 0, corner_frame_size, corner_frame_size);
    func_->glScissor(0, 0, corner_frame_size, corner_frame_size);

    // To make the axis appear over other objects: reserve a tiny bit of the
    // front depth range. NOTE: do remember to restore it later.
    func_->glDepthRangef(0, 0.001f);

    const mat4& proj = transform::ortho(-1, 1, -1, 1, -1, 1);
    const mat4& view = camera_->orientation().inverse().matrix();
    const mat4& MVP = proj * view;

    // camera position is defined in world coordinate system.
    const vec3& wCamPos = camera_->position();
    // it can also be computed as follows:
    //const vec3& wCamPos = invMV * vec4(0, 0, 0, 1);
    const mat4& MV = camera_->modelViewMatrix();
    const vec4& wLightPos = inverse(MV) * setting::light_position;

    program->bind();
    program->set_uniform("MVP", MVP);
    program->set_uniform("wLightPos", wLightPos);
    program->set_uniform("wCamPos", wCamPos);
    program->set_uniform("ssaoEnabled", false);
    program->set_uniform("per_vertex_color", true);
    axes_->draw(false);
    program->release();

    // restore
    func_->glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
    func_->glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    func_->glDepthRangef(0.0f, 1.0f);
}


void PaintCanvas::preDraw() {
    // For normal drawing, i.e., drawing triggered by the paintEvent(),
    // the clearing is done before entering paintGL().
    // If you want to reuse the paintGL() method for offscreen rendering,
    // you have to clear both color and depth buffers beforehand.
    //func_->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}


void PaintCanvas::postDraw() {
    // Visual hints: axis, camera, grid...
    if (show_corner_axes_)
        drawCornerAxes();


    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.beginNativePainting();

	char buffer[1024];
	sprintf(buffer, "Rendering (ms): %4.1f", gpu_time_);
	painter.drawText(20, 20, QString::fromStdString(buffer));

    if (show_pivot_point_) {
        painter.setPen(Qt::blue);
        const vec3& p = camera()->pivotPoint();
        painter.drawText(20, 50, QString("Pivot point: (%1, %2, %3)").arg(p.x).arg(p.y).arg(p.z));
        const float size = 10;
		painter.drawLine(pivot_point_.x() - size, pivot_point_.y(), pivot_point_.x() + size, pivot_point_.y());
		painter.drawLine(pivot_point_.x(), pivot_point_.y() - size, pivot_point_.x(), pivot_point_.y() + size);
	}

    painter.endNativePainting();
    painter.end();
    func_->glEnable(GL_DEPTH_TEST); // it seems QPainter disables depth test?
}


AmbientOcclusion_HBAO* PaintCanvas::ssao() {
    if (!ssao_)
        ssao_ = new AmbientOcclusion_HBAO(camera_);
    return ssao_;
}


Shadow* PaintCanvas::shadow() {
    if (!shadow_)
        shadow_ = new SoftShadow(camera_);
    return shadow_;
}


Transparency* PaintCanvas::transparency() {
    if (!transparency_)
        transparency_ = new DualDepthPeeling(camera_);
    return transparency_;
}


easy3d::EyeDomeLighting* PaintCanvas::edl() {
    if (!edl_)
        edl_ = new EyeDomeLighting(camera_);
    return edl_;
}


void PaintCanvas::enableShadow(bool b) {
    shadow_enabled_ = b;
    // shadow and transparency cannot co-exist
    if (shadow_enabled_ && transparency_enabled_)
        transparency_enabled_ = false;
}


void PaintCanvas::enableTransparency(bool b) {
    transparency_enabled_ = b;
    // ssao and transparency cannot co-exist
    if (transparency_enabled_ && ssao())
        ssao()->set_algorithm(AmbientOcclusion_HBAO::SSAO_NONE);
    // shadow and transparency cannot co-exist
    if (transparency_enabled_ && shadow_enabled_)
        shadow_enabled_ = false;
 }


void PaintCanvas::enableEyeDomeLighting(bool b) {
    edl_enabled_ = b;
}


void PaintCanvas::draw() {
    // Optimization tips: rendering with multi-effects (e.g., shadowing, SSAO)
    // can benefit from sharing the same geometry pass.

    std::vector<TrianglesDrawable*> surfaces;
    for (auto m : models_) {
        for (auto d : m->triangles_drawables())
            surfaces.push_back(d);
    }
    if (shadow_enabled_) {
        shadow()->draw(surfaces);
        return;
    }
    else if (transparency_enabled_) {
        transparency()->draw(surfaces);
        return;
    }
    else if (ssao() && ssao()->algorithm() != AmbientOcclusion_HBAO::SSAO_NONE) {
        ssao()->draw(models_, 4);
        return;
    }


    if (models_.empty())
        return;

    // Let's check if wireframe and surfaces are both shown. If true, we
    // make the depth coordinates of the surface smaller, so that displaying
    // the mesh and the surface together does not cause Z-fighting.
    std::size_t count = 0;
    for (auto m : models_) {
        if (!m->is_visible())
            continue;
        for (auto d : m->lines_drawables()) {
            if (d->is_visible())
                ++count;
        }
    }
    if (count > 0) {
        func_->glEnable(GL_POLYGON_OFFSET_FILL);
        func_->glPolygonOffset(0.5f, -0.0001f);
    }

    const mat4& MVP = camera_->modelViewProjectionMatrix();
    // camera position is defined in world coordinate system.
    const vec3& wCamPos = camera_->position();
    // it can also be computed as follows:
    //const vec3& wCamPos = invMV * vec4(0, 0, 0, 1);
    const mat4& MV = camera_->modelViewMatrix();
    const vec4& wLightPos = inverse(MV) * setting::light_position;

    ShaderProgram* program = ShaderManager::get_program("surface_color");
    if (!program) {
        std::vector<ShaderProgram::Attribute> attributes;
        attributes.push_back(ShaderProgram::Attribute(ShaderProgram::POSITION, "vtx_position"));
        attributes.push_back(ShaderProgram::Attribute(ShaderProgram::COLOR, "vtx_color"));
        attributes.push_back(ShaderProgram::Attribute(ShaderProgram::NORMAL, "vtx_normal"));
        program = ShaderManager::create_program_from_files("surface_color", attributes);
    }
    if (program) {
        program->bind();
        program->set_uniform("MVP", MVP);
        program->set_uniform("wLightPos", wLightPos);
        program->set_uniform("wCamPos", wCamPos);
        program->set_uniform("ssaoEnabled", ssao()->algorithm() != AmbientOcclusion_HBAO::SSAO_NONE);
        if (ssao()->algorithm() != AmbientOcclusion_HBAO::SSAO_NONE)
            program->bind_texture("ssaoTexture", ssao_->ssao_texture(), 0);
        for (std::size_t idx = 0; idx < models_.size(); ++idx) {
            Model* m = models_[idx];
            if (!m->is_visible())
                continue;
            for (auto d : m->triangles_drawables()) {
                if (d->is_visible()) {
                    program->set_uniform("per_vertex_color", d->per_vertex_color() && d->color_buffer());
                    program->set_uniform("default_color", d->default_color());
                    d->draw(false);
                }
            }
        }
        if (ssao()->algorithm() != AmbientOcclusion_HBAO::SSAO_NONE)
            program->release_texture();
        program->release();
    }

    if (count > 0)
        func_->glDisable(GL_POLYGON_OFFSET_FILL);

    program = ShaderManager::get_program("lines_color");
    if (!program) {
        std::vector<ShaderProgram::Attribute> attributes;
        attributes.push_back(ShaderProgram::Attribute(ShaderProgram::POSITION, "vtx_position"));
        attributes.push_back(ShaderProgram::Attribute(ShaderProgram::COLOR, "vtx_color"));
        program = ShaderManager::create_program_from_files("lines_color", attributes);
    }
    if (program) {
        program->bind();
        program->set_uniform("MVP", MVP);
        for (auto m : models_) {
            if (!m->is_visible())
                continue;
            for (auto d : m->lines_drawables()) {
                if (d->is_visible()) {
                    program->set_uniform("per_vertex_color", d->per_vertex_color() && d->color_buffer());
                    program->set_uniform("default_color", d->default_color());
                    d->draw(false);
                }
            }
        }
        program->release();
    }

    program = ShaderManager::get_program("points_color");
    if (!program) {
        std::vector<ShaderProgram::Attribute> attributes;
        attributes.push_back(ShaderProgram::Attribute(ShaderProgram::POSITION, "vtx_position"));
        attributes.push_back(ShaderProgram::Attribute(ShaderProgram::COLOR, "vtx_color"));
        attributes.push_back(ShaderProgram::Attribute(ShaderProgram::NORMAL, "vtx_normal"));
        program = ShaderManager::create_program_from_files("points_color", attributes);
    }
    if (program) {

        if (edl_enabled_)
            edl()->begin();

        //glDisable(GL_MULTISAMPLE);
        program->bind();
        program->set_uniform("MVP", MVP);
        program->set_uniform("wLightPos", wLightPos);
        program->set_uniform("wCamPos", wCamPos);
        program->set_uniform("ssaoEnabled", ssao()->algorithm() != AmbientOcclusion_HBAO::SSAO_NONE);
        if (ssao()->algorithm() != AmbientOcclusion_HBAO::SSAO_NONE)
            program->bind_texture("ssaoTexture", ssao_->ssao_texture(), 0);
        for (auto m : models_) {
            if (!m->is_visible())
                continue;
            for (auto d : m->points_drawables()) {
                if (d->is_visible()) {
                    program->set_uniform("lighting", d->normal_buffer());
                    program->set_uniform("per_vertex_color", d->per_vertex_color() && d->color_buffer());
                    program->set_uniform("default_color", d->default_color());
                    d->draw(false);
                }
            }
        }
        if (ssao()->algorithm() != AmbientOcclusion_HBAO::SSAO_NONE) {
            program->release_texture();
    #ifndef NDEBUG
            ssao_->draw_occlusion(200, 10, 400, static_cast<int>(400.0f * height() / width()));
    #endif
        }
        program->release();
        //glEnable(GL_MULTISAMPLE);

        if (edl_enabled_)
            edl()->end();
    }
}
