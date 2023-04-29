#include <game.hpp>
#include <game_interface.hpp>

#include <math/integer.hpp>
#include <math/float64.hpp>
#include <math/rectangle2.hpp>
#include <math/vector4.hpp>
#include <math/matrix2.hpp>
#include <math/matrix4.hpp>

#include <collision.hpp>

#include <windows.h>
#include <stdio.h>
#define osOutputDebugString(MSG, ...) \
{  \
    char OutputBuffer_##__LINE__[256]; \
    sprintf(OutputBuffer_##__LINE__, MSG, __VA_ARGS__); \
    OutputDebugStringA(OutputBuffer_##__LINE__); \
} void(0)

#if DEBUG
debug_time_measurement *global_debug_measurements;
uint32 global_debug_call_depth;
#endif


GLOBAL math::vector4 sky_color = V4(148.0 / 255.0, 204.0 / 255.0, 209.0 / 255.0, 1.0);
GLOBAL math::vector4 porter_color = V4(55.0/255.0, 70.0/255.0, 122.0/255.0, 1);
GLOBAL math::vector4 ground_color = V4(50.0/255.0, 115.0/255.0, 53.0/255.0, 1);
GLOBAL math::vector4 stones_color = V4(184.0/255.0, 165.0/255.0, 136.0/255.0, 1);


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


//
// Arguments:
// - execution_context *context;
// - memory_block game_memory;
//
INITIALIZE_MEMORY_FUNCTION(initialize_memory)
{
    using namespace math;

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
    postbox.e->collidable = false;

    gs->postbox = postbox.e; // @IMPORTANT!

    for (int i = 0; i < 20; i++)
    {
        auto stone = push_entity(gs);
        stone.e->type = ENTITY_STONE;
        stone.e->position = V2((i + 2) * 10.f, -4.);
        stone.e->width = 0.5f;
        stone.e->height = 1.f;
        stone.e->collidable = true;
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
        move_data.acceleration.y += 1.8f;
        move_data.jump = true;
    }

    osOutputDebugString("move_data = {%4.2f, %4.2f}\n", move_data.acceleration.x, move_data.acceleration.y);

    gs->default_camera.position.x = gs->sam->position.x + 7.f;

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
    auto gravity = V2(0, -0.098); // m/s^2
#else
    auto gravity = V2(0, 0); // m/s^2
#endif

    osOutputDebugString("old v = {%4.2f, %4.2f}\n", gs->sam->velocity.x, gs->sam->velocity.y);

    for (uint32 entity_index = 1; entity_index < gs->entity_count; entity_index++)
    {
        entity *e = get_entity(gs, entity_index);
        if (e->type == ENTITY_SAM)
        {
            float32 dt_ = dt; // * 0.05f;
            for (int move = 0; move < 5; move++)
            {
                auto acceleration = gravity;
                {
                    float32 friction_coefficient = 0.5f;
                    float32 friction = friction_coefficient * (-e->velocity.x);
                    osOutputDebugString("friction=%4.2f  ", friction);
                    acceleration += (move_data.acceleration + V2(friction, 0));
                }

                auto old_v = e->velocity;
                e->velocity = e->velocity + acceleration * dt_;
                osOutputDebugString("a=(%4.2f, %4.2f);\n", acceleration.x, acceleration.y);

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
                    new_p = old_p + collision.t_in_meters * direction;
                    direction = normalized(new_p - old_p, &distance);

                    auto velocity_projection = dot(collision.entity1->velocity, collision.normal);
                    e->velocity = (collision.entity1->velocity - velocity_projection * collision.normal);
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
                draw_aligned_rectangle(
                    context, gs,
                    e->position.x, e->position.y,
                    e->width * 0.5f, e->height * 0.5f,
                    e->collided ? V4(1, 0, 0, 1) :
                    // e->on_ground ? V4(0, 0, 0, 1) :
                    porter_color);
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

            case ENTITY_INVALID:
                ASSERT_FAIL();
            break;
        }
    }

    DEBUG_END_TIME_MEASUREMENT(update_and_render);
}


#include <memory/allocator.cpp>
#include <string_id.cpp>
#include <rs/resource_system.cpp>
#include <execution_context.cpp>
