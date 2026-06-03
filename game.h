#pragma once
#include "screens.h"
#include "entities.h"
#include "splashkit-arrays.h"
#include "items.h"
#include "ui.h"
#include <functional>
#include <map>
#include <vector>

class screen;
class player;
class enemy;
class npc;
class item;
struct ticker;
enum class screen_type;

extern int WIN_WIDTH;
extern int WIN_HEIGHT;

int uid();
/**
 * Cumulative normal distribution function, obtained from https://stackoverflow.com/questions/2328258/cumulative-normal-distribution-function-in-c-c and verified with Desmos.
 *
 * Calculates the cumulative probability of a normal distribution at a value `value`.
 *
 * @param value The value to target.
 * @param mean The mean (mu) of the distribution.
 * @param stdev The standard deviation (sigma) of the distribution.
 * @returns Chance of reaching the value or less.
 */
double cndf(double value, double mean, double stdev);

/**
 * An entry on the game's backstack, detailing a combination of menus to be restored to.
 *
 * If `full_active` is true, only `full` is a valid pointer. The other three will be `nullptr`. The reverse is also true.*/
struct backstack_entry final
{
    bool full_active;
    screen *full;
    screen *overlay;
    screen *upper;
    screen *lower;
};

/**
 * Representation and storage for colour palettes.
 *
 * Instances of this class are sets of colours for specific tasks. Instances cannot and should not be generated at runtime (for now). Instead, refer to the static functions that return pointers to predefined colour palettes.
 */
class palette final
{
    palette(string name, color bg, color main, color text);

public:
    static color hp_colour;
    static color mana_colour;
    static color line_colour;
    static color next_room_colour;

    string name;
    color bg;
    color main;
    color text;

    /**
     * @returns A pointer to the default light colour scheme.
     */
    static palette light();
    /**
     * @returns A pointer to the default dark colour scheme.
     */
    static palette dark();

    palette() = default;
};

class room_type
{
public:
    enum types : int
    {
        BATTLE_LV1 = 0,
        BATTLE_LV2 = 1,
        BATTLE_LV3 = 2,
        BATTLE_LV4 = 3,
        MERCHANT = 4,
        CHEST = 5,
        BOSS = 6,
    };

    static std::map<room_type::types, float> room_chances;

    static room_type::types random_type();
};

struct room_schema
{
    const string type = "type";       // int corresponding to room_type
    const string scale_x = "scale_x"; // double
    const string scale_y = "scale_y"; // double
    const string x = "x";             // int
    const string y = "y";             // int
    const string next = "next";       // array of ids (ints)
    const string id = "id";           // int
};

class room final
{
    string sprite;
    string hover_sprite;
    rectangle hitbox;
    point_2d render_at;
    drawing_options draw_opts;
    bool hover;
    point_2d line_start;
    point_2d line_end;
    bool moved_this_frame;

public:
    static room_schema schema;
    static const int X_GAP = 60;
    static const int Y_GAP = 60;
    static const int ROOM_WIDTH = 50;
    static const int ROOM_HEIGHT = 50;

    static room *load(game *game_obj, json save, std::vector<json> &saves);

    int id;
    bool active;
    bool current;
    room_type::types type;
    dynamic_array<room *> next;

    room() = default;
    room(room_type::types type, drawing_options draw_opts);
    room(room_type::types type, drawing_options draw_opts, point_2d pos);

    void update(game *game_obj);
    void render(game *game_obj);
    void set_pos(point_2d pos);
    void move(int x, int y);
    json save();
};

struct game_schema
{
    const string palette = "palette";           // palette name
    const string complete = "complete";         // bool, gameplay-related fields won't be present if true
    const string map = "map";                   // array of arrays of room jsons
    const string ground = "ground";             // array of item_stack jsons
    const string current_room = "current_room"; // int (id)
    const string player = "player";             // player json
};

/**
 * The singleton that holds all the important information of the game. Accessible through the static `instance` function.
 */
class game final
{
    static const struct game_schema schema;
    static const int GROUND_SIZE = 20;
    static const int MAP_LENGTH = 10;
    static const int MAX_MAP_HEIGHT = 6;

    bool full_screen_active;
    screen *full_screen;
    screen *upper_screen;
    screen *lower_screen;
    screen *overlay;
    dynamic_array<backstack_entry> backstack;
    point_2d ground_anchor;
    point_2d ground_gap;
    drawing_options ground_opts;
    tooltip *current_tooltip;
    bool tooltip_active;
    int layer;

    game();

    /**
     * Update cached mouse-related values. Currently the current and previous state of the left button, and the mouse position.
     */
    void update_mouse_vars();

    void populate_ground();

    /**
     * Generate the map. This map will be used for the rest of the game.
     */
    void generate_map();

public:
    player *player_object;
    dynamic_array<enemy *> enemies;
    dynamic_array<npc *> npcs;
    palette colours;
    bool cur_mouse_left;
    bool prev_mouse_left;
    bool cur_mouse_right;
    bool prev_mouse_right;
    point_2d mouse_pos;
    bool in_battle;
    fixed_array<item_stack *, game::GROUND_SIZE> ground;
    /// @brief -1 for player, any other int for position in `enemies`
    int turn;
    dynamic_array<room *> map;
    point_2d prev_mouse_pos;
    bool complete;
    room *current_room;

    /**
     * An escape hatch. Make it work, make it right, make it fast, in that order.
     *
     * @returns A pointer to the game singleton.
     */
    static game *instance();

    /**
     * Make all changes to the state of the game and its screens.
     */
    void update();
    /**
     * Display the current state of the game's current screen(s) to the current window.
     */
    void render();

    /**
     * Swap the current screen of the game. Which screen variable to switch and setting the `full_screen_active` flag are handled automatically based on the screen's `screen_type`. A manual override will be implemented if needed.
     *
     * @param to Pointer to the screen to switch to, usually obtained from the scren class's static `instance` function.
     */
    void change_screen(screen *to);

    /**
     * Swap the current screen of the game to a combination of an overlay, upper, and lower screen. Automatically sets the `full_screen_active` flag.
     *
     * @param overlay Pointer to the overlay screen to use.
     * @param upper Pointer to the upper screen to use.
     * @param lower Pointer to the lower screen to use.
     */
    void change_screen(screen *overlay, screen *upper, screen *lower);

    /**
     * Pop the first entry of the backstack and restore the menus specified therein. Does nothing if the backstack is empty (although this case should never happen in practice).
     */
    void back();

    void spawn_enemies();

    void remove_entity(entity *target);

    void end_room();

    void progress_turn();

    void next_attack();

    void start_room(room *next);

    /**
     * Drop an item onto the ground. Also clears the original stack if successful. Burns the stack if unsuccessful.
     */
    void drop(item_stack *stack);
    void drop(item *thing, int count);

    void add_ticker(ticker ticker_obj, screen_type target);

    void set_tooltip(tooltip *to);
    void render_tooltip();

    /**
     * Write all important information to JSON and store it.
     */
    void save();

    void load();

    /**
     * Delete all info regarding the state of the game (players, enemies, etc.). Until `reset` is called (or everything is manually set) the game is in an UNSAFE state.
     */
    void clear();

    /**
     * Populate game members back to a clean state.
     */
    void reset();

    ~game();
};