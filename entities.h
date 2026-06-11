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

/**
 * Exactly what it says on the tin. Create the timer if it doesn't exit, return the already existing instance if it does.
 *
 * @returns The timer.
 */
timer create_or_get_timer(string name);

/**
 * @returns A ticker contatining a lambda for text that fades over time, specified by the parameters.
 */
ticker disappearing_text(string text, color start_colour, point_2d pos, int delay = 2000, string font = "font_bold", int font_size = 32);

/**
 * Important slot indexes for the player inventory.
 */
enum class inv_slot : int
{
    MELEE,
    RANGED,
    ACCESSORY,
    STORAGE = 5,
};

/**
 * Status effects entities can be affected with.
 */
enum class status : int
{
    /// @brief Outgoing melee and ranged damage -20%
    BLINDED = 0,
    /// @brief Incoming melee and ranged damage +20% (not implemented).
    GLORNGUS_GREATSWORD_CHARGE = 1,
};

/**
 * Base class for npc's, differentiated from enemies by their inability to attack. There is some duplicated code bewteen the two, but when I tried to combine them things started breaking and there wasn't enough time to figure out why.
 */
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
    /**
     * Make all changes to the entity's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    virtual void update(game *game_obj);
    /**
     * Display the entity on the screen.
     *
     * @param game_obj Pointer providing access to the game.
     */
    virtual void render(game *game_obj);
    /**
     * @returns A JSON object that `load` can recreate this from.
     */
    virtual json save() = 0;
    /**
     * Recreate this from a JSON object of the proper schema.
     *
     * @param from The object to use.
     */
    virtual void load(json from) = 0;
    virtual ~npc();
};

class chest : public npc
{
    tooltip *on_hover;
    bool opened;

public:
    chest();
    /**
     * Make all changes to the entity's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    void update(game *game_obj) override;
    json save() override;
    void load(json from) override;
    virtual ~chest() override;
};

struct merchant_schema
{
    const string items = "items"; // array of item_stack JSONs
    const string costs = "costs"; // array of item_stack JSONs
};

class merchant : public npc
{
    static struct merchant_schema schema;
    static const int WARE_COUNT = 8;

    point_2d menu_render_at;
    drawing_options menu_draw_opts;
    dynamic_array<std::pair<item_stack *, item_stack *>> wares;
    tooltip *hover;

public:
    merchant();
    /**
     * Make all changes to the entity's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    virtual void update(game *game_obj) override;
    /**
     * Display the entity on the screen.
     *
     * @param game_obj Pointer providing access to the game.
     */
    virtual void render(game *game_obj) override;
    json save() override;
    void load(json from) override;
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
     * Things to do when this takes damage.
     *
     * @param game_obj Access to the game object.
     * @param origin The entity that damaged this.
     * @param damage How much damage is coming in.
     * @param res_type Which attribute should be used to calculate resistance.
     * @returns Whether this died as a result, and how much damage was taken in the end.
     */
    virtual std::pair<bool, int> take_damage(game *game_obj, entity *origin, int damage, attribute res_type);
    /**
     * Things to do when this entity dies.
     *
     * @param game_obj Access to the game object.
     * @param origin What entity this died to.
     */
    virtual void on_death(game *game_obj, entity *origin) = 0;
    /**
     * Combine attributes between this and another attribute map.
     *
     * @param to_add The map to add from.
     */
    void add_attributes(attrmap &to_add);
    /**
     * Subtract attributes from this according to another attribute map.
     *
     * @param to_remove The map to subtract from.
     */
    void remove_attributes(attrmap &to_remove);
    /**
     * Reset this entity's attributes and add all applicable maps, e.g. from weapons and accessories.
     */
    virtual void sync_attributes();
    /**
     * Things to do at the end of the turn.
     *
     * @param game_obj Access to the game object.
     */
    virtual void end_of_turn(game *game_obj);
    /**
     * Calculate how much damage this should deal according to its attributes and what it's attacking with.
     *
     * @param weapon The item to attack with.
     * @returns Calculation result.
     */
    virtual int calc_outgoing_dmg(item *weapon);
    virtual ~entity();
};

/**
 * JSON keys for the player class.
 */
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

    /**
     * Take action according to how an item stack in the inventory was clicked.
     *
     * @param result The update result, i.e. whether and how it was clicked.
     * @param pos Index of the item stack in the inventory.
     * @param game_obj Access to the game.
     */
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

    /**
     * Make all changes to the entity's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    void update(game *game_obj);
    /**
     * Display the entity on the screen.
     *
     * @param game_obj Pointer providing access to the game.
     */
    void render(game *game_obj) override;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
    void attack(game *game_obj, enemy *target, inv_slot slot);
#pragma clang diagnostic pop
    /**
     * Things to do when this takes damage.
     *
     * @param game_obj Access to the game object.
     * @param origin The entity that damaged this.
     * @param damage How much damage is coming in.
     * @param res_type Which attribute should be used to calculate resistance.
     * @returns Whether this died as a result, and how much damage was taken in the end.
     */
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
    /**
     * Things to do at the end of the turn.
     *
     * @param game_obj Access to the game object.
     */
    void sync_attributes() override;
    /**
     * Begin the attack phase. For the player, this means changing the lower screen to the target choosing screen.
     *
     * @param game_obj Access to the game object.
     * @param slot Index of the inventory slot whose item the player is atatcking with.
     */
    void start_attack(game *game_obj, inv_slot slot);
    /**
     * @returns This player converted into a JSON object that can be used in `load`.
     */
    json save();
    /**
     * Set this player's state according to a JSON object that conforms to the player schema.
     *
     * @param from The JSON object to load from.
     */
    void load(json from);

    ~player();
};

class enemy : public entity
{
    static const int type_id = 0;

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
    // Associates each enemy type to its id.
    static std::map<int, enemy_ctor> registry;
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
    /**
     * Things to do when this takes damage.
     *
     * @param game_obj Access to the game object.
     * @param origin The entity that damaged this.
     * @param damage How much damage is coming in.
     * @param res_type Which attribute should be used to calculate resistance.
     * @returns Whether this died as a result, and how much damage was taken in the end.
     */
    std::pair<bool, int> take_damage(game *game_obj, entity *origin, int damage, attribute res_type) override;
    /**
     * Things to do when this entity dies.
     *
     * @param game_obj Access to the game object.
     * @param origin What entity this died to.
     */
    void on_death(game *game_obj, entity *origin) override;
    /**
     * Attack with the weapon this currently wields.
     *
     * @param game_obj Access to the game object.
     * @param target The attack target, usually supplied by `start_attack`.
     */
    virtual void attack(game *game_obj, entity *target);
    /**
     * Begin the attack phase. Originally created for parity with the player, this just calls `attack`.
     *
     * @param game_obj Access to the game object.
     */
    virtual void start_attack(game *game_obj);
    /**
     * Reset this entity's attributes and add all applicable maps, e.g. from weapons and accessories.
     */
    void sync_attributes() override;
    /**
     * Drop any loot this drops on death.
     *
     * @param game_obj Access to the game object.
     */
    virtual void drop(game *game_obj);
    /**
     * Get the type_id of this enemy. Even though it is public, the game cannot see it since:
     *
     * 1) type_id has to be static, since all instances of the enemy have the same one.
     *
     * 2) The game always works with enemy*, not its subclasses.
     *
     * 3) Because of C++'s moronic static resolution rules and refusal to consider types objects, accessing type_id through that will return enemy::type_id.
     *
     * 4) Thus, it must be given by the enemy itself through overriden functions, which will dispatch to the proepr version at runtime.
     *
     * And yes, this must be duplicated for every enemy class otherwise static resolution will bite you in the arse, and there isn't any `static virtual`.
     *
     * @returns The type_id of the type of the enemy.
     */
    virtual int get_typeid();

    ~enemy();
};

class dagger_man : public enemy
{
    static const int type_id = 1;

public:
    dagger_man(point_2d loc);
    /**
     * Drop any loot this drops on death.
     *
     * @param game_obj Access to the game object.
     */
    void drop(game *game_obj) override;
    int get_typeid() override;
};

class recruit : public enemy
{
    static const int type_id = 2;

public:
    recruit(point_2d loc);
    /**
     * Drop any loot this drops on death.
     *
     * @param game_obj Access to the game object.
     */
    void drop(game *game_obj) override;
    int get_typeid() override;
};

class glorngus : public enemy
{
    static const int type_id = 3;

public:
    glorngus(point_2d loc);
    int get_typeid() override;
};

class gravedigger : public enemy
{
    static const int type_id = 4;

public:
    gravedigger(point_2d loc);
    /**
     * Attack with the weapon this currently wields. Also spawns a dirt patch to put on the player's screen.
     *
     * @param game_obj Access to the game object.
     * @param target The attack target, usually supplied by `start_attack`.
     */
    void attack(game *game_obj, entity *target) override;
    /**
     * Drop any loot this drops on death.
     *
     * @param game_obj Access to the game object.
     */
    void drop(game *game_obj) override;
    int get_typeid() override;
};

class nightwatch : public enemy
{
    static const int type_id = 5;

public:
    nightwatch(point_2d loc);
    /**
     * Attack with the weapon this currently wields. Switches between the shotgun and lantern every turn.
     *
     * @param game_obj Access to the game object.
     * @param target The attack target, usually supplied by `start_attack`.
     */
    void attack(game *game_obj, entity *target) override;
    /**
     * Drop any loot this drops on death.
     *
     * @param game_obj Access to the game object.
     */
    void drop(game *game_obj) override;
    int get_typeid() override;
};

class amalgam : public enemy
{
    static const int type_id = 6;

public:
    amalgam(point_2d loc);
    int get_typeid() override;
};

class glorngus_evolved : public enemy
{
    static const int type_id = 7;

public:
    glorngus_evolved(point_2d loc);
    int get_typeid() override;
};

class felknight : public enemy
{
    static const int type_id = 8;

public:
    felknight(point_2d loc);
    /**
     * Drop any loot this drops on death.
     *
     * @param game_obj Access to the game object.
     */
    void drop(game *game_obj) override;
    int get_typeid() override;
};

class glorngus_ex : public enemy
{
    static const int type_id = 9;

public:
    glorngus_ex(point_2d loc);
    /**
     * Drop any loot this drops on death.
     *
     * @param game_obj Access to the game object.
     */
    void drop(game *game_obj) override;
    int get_typeid() override;
};