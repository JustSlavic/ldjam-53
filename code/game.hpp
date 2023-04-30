#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include <base.hpp>

#include <array.hpp>
#include <math/integer.hpp>
#include <math/vector2.hpp>
#include <math/vector3.hpp>
#include <math/rectangle2.hpp>
#include <rs/resource_system.hpp>

// namespace game {


#if DEBUG
struct debug_time_measurement;
extern debug_time_measurement *global_debug_measurements;
extern uint32 global_debug_call_depth;

#define DEBUG_BEGIN_TIME_MEASUREMENT(NAME) \
    uint64 debug_begin_time_measurement_##NAME##__ = DEBUG_CYCLE_COUNT(); \
    (void) 0

#define DEBUG_END_TIME_MEASUREMENT(NAME) \
    uint64 debug_end_time_measurement_##NAME##__ = DEBUG_CYCLE_COUNT(); \
    add_measurement(global_debug_measurements + DEBUG_TIME_SLOT_##NAME, debug_end_time_measurement_##NAME##__ - debug_begin_time_measurement_##NAME##__); \
    (void) 0
#else
#define DEBUG_BEGIN_TIME_MEASUREMENT(NAME)
#define DEBUG_END_TIME_MEASUREMENT(NAME)
#endif


struct entity_id
{
    uint32 index;
};

enum entity_type
{
    ENTITY_INVALID = 0,
    ENTITY_SAM,
    ENTITY_PACKAGE,
    ENTITY_GROUND,
    ENTITY_POSTBOX,
    ENTITY_STONE,
};

struct entity
{
    entity_type type;

    math::vector2 position;
    math::vector2 velocity;
    float32 mass;
    float32 width;
    float32 height;

    bool32 collidable;
    bool32 collided;
    bool32 deleted;
};

namespace game {

struct camera {
    math::vector3 position;
};

} // namespace game

struct sam_move
{
    math::vector2 acceleration;
    bool32 jump;
};

struct game_state
{
    memory::allocator game_allocator;

    game::camera default_camera;

    entity *sam;
    entity *postbox;
    entity *ground;

    uint32 carried_packages;
    uint32 score;

    float32 blink_time;
    float32 blink_freq;
    bool32  draw_scanner;
    bool32  jumps_available;

    rs::resource_token rectangle_mesh;
    rs::resource_token rectangle_shader;

    rs::resource_token five_mesh;
    rs::resource_token zero_mesh;
    rs::resource_token ten_mesh;
    rs::resource_token fifty_mesh;

    entity *entities;
    usize entities_capacity;
    usize entity_count;
};


struct entity_ref
{
    entity *e;
    uint32 eid;
};


entity_ref push_entity(game_state *gs);
entity *get_entity(game_state *gs, uint32 eid);


#endif // GAME_STATE_HPP
