#include <game.hpp>
#include <game_interface.hpp>


// @todo:
// + packages
// + pick up packages (backpack)
// - on hit stone loose packages
// - on hit stone teleport onto this stone with the signature blinking
// + sent packages via postbox
// - draw the score number as roman numerals


#include <math/integer.hpp>
#include <math/float64.hpp>
#include <math/rectangle2.hpp>
#include <math/vector4.hpp>
#include <math/matrix2.hpp>
#include <math/matrix4.hpp>

#include <collision.hpp>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <stdio.h>
#define osOutputDebugString(MSG, ...) \
{  \
    char OutputBuffer_##__LINE__[256]; \
    sprintf(OutputBuffer_##__LINE__, MSG, __VA_ARGS__); \
    OutputDebugStringA(OutputBuffer_##__LINE__); \
} void(0)
float uniform_real(float from, float to)
{
    ASSERT(to > from);

    float r = float(rand()) / float(RAND_MAX / to); // Uniform [0, to]
    return r + from;
}
int32 uniform_int(int32 from, int32 to)
{
    ASSERT(to > from);

    int r = rand() % (to - from) + to;
    return r;
}

#if DEBUG
debug_time_measurement *global_debug_measurements;
uint32 global_debug_call_depth;
#endif

GLOBAL float32 package_width = 0.9f;
GLOBAL float32 package_height = 0.2f;
GLOBAL float32 lou_width = 0.3f;
GLOBAL float32 lou_height = 0.5f;


GLOBAL float32 letter_width = 0.5f;
GLOBAL float32 letter_height = 0.8f;
GLOBAL float32 spacing = 0.4f;


GLOBAL math::vector4 sky_color = V4(148.0 / 255.0, 204.0 / 255.0, 209.0 / 255.0, 1.0);
GLOBAL math::vector4 porter_color = V4(55.0/255.0, 70.0/255.0, 122.0/255.0, 1);
GLOBAL math::vector4 ground_color = V4(50.0/255.0, 115.0/255.0, 53.0/255.0, 1);
GLOBAL math::vector4 stones_color = V4(184.0/255.0, 165.0/255.0, 136.0/255.0, 1);
GLOBAL math::vector4 package_color = V4(255.0/255.0, 255.0/255.0, 0.0/255.0, 1);
GLOBAL math::vector4 lou_color = V4(247.0/255.0, 180.0/255.0, 54.0/255.0, 1);

INLINE entity_ref push_entity(game_state *gs)
{
    ASSERT(gs->entity_count < gs->entities_capacity);
    entity_ref result;
    result.eid = (uint32) gs->entity_count++;
    result.e = gs->entities + result.eid;
    return result;
}

INLINE entity *get_entity(game_state *gs, uint32 eid)
{
    ASSERT(eid < gs->entity_count);

    entity *result = gs->entities + eid;
    return result;
}

void draw_aligned_rectangle(execution_context *context, game_state *gs, float32 x, float32 y, float32 half_width, float32 half_height, math::vector4 color)
{
    render_command::command_draw_mesh_with_color draw_aligned_rectangle;
    draw_aligned_rectangle.mesh_token = gs->rectangle_mesh;
    draw_aligned_rectangle.shader_token = gs->rectangle_shader;
    draw_aligned_rectangle.model =
        math::translated(V3(x, y, 0),
        math::scaled(V3(half_width, half_height, 1),
            math::matrix4::identity()));
    draw_aligned_rectangle.color = color;

    push_draw_mesh_with_color_command(context, draw_aligned_rectangle);
}


void draw_0(execution_context *context, game_state *gs, float32 offset_x, float32 offset_y)
{
    render_command::command_draw_mesh_with_color cmd;
    cmd.mesh_token = gs->zero_mesh;
    cmd.shader_token = gs->rectangle_shader;
    cmd.model =
        math::translated(V3(offset_x, offset_y, 0),
        math::scaled(V3(letter_width, letter_height, 1),
            math::matrix4::identity()));
    cmd.color = package_color;

    push_draw_mesh_with_color_command(context, cmd);
}

void draw_1(execution_context *context, game_state *gs, float32 offset_x, float32 offset_y)
{
    draw_aligned_rectangle(context, gs,
        offset_x,
        offset_y,
        letter_width * 0.25f, letter_height,
        package_color);
}

void draw_5(execution_context *context, game_state *gs, float32 offset_x, float32 offset_y)
{
    render_command::command_draw_mesh_with_color cmd;
    cmd.mesh_token = gs->five_mesh;
    cmd.shader_token = gs->rectangle_shader;
    cmd.model =
        math::translated(V3(offset_x, offset_y, 0),
        math::scaled(V3(letter_width, letter_height, 1),
            math::matrix4::identity()));
    cmd.color = package_color;

    push_draw_mesh_with_color_command(context, cmd);
}

void draw_10(execution_context *context, game_state *gs, float32 offset_x, float32 offset_y)
{
    render_command::command_draw_mesh_with_color cmd;
    cmd.mesh_token = gs->ten_mesh;
    cmd.shader_token = gs->rectangle_shader;
    cmd.model =
        math::translated(V3(offset_x, offset_y, 0),
        math::scaled(V3(letter_width, letter_height, 1),
            math::matrix4::identity()));
    cmd.color = package_color;

    push_draw_mesh_with_color_command(context, cmd);
}

void draw_50(execution_context *context, game_state *gs, float32 offset_x, float32 offset_y)
{
    render_command::command_draw_mesh_with_color cmd;
    cmd.mesh_token = gs->fifty_mesh;
    cmd.shader_token = gs->rectangle_shader;
    cmd.model =
        math::translated(V3(offset_x, offset_y, 0),
        math::scaled(V3(letter_width, letter_height, 1),
            math::matrix4::identity()));
    cmd.color = package_color;

    push_draw_mesh_with_color_command(context, cmd);
}


void teleport_back(game_state *gs)
{
    float32 x = gs->sam->position.x;
    for (uint32 entity_index = 1; entity_index < gs->entity_count; entity_index++)
    {
        auto *e = get_entity(gs, entity_index);
        e->position.x -= 100.f;
    }
    gs->default_camera.position.x -= 100.f;
}

void sprinkle_stones(game_state *gs)
{
    float32 x = gs->sam->position.x;
    uint32 sprinkled = 0;
    for (uint32 entity_index = 1; entity_index < gs->entity_count; entity_index++)
    {
        auto *e = get_entity(gs, entity_index);
        if ((e->type == ENTITY_STONE) && (e->position.x < x - 20.f))
        {
            float32 x_mean = (sprinkled + 3) * 3.f;
            e->position = V2(uniform_real(x_mean - 1.f, x_mean + 1.f) + 20.0f, -4. + e->height * 0.5f - uniform_real(0.1f, 0.3f));
            sprinkled += 1;
        }
    }
}


void sprinkle_packages(game_state *gs)
{
    auto x = gs->sam->position.x;
    uint32 sprinkled = 0;
    for (uint32 entity_index = 1; entity_index < gs->entity_count; entity_index++)
    {
        auto *e = get_entity(gs, entity_index);
        if ((e->type == ENTITY_PACKAGE) && (e->position.x < x - 20.f))
        {
            float32 x_mean = (sprinkled + 3) * 3.f;
            e->position = V2(uniform_real(x_mean - 1.f, x_mean + 1.f) + 20.0f, -2);
            e->deleted = false;
            sprinkled += 1;
        }
    }
}


//
// Arguments:
// - execution_context *context;
// - memory_block game_memory;
//
INITIALIZE_MEMORY_FUNCTION(initialize_memory)
{
    using namespace math;

    srand((unsigned int)time(NULL));

    global_debug_measurements = context->debug_measurements;

    ASSERT(sizeof(game_state) < game_memory.size);
    game_state *gs = (game_state *) game_memory.memory;

    memory::initialize_memory_arena(&gs->game_allocator, (byte *) game_memory.memory + sizeof(game_state), game_memory.size - sizeof(game_state));

    gs->entities = (entity *) ALLOCATE_BUFFER_(&gs->game_allocator, sizeof(entity) * 200000);
    gs->entities_capacity = 200000;

    // @note: let zero-indexed entity be 'null entity' representing lack of entity
    gs->entity_count = 1;
    gs->default_camera.position = V3(0, 0, 20);

    float32 vbo_init[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
    };

    uint32 ibo_init[] = {
        0, 1, 2, // first triangle
        2, 3, 0, // second triangle
    };

    gfx::vertex_buffer_layout vbl = {};
    gfx::push_layout_element(&vbl, 3);

    auto vbo = ALLOCATE_BLOCK_(&context->temporary_allocator, sizeof(vbo_init));
    memory::copy(vbo.memory, vbo_init, sizeof(vbo_init));

    auto ibo = ALLOCATE_BLOCK_(&context->temporary_allocator, sizeof(ibo_init));
    memory::copy(ibo.memory, ibo_init, sizeof(ibo_init));

    gs->rectangle_mesh = create_mesh_resource(&context->resource_storage, vbo, ibo, vbl);
    gs->rectangle_shader = create_shader_resource(&context->resource_storage, STRID("rectangle.shader"));

    gs->blink_freq = 2.0f;

    // Mesh for letter V
    {
        float32 V_vbo[] = {
            -1.0f,  1.0f, 0.0f,
            -0.3f, -1.0f, 0.0f,
             0.3f, -1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,
             0.6f,  1.0f, 0.0f,
             0.0f, -0.7f, 0.0f,
            -0.6f,  1.0f, 0.0f,
        };

        auto _V_vbo = ALLOCATE_BLOCK_(&context->temporary_allocator, sizeof(V_vbo));
        memory::copy(_V_vbo.memory, V_vbo, sizeof(V_vbo));

        uint32 V_ibo[] = {
            0, 1, 6,
            1, 5, 6,
            5, 2, 3,
            5, 3, 4,
            1, 2, 5,
        };

        auto _V_ibo = ALLOCATE_BLOCK_(&context->temporary_allocator, sizeof(V_ibo));
        memory::copy(_V_ibo.memory, V_ibo, sizeof(V_ibo));

        gs->five_mesh = create_mesh_resource(&context->resource_storage, _V_vbo, _V_ibo, vbl);
    }

    // Mesh for letter L
    {
        float32 L_vbo[] = {
            -1.0f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
             1.0f, -0.7f, 0.0f,
            -0.6f, -0.7f, 0.0f,
            -0.6f,  1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
        };

        auto _L_vbo = ALLOCATE_BLOCK_(&context->temporary_allocator, sizeof(L_vbo));
        memory::copy(_L_vbo.memory, L_vbo, sizeof(L_vbo));

        uint32 L_ibo[] = {
            0, 1, 2,
            0, 2, 3,
            0, 3, 4,
            0, 4, 5,
        };

        auto _L_ibo = ALLOCATE_BLOCK_(&context->temporary_allocator, sizeof(L_ibo));
        memory::copy(_L_ibo.memory, L_ibo, sizeof(L_ibo));

        gs->fifty_mesh = create_mesh_resource(&context->resource_storage, _L_vbo, _L_ibo, vbl);
    }

    // Mesh for letter O
    {
        float32 O_vbo[] = {
            -1.0f,  1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,
            -0.5f,  0.6f, 0.0f,
            -0.5f, -0.6f, 0.0f,
             0.5f, -0.6f, 0.0f,
             0.5f,  0.6f, 0.0f,
        };

        auto _O_vbo = ALLOCATE_BLOCK_(&context->temporary_allocator, sizeof(O_vbo));
        memory::copy(_O_vbo.memory, O_vbo, sizeof(O_vbo));

        uint32 O_ibo[] = {
            0, 1, 4,
            1, 5, 4,
            1, 2, 5,
            2, 6, 5,
            6, 2, 3,
            3, 7, 6,
            0, 4, 7,
            7, 3, 0,
        };

        auto _O_ibo = ALLOCATE_BLOCK_(&context->temporary_allocator, sizeof(O_ibo));
        memory::copy(_O_ibo.memory, O_ibo, sizeof(O_ibo));

        gs->zero_mesh = create_mesh_resource(&context->resource_storage, _O_vbo, _O_ibo, vbl);
    }

    // Mesh for letter X
    {
        float32 X_vbo[] = {
            -1.0f, -1.0f, 0.0f,
            -0.5f, -1.0f, 0.0f,
             0.0f, -0.3f, 0.0f,
             0.5f, -1.0f, 0.0f,
             1.0f, -1.0f, 0.0f,
             0.3f,  0.0f, 0.0f,
             1.0f,  1.0f, 0.0f,
             0.5f,  1.0f, 0.0f,
             0.0f,  0.3f, 0.0f,
            -0.5f,  1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,
            -0.3f,  0.0f, 0.0f,
        };

        auto _X_vbo = ALLOCATE_BLOCK_(&context->temporary_allocator, sizeof(X_vbo));
        memory::copy(_X_vbo.memory, X_vbo, sizeof(X_vbo));

        uint32 X_ibo[] = {
            0, 1, 11,
            1, 2, 11,
            2, 3, 4,
            4, 5, 2,
            2, 5, 11,
            11, 5, 8,
            5, 6, 7,
            5, 7, 8,
            8, 9, 10,
            10, 11, 8,
        };

        auto _X_ibo = ALLOCATE_BLOCK_(&context->temporary_allocator, sizeof(X_ibo));
        memory::copy(_X_ibo.memory, X_ibo, sizeof(X_ibo));

        gs->ten_mesh = create_mesh_resource(&context->resource_storage, _X_vbo, _X_ibo, vbl);
    }

    auto sam_ref = push_entity(gs);
    sam_ref.e->type = ENTITY_SAM;
    sam_ref.e->position = V2(0);
    sam_ref.e->width = 0.5f; // meters
    sam_ref.e->height = 1.85f; // meters
    sam_ref.e->mass = 80.0f; // kg
    sam_ref.e->collidable = true;

    gs->sam = sam_ref.e; // @IMPORTANT!

    auto ground = push_entity(gs);
    ground.e->type = ENTITY_GROUND;
    ground.e->position = V2(0., -6.);
    ground.e->width = 200.f;
    ground.e->height = 4.f;
    ground.e->mass = 100000.0f;
    ground.e->collidable = true;

    gs->ground = ground.e; // @IMPORTANT!

    auto postbox = push_entity(gs);
    postbox.e->type = ENTITY_POSTBOX;
    postbox.e->position = V2(100, -3);
    postbox.e->width = 1.f;
    postbox.e->height = 2.f;
    postbox.e->mass = 9000.f;
    postbox.e->collidable = true;

    gs->postbox = postbox.e; // @IMPORTANT!

    for (int i = 0; i < 50; i++)
    {
        auto stone = push_entity(gs);
        stone.e->type = ENTITY_STONE;
        float32 x_mean = (i + 2) * 1.83f;
        stone.e->width = uniform_real(0.5f, 0.8f);
        stone.e->height = ((float32) uniform_int(2, 5)) * 0.1f;
        stone.e->position = V2(uniform_real(x_mean - 0.5f, x_mean + 0.5f), -4. + stone.e->height * 0.5f - uniform_real(0.1f, 0.3f));
        stone.e->collidable = true;
    }

    for (int i = 0; i < 20; i++)
    {
        auto package = push_entity(gs);
        package.e->type = ENTITY_PACKAGE;
        float32 x_mean = (i + 3) * 3.f;
        package.e->position = V2(uniform_real(x_mean - 1.f, x_mean + 1.f), -2);
        package.e->width = package_width;
        package.e->height = package_height;
        package.e->collidable = true;
    }
}

//
// Arguments:
// - execution_context *context;
// - memory_block game_memory;
// - input_devices input;
// - float32 dt;
//
UPDATE_AND_RENDER_FUNCTION(update_and_render)
{
    using namespace math;

    global_debug_measurements = context->debug_measurements;

    DEBUG_BEGIN_TIME_MEASUREMENT(update_and_render);

    game_state *gs = (game_state *) game_memory.memory;
    sam_move move_data = {};

    if (get_press_count(input->keyboard_device[keyboard::esc]))
    {
        push_execution_command(context, exit_command());
    }

    // Balance
    if (get_hold_count(input->keyboard_device[keyboard::a]))
    {
    }
    if (get_hold_count(input->keyboard_device[keyboard::d]))
    {
    }

    // Move
    move_data.acceleration.x = 0.2f;
    if (get_press_count(input->keyboard_device[keyboard::space]))
    {
        if (gs->jumps_available > 0)
        {
            move_data.acceleration.y += (1.5f / (0.1f * gs->carried_packages + 1)) / dt;
            move_data.jump = true;
            gs->jumps_available -= 1;
        }
    }

    // osOutputDebugString("move_data = {%4.2f, %4.2f}\n", move_data.acceleration.x, move_data.acceleration.y);

    if (gs->sam->position.x > 50.f && gs->sam->position.x < 60.f)
        gs->postbox->position.x = 100.0f;

    gs->default_camera.position.x = gs->sam->position.x + 8.f;

    // Setup camera
    {
        render_command::command_setup_camera setup_camera;
        setup_camera.camera_position = gs->default_camera.position;
        setup_camera.look_at_position = V3(gs->default_camera.position.x, gs->default_camera.position.y, 0);
        setup_camera.camera_up_direction = V3(0, 1, 0);

        push_setup_camera_command(context, setup_camera);
    }

    // Background
    {
        render_command::command_draw_background draw_background;
        draw_background.mesh = gs->rectangle_mesh;
        draw_background.shader = gs->rectangle_shader;
        draw_background.color = sky_color;
        push_draw_background_command(context, draw_background);
    }

    // Coordinates
    {
        // X axis
        draw_aligned_rectangle(context, gs, 0.5f, 0.f, 0.5f, 0.005f, V4(0.9, 0.2, 0.2, 1.0));
        // Y axis
        draw_aligned_rectangle(context, gs, 0.f, 0.5f, 0.005f, 0.5f, V4(0.2, 0.9, 0.2, 1.0));
    }

#if 1
    auto gravity = V2(0, -0.3); // m/s^2
#else
    auto gravity = V2(0, 0); // m/s^2
#endif

    // osOutputDebugString("old v = {%4.2f, %4.2f}\n", gs->sam->velocity.x, gs->sam->velocity.y);

    for (uint32 entity_index = 1; entity_index < gs->entity_count; entity_index++)
    {
        entity *e = get_entity(gs, entity_index);
        if (e->deleted) continue;

        auto old_v_ = e->velocity;

        if ((e->type == ENTITY_SAM && gs->blink_time <= 0.f) || e->type == ENTITY_PACKAGE)
        {
            float32 dt_ = dt; // * 0.05f;
            for (int move = 0; move < 5; move++)
            {
                auto acceleration = gravity;
                if (e->type == ENTITY_SAM)
                {
                    float32 friction_coefficient = 0.5f;
                    float32 friction = friction_coefficient * (-e->velocity.x);
                    // osOutputDebugString("friction=%4.2f  ", friction);
                    acceleration += (move_data.acceleration + V2(friction, 0));
                }

                auto old_v = e->velocity;
                e->velocity = e->velocity + acceleration * dt_;
                // osOutputDebugString("a=(%4.2f, %4.2f);\n", acceleration.x, acceleration.y);

                auto old_p = e->position;
                auto new_p = e->position + old_v * dt_;

                if (new_p.y < gs->ground->position.y + gs->ground->position.y * 0.5f)
                {
                    new_p.y = gs->ground->position.y + gs->ground->position.y * 0.5f;
                }

                auto full_distance = 0.f;
                auto direction = normalized(new_p - old_p, &full_distance);
                auto distance = full_distance;
                if (distance == 0) continue;

                collision_data collision = no_collision();

                for (uint32 test_eid = 1; test_eid < gs->entity_count; test_eid++)
                {
                    // Do not self collide!
                    if (entity_index == test_eid) continue;
                    entity *test_entity = get_entity(gs, test_eid);
                    if (test_entity->deleted) continue;

                    if (!test_entity->collidable) continue;
                    auto minkowski_width = (e->width + test_entity->width) * 0.5f;
                    auto minkowski_height = (e->height + test_entity->height) * 0.5f;

                    if (math::absolute(new_p.x - test_entity->position.x) < minkowski_width &&
                        math::absolute(new_p.y - test_entity->position.y) < minkowski_height)
                    {
                        // Collided
                        auto tl = V2(test_entity->position.x - minkowski_width,
                                     test_entity->position.y + minkowski_height);
                        auto bl = V2(test_entity->position.x - minkowski_width,
                                     test_entity->position.y - minkowski_height);
                        auto tr = V2(test_entity->position.x + minkowski_width,
                                     test_entity->position.y + minkowski_height);
                        auto br = V2(test_entity->position.x + minkowski_width,
                                     test_entity->position.y - minkowski_height);

                        math::vector2 vertices[5] = { bl, tl, tr, br, bl };

                        for (uint32 i = 0; i < ARRAY_COUNT(vertices) - 1; i++)
                        {
                            auto seg_1a = old_p;
                            auto seg_1b = new_p;

                            auto seg_2a = vertices[i];
                            auto seg_2b = vertices[i + 1];

                            auto wall = (seg_2b - seg_2a);
                            auto normal = normalized(V2(-wall.y, wall.x));

                            if (dot(new_p - old_p, normal) < 0.f)
                            {
                                collision_data temp_collision = {};

                                float32 t1, t2;
                                math::vector2 c1, c2;
                                float32 sq_distance = sq_distance_segment_segment(
                                    seg_1a, seg_1b, seg_2a, seg_2b,
                                    t1, t2, c1, c2);

                                collision.entity1 = e;
                                collision.entity2 = test_entity;
                                collision.normal = normal;
                                collision.t_in_meters = 0;
                            }
                        }
                    }
                }

                if (collision.t_in_meters < math::infinity)
                {
                    if (collision.entity1->type == ENTITY_SAM && collision.entity2->type == ENTITY_PACKAGE)
                    {
                        collision.entity2->deleted = true;
                        gs->carried_packages += 1;
                    }
                    else if (collision.entity1->type == ENTITY_SAM && collision.entity2->type == ENTITY_POSTBOX)
                    {
                        gs->score += gs->carried_packages;
                        gs->carried_packages = 0;

                        if (gs->sam->position.x > 50.f)
                        {
                            teleport_back(gs);
                            sprinkle_stones(gs);
                            sprinkle_packages(gs);
                            new_p = gs->sam->position;
                        }

                    }
                    else if ((collision.entity1->type == ENTITY_SAM || collision.entity1->type == ENTITY_PACKAGE)
                        && (collision.entity2->type == ENTITY_STONE || collision.entity2->type == ENTITY_GROUND))
                    {
                        new_p = old_p + collision.t_in_meters * direction;

                        direction = normalized(new_p - old_p, &distance);

                        auto velocity_projection = dot(collision.entity1->velocity, collision.normal);
                        e->velocity = (collision.entity1->velocity - velocity_projection * collision.normal);

                        if (e->type == ENTITY_SAM)
                            gs->jumps_available = 1;
                    }

                    if (collision.entity1->type == ENTITY_SAM || collision.entity1->type == ENTITY_PACKAGE)
                    if (new_p.y < (gs->ground->position.y + gs->ground->height / 2 + collision.entity1->height / 2))
                    {
                        new_p.y = (gs->ground->position.y + gs->ground->height / 2 + collision.entity1->height / 2);
                    }
                    if (collision.entity1->type == ENTITY_SAM)
                    {
                        if (e->velocity.x < EPSILON)
                        {
                            gs->carried_packages /= 2;
                            if (collision.entity2->type == ENTITY_STONE)
                            {
                                new_p.y = collision.entity2->position.y + collision.entity2->height * 0.5f + e->height * 0.5f + 2.f;
                                gs->blink_time = 15.f;
                                gs->jumps_available = 1;
                            }
                        }

                        if (collision.entity2->type != ENTITY_STONE &&
                            collision.entity2->type != ENTITY_PACKAGE &&
                            collision.entity2->type != ENTITY_POSTBOX)
                        {
                            if (new_p.y < (collision.entity2->position.y + collision.entity2->height / 2 + collision.entity1->height / 2))
                            {
                                new_p.y = (collision.entity2->position.y + collision.entity2->height / 2 + collision.entity1->height / 2);
                            }
                        }
                    }
                }

                e->position = new_p;
                gs->ground->position.x = gs->sam->position.x;

                auto ddt = dt_ * (distance / full_distance);
                dt_ -= ddt;
                if (dt_ < EPSILON) break;
            }
        }


        switch (e->type)
        {
            case ENTITY_SAM:
            {
                float32 blink_phase = gs->blink_time / gs->blink_freq;
                if (((int) blink_phase) % 2 > 0) continue;

                draw_aligned_rectangle(
                    context, gs,
                    e->position.x, e->position.y,
                    e->width * 0.5f, e->height * 0.5f,
                    // e->collided ? V4(1, 0, 0, 1) :
                    // e->on_ground ? V4(0, 0, 0, 1) :
                    porter_color);

                draw_aligned_rectangle(
                    context, gs,
                    e->position.x + 0.05f, e->position.y + 1.0f,
                    e->width * 0.5f, 0.35f,
                    porter_color);

                draw_aligned_rectangle(
                    context, gs,
                    e->position.x + 0.2f, e->position.y + 1.0f,
                    0.05f, 0.3f,
                    V4(242.0/255.0, 242.0/255.0, 218.0/255.0, 1));
                draw_aligned_rectangle(
                    context, gs,
                    e->position.x - 0.1f, e->position.y + 0.2f,
                    0.025f, 0.4f,
                    V4(0.9, 0.2, 0.2, 1.0));
                draw_aligned_rectangle(
                    context, gs,
                    e->position.x, e->position.y - 0.4f,
                    0.025f, 0.5f,
                    V4(0.9, 0.2, 0.2, 1.0));

                draw_aligned_rectangle(
                    context, gs,
                    e->position.x + 0.1f, e->position.y - 0.85f,
                    0.2f, 0.1f,
                    V4(0, 0, 0, 1));

                // Lou
                draw_aligned_rectangle(
                    context, gs,
                    e->position.x + 0.3f, e->position.y + 0.06f,
                    lou_width * 0.5f, lou_height * 0.5f,
                    lou_color);

                // Packages
                float32 backpack_x_offset = 0.4f;
                float32 backpack_y_offset = 0.2f;

                for (uint32 i = 0; i < gs->carried_packages; i++)
                {
                    if (i < 4)
                    {
                        draw_aligned_rectangle(
                            context, gs,
                            e->position.x - backpack_x_offset - i * 0.25f, e->position.y + backpack_y_offset,
                            package_height * 0.5f, package_width * 0.5f,
                            package_color);
                    }
                    else
                    {
                        draw_aligned_rectangle(
                            context, gs,
                            e->position.x - backpack_x_offset - 0.4f, e->position.y + 0.6f + (i - 4) * 0.25f + backpack_y_offset,
                            package_width * 0.5f, package_height * 0.5f,
                            package_color);
                    }
                }

                // Scanner
                {
                    draw_aligned_rectangle(
                        context, gs,
                        e->position.x - 0.4f, e->position.y + 0.5f,
                        0.175f, 0.05f,
                        porter_color);

                    draw_aligned_rectangle(
                        context, gs,
                        e->position.x - 0.5f, e->position.y + 1.f,
                        0.05f, 0.5f,
                        porter_color);
                    draw_aligned_rectangle(
                        context, gs,
                        e->position.x - 0.4f, e->position.y + 1.2f,
                        0.05f, 0.3f,
                        V4(1., 1., 1., 1.0f));
                }
            }
            break;

            case ENTITY_GROUND:
            {
                draw_aligned_rectangle(
                    context, gs,
                    e->position.x, e->position.y,
                    e->width * 0.5f, e->height * 0.5f,
                    e->collided ? V4(1, 0, 0, 1) :
                    ground_color);
            }
            break;

            case ENTITY_POSTBOX:
            {
                draw_aligned_rectangle(
                    context, gs,
                    e->position.x, e->position.y,
                    e->width * 0.5f, e->height * 0.5f,
                    porter_color);
            }
            break;

            case ENTITY_STONE:
            {
                draw_aligned_rectangle(
                    context, gs,
                    e->position.x, e->position.y,
                    e->width * 0.5f, e->height * 0.5f,
                    stones_color);
            }
            break;

            case ENTITY_PACKAGE:
            {
                draw_aligned_rectangle(
                    context, gs,
                    e->position.x, e->position.y,
                    e->width * 0.5f, e->height * 0.5f,
                    package_color);
            }
            break;

            case ENTITY_INVALID:
                ASSERT_FAIL();
            break;
        }
    }

    // Score
    {
        float32 offset_x = gs->default_camera.position.x - 10.0f;
        float32 offset_y = gs->default_camera.position.y + 5.0f;

        if (gs->score == 0)
        {
            draw_0(context, gs, offset_x, offset_y);
        }
        else
        {
            uint32 score = gs->score;
            while(score > 0)
            {
                if (score >= 50)
                {
                    // draw L
                    draw_50(context, gs, offset_x, offset_y);
                    score -= 50;
                }
                else if (score >= 40)
                {
                    // draw XL
                    draw_10(context, gs, offset_x, offset_y);
                    offset_x += letter_width + spacing;
                    draw_50(context, gs, offset_x, offset_y);
                    score -= 40;
                }
                else if (score >= 10)
                {
                    // draw X
                    draw_10(context, gs, offset_x, offset_y);
                    score -= 10;
                }
                else if (score >= 9)
                {
                    // draw IX
                    draw_1(context, gs, offset_x, offset_y);
                    offset_x += letter_width + spacing;
                    draw_10(context, gs, offset_x, offset_y);
                    score -= 9;
                }
                else if (score >= 5)
                {
                    // draw V
                    draw_5(context, gs, offset_x, offset_y);
                    score -= 5;
                }
                else if (score >= 4)
                {
                    // draw IV
                    draw_1(context, gs, offset_x, offset_y);
                    offset_x += letter_width + spacing;
                    draw_5(context, gs, offset_x, offset_y);
                    score -= 4;
                }
                else
                {
                    // draw I
                    draw_1(context, gs, offset_x, offset_y);
                    score -= 1;
                }

                offset_x += letter_width + spacing;
            }
        }
    }

    gs->blink_time -= dt;

    DEBUG_END_TIME_MEASUREMENT(update_and_render);
}


// #include <memory/allocator.cpp>
// #include <string_id.cpp>
// #include <rs/resource_system.cpp>
#include <execution_context.cpp>
