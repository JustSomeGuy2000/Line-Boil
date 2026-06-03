#pragma once
#include "splashkit.h"
#include "splashkit-arrays.h"
#include "game.h"
#include "entities.h"
#include "ui.h"
#include <map>

class game;
class player;
class entity;
class tooltip;

/**
 * Indicates the function and available slots of an item. Items may only have one of these.
 */
enum class item_type
{
    /// @brief Can be equipped into the melee slot.
    MELEE,
    /// @brief Can be equipped into the staff slot.
    STAFF,
    /// @brief Cannot be equipped and serves no purpose save existing.
    COLLECTIBLE,
    /// @brief Cannot be equipped but can be consumed.
    CONSUMABLE,
    /// @brief Can be equipped into the accessory slots.
    ACCESSORY,
};

/**
 * Indicates the way the item can affect the world.
 */
enum class attribute
{
    /// @brief Melee damage (flat int).
    MELEE_DMG,
    /// @brief Ranged damage (flat int).
    RANGED_DMG,
    /// @brief Costs mana to use (flat int).
    MANA_COST,
    /// @brief Alters max HP (flat int).
    MAX_HP,
    /// @brief Alters max mana (flat int).
    MAX_MANA,
    /// @brief Alters max boil (flat int).
    MAX_BOIL,
    /// @brief Provides damage reduction against melee attacks (int/100).
    MELEE_DEF,
    /// @brief Provides damage reduction against ranged attacks (int/100).
    RANGED_DEF,
    /// @brief Increases melee damage dealt (int/100).
    MELEE_MOD,
    /// @brief Increases ranged damage dealt (int/100).
    RANGED_MOD,
    /// @brief Increases the target's boil (only applicable for enemies) (flat int).
    BOIL_INCREASE,
    /// @brief On death, decreases the killer's boil (flat int).
    BOIL_REDUCTION,
    /// @brief Provides resistance against incoming boil (int/100).
    BOIL_DEFENSE,
    /// @brief Alters mana regen (flat int).
    MANA_REGEN,
    /// @brief Too lazy to iterate through this enum manually, this provides stop condition for loops.
    DUMMY_LAST,
};
typedef std::map<attribute, int> attrmap;
attrmap default_attrmap();
attrmap make_attrmap(attrmap to_start = default_attrmap());

/**
 * The result of an item stack update. Informs its owner on what to do. The item itself can have no connection to the owner since players and enemies are separate.
 */
enum class item_update_result
{
    /// @brief Nothing happened.
    NONE,
    /// @brief Left-clicked.
    LCLICK,
    /// @brief Right-clicked.
    RCLICK,
    /// @brief Shift-right-clicked.
    SRCLICK,
};

/**
 * The base class for all items. Each item has one instance that controls its behaviour.
 */
class item
{

public:
    static std::map<int, item *> registry;

    string name;
    int max;
    string sprite;
    item_type type;
    string desc;
    std::map<attribute, int> attributes;
    std::map<attribute, int> modifiers;
    int id;

    item(string name, int max, item_type type, string sprite, string desc, attrmap attributes = default_attrmap(), attrmap modifiers = default_attrmap());
    /**
     * Instantiate all items, chiefly to register them. Very important to call before loading saves, otherwise `item::retrieve` will likely fail. Also very important to add every item in here manually.
     */
    static void init();
    static item *instance();
    static int counter();
    static void registrate(item *instance, ...);
    /**
     * Retrieve an item from the registry according to its id. Returns the base Orb if the id is not found.
     *
     * @param id Id of item to look for.
     * @returns Pointer to item instance, or pointer to Orb if not found.
     */
    static item *retrieve(int id);

    virtual bool attack(game *game_obj, entity *user, entity *target, int damage);
    virtual void consume(entity *owner);

    virtual ~item();
};

struct item_stack_schema
{
    const string contains = "contains"; // id (int), -1 if null
    const string count = "count";       // int
    const string scale_x = "scale_x";   // double
    const string scale_y = "scale_y";   // double
    const string x = "x";               // int
    const string y = "y";               // int
};

/**
 * Container class for a stack of items, that is, an type of item and how many there are. Contains additional information and helper functions extrinsic to the item's function.
 */
class item_stack final
{
    rectangle hitbox;
    tooltip *hover;
    tooltip *expanded;
    /// @brief If this is nullptr there is no item here.
    item *contains;
    int count;

    void regenerate();

public:
    static item_stack_schema schema;

    point_2d loc;
    drawing_options draw_opts;

    item_stack();
    item_stack(drawing_options draw_opts, point_2d loc);
    item_stack(item *contains, int count, drawing_options draw_opts, point_2d loc);

    item_update_result update(game *game_obj);
    void render(game *game_obj);
    item *get_contains();
    int get_count();
    void alter(item *contains, int count);
    /**
     * Increase the count of this stack, as long as the final value is valid (between 0 and `item.max`, inclusive).
     *
     * @returns If the increment succeeded.
     */
    bool increment(int by);
    /**
     * Attempt to combine `to` and this stack.
     *
     * @returns If the combination was completely successful, i.e., there is nothing left in `to`.
     */
    bool add_stack(item_stack *stack);
    /**
     * Move the contents of this into `to`.
     *
     * @param to The target item stack.
     */
    void relocate_item(item_stack *to);
    /**
     * Swap the contents of this and `to`.
     *
     * @param to The item stack to swap with.
     */
    void swap(item_stack *to);
    /**
     * Reset this to a default state.
     */
    void clear();
    /**
     * Clear with visual effects (to be implemented).
     */
    void burn();

    bool is_empty();

    json save();
    void load(json from);

    ~item_stack();
};

extern item *coin;
extern item *dagger;
extern item *basic_staff;
extern item *pike;
extern item *glorngus_fists;
extern item *shovel;
class nightwatch_lantern final : public item
{
    nightwatch_lantern();

public:
    static nightwatch_lantern *instance();
    bool attack(game *game_obj, entity *user, entity *target, int damage) override;
};
extern item *shotgun;
extern item *amalgam_strike;
extern item *glorngus_fists_evolved;
extern item *felknight_greatsword;
class glorngus_claymore final : public item
{
    glorngus_claymore();

public:
    static glorngus_claymore *instance();
    bool attack(game *game_obj, entity *user, entity *target, int damage) override;
};