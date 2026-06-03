#pragma once
#include "splashkit.h"
#include "splashkit-arrays.h"
#include "game.h"
#include "items.h"
#include "entities.h"
#include "screens.h"
#include "ui.h"
#include <utility>
#include <functional>
#include <map>

/**
 * This took an hour.
 *
 * Stands for enemy constructor encapsulator (e-ctor-er). Named it ectorer because it sounds cool and mysterious.
 *
 * @param T The type to add. Must have a constuctor conforming to enemy_ctor and a spawn_info field of type enemy_spawn_info.
 */
#define ectorer(T) [](point_2d loc) { return new T(loc); }

class item;
class item_stack;
class game;
class enemy;
enum class item_update_result;
enum class attribute;
struct ticker;
class tooltip;
typedef std::function<enemy *(point_2d)> enemy_ctor;
typedef std::map<attribute, int> attrmap;
attrmap default_attrmap();

timer create_or_get_timer(string name);

ticker disappearing_text(string text, color start_colour, point_2d pos, int delay = 2000, string font = "font_bold", int font_size = 32);

/**
 * Important slot numbers for the player inventory.
 */
enum class inv_slot : int
{
    MELEE,
    RANGED,
    ACCESSORY,
    STORAGE = 5,
};

enum class status : int
{
    BLINDED = 0,
    GLORNGUS_GREATSWORD_CHARGE = 1,
};

class npc
{
protected:
    string sprite;
    drawing_options draw_opts;

public:
    string name;
    rectangle hitbox;
    point_2d render_at;

    npc(string name, string sprite, drawing_options draw_opts, point_2d loc);
    virtual void update(game *game_obj);
    virtual void render(game *game_obj);
    virtual ~npc();
};

class chest : public npc
{
    tooltip *on_hover;
    bool opened;

public:
    chest();
    void update(game *game_obj) override;
    virtual ~chest() override;
};

class merchant : public npc
{
    static const int WARE_COUNT = 8;

    point_2d menu_render_at;
    drawing_options menu_draw_opts;
    dynamic_array<std::pair<item_stack *, item_stack *>> wares;
    tooltip *hover;

public:
    merchant();
    virtual void update(game *game_obj) override;
    virtual void render(game *game_obj) override;
    ~merchant();
};

/**
 * The base class for everything that can be encountered in the dungeon. Contains properties general to all of them.
 */
class entity
{
protected:
    string sprite;
    drawing_options draw_opts;

public:
    string name;
    int max_hp;
    int current_hp;
    rectangle hitbox;
    point_2d render_at;
    attrmap attributes;
    attrmap original_attributes;
    std::map<status, int> statuses;

    entity(string name, string sprite, drawing_options draw_opts, point_2d loc, attrmap starting_attributes = default_attrmap());
    /**
     * Display the entity on the screen.
     *
     * @param game_obj Pointer providing access to the game.
     */
    virtual void render(game *game_obj);
    /**
     * Take some damage.
     *
     * @param game_obj Access to the game.
     * @param origin The entity that dealt the damage.
     * @param damage How much damage was taken.
     * @returns Whether this entity died as a result, and the amount of damage taken after modifiers.
     */
    virtual std::pair<bool, int> take_damage(game *game_obj, entity *origin, int damage, attribute res_type);
    virtual void on_death(game *game_obj, entity *origin) = 0;
    void add_attributes(attrmap &to_add);
    void remove_attributes(attrmap &to_remove);
    virtual void sync_attributes();
    virtual void end_of_turn(game *game_obj);
    virtual int calc_outgoing_dmg(item *weapon);
    virtual ~entity();
};

struct player_schema
{
    const string current_hp = "current_hp";     // int
    const string current_mana = "current_mana"; // int
    const string current_boil = "current_boil"; // int
    const string inv = "inv";                   // list of item_stack jsons
};

/**
 * The class representing the player. Although similar to an entity, it is different enough that I made it completely separate.
 */
class player final : public entity
{
    rectangle hp_bar;
    rectangle mana_bar;
    rectangle boil_bar;

    // Arrangement variables for the inventory

    point_2d melee_slot;
    point_2d ranged_slot;
    point_2d acc_slot_anchor;
    int acc_slot_gap;
    point_2d inv_anchor;
    int inv_x_gap;
    int inv_y_gap;
    drawing_options large_slot_opts;
    drawing_options small_slot_opts;

    void handle_item_update(item_update_result result, int pos, game *game_obj);

public:
    static player_schema schema;

    int max_mana;
    int current_mana;
    int max_boil;
    int current_boil;
    int mana_regen;
    fixed_array<item_stack *, 20> inv;

    player();

    void update(game *game_obj);
    void render(game *game_obj) override;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
    void attack(game *game_obj, enemy *target, inv_slot slot);
#pragma clang diagnostic pop
    std::pair<bool, int> take_damage(game *game_obj, entity *origin, int damage, attribute res_type) override;
    void on_death(game *game_obj, entity *origin) override;
    /**
     * Attempt to move an item stack into this player's storage.
     *
     * @returns Whether the operation was successful.
     */
    bool move_to_storage(item_stack *stack);
    /**
     * Attempt to move an item stack into an open accessory slot.
     *
     * @returns Whether the operation was successful.
     */
    bool move_to_accessories(item_stack *stack);
    void sync_attributes() override;
    void start_attack(game *game_obj, inv_slot slot);
    json save();
    void load(json from);

    ~player();
};

class enemy : public entity
{
protected:
    item_stack *weapon;
    int info_size;
    string info_font;
    point_2d name_coords;
    point_2d hp_coords;
    int coin_min;
    int coin_max;

public:
    static const int BUCKET_WIDTH = 5;
    static const int BUCKET_COUNT = 9;
    static constexpr double STDEV = 5;
    // Associates each enemy to its value.
    static std::vector<std::pair<enemy_ctor, int>> registry;
    // Assigns a bucket upper bound to a list of enemies.
    static std::map<int, std::vector<enemy_ctor>> buckets;
    static std::vector<enemy_ctor> bosses;

    static void init();
    static void registrate(std::vector<enemy_ctor> registrees);

    int value;

    enemy(string name, string sprite, drawing_options draw_opts, point_2d loc, int max_hp, int coin_min, int coin_max, int value, attrmap attributes = default_attrmap());

    /**
     * Display the enemy on the screen.
     *
     * @param game_obj Pointer providing access to the game.
     */
    void render(game *game_obj) override;
    std::pair<bool, int> take_damage(game *game_obj, entity *origin, int damage, attribute res_type) override;
    void on_death(game *game_obj, entity *origin) override;
    virtual void attack(game *game_obj, entity *target);
    virtual void start_attack(game *game_obj);
    void sync_attributes() override;
    virtual void drop(game *game_obj);

    ~enemy();
};

class dagger_man : public enemy
{
public:
    dagger_man(point_2d loc);
    void drop(game *game_obj) override;
};

class recruit : public enemy
{
public:
    recruit(point_2d loc);
    void drop(game *game_obj) override;
};

class glorngus : public enemy
{
public:
    glorngus(point_2d loc);
};

class gravedigger : public enemy
{
public:
    gravedigger(point_2d loc);
    void attack(game *game_obj, entity *target) override;
    void drop(game *game_obj) override;
};

class nightwatch : public enemy
{

public:
    nightwatch(point_2d loc);
    void attack(game *game_obj, entity *target) override;
    void drop(game *game_obj) override;
};

class amalgam : public enemy
{
public:
    amalgam(point_2d loc);
};

class glorngus_evolved : public enemy
{
public:
    glorngus_evolved(point_2d loc);
};

class felknight : public enemy
{
public:
    felknight(point_2d loc);
    void drop(game *game_obj) override;
};

class glorngus_ex : public enemy
{
public:
    glorngus_ex(point_2d loc);
    void drop(game *game_obj) override;
};