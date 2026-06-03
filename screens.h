#pragma once
#include "game.h"
#include "items.h"
#include "splashkit.h"
#include "splashkit-arrays.h"
#include "ui.h"
#include "entities.h"
#include <functional>

class game;
class default_btn;
class symbol_btn;
class beside_btn;
enum class item_update_result;
enum class inv_slot;

struct ticker
{
    string timer;
    std::function<bool(game *)> func;
};

/**
 * Which parts of the window the screen should occupy. `FULL` overrides everything else, while the other 3 are displayed simultaneously.
 */
enum class screen_type
{
    /// @brief This screen occupies the full window.
    FULL,
    /// @brief This screen occupies the upper part of the window. This is an arbitrary bound.
    UPPER,
    /// @brief This screen occupies the lower part of the window. This is an arbitrary bound.
    LOWER,
    /// @brief This screen is an extra layer displayed on the entire window if a `FULL` screen is not being displayed.
    OVERLAY
};

/**
 * Base class for all screens. Not supposed to ever be shown, and will display an error message if it is.
 */
class screen
{
protected:
    /**
     * @param type What sort of screen it is.
     * @param is_battle_screen Whether the game is in_battle when this screen is active. Only applies to full and upper screens.
     */
    screen(screen_type type, bool is_battle_screen = false);

public:
    static rectangle upper_mask;
    bool is_battle_screen;
    screen_type type;
    dynamic_array<ticker> tickers;

    /**
     * @returns The pointer to the screen singleton.
     */
    static screen *instance();

    /**
     * Make all changes to the screen's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    virtual void update(game *game_object);
    /**
     * Display the screen to the current window.
     *
     * @param game_object A pointer giving access to the game.
     */
    virtual void render(game *game_object);

    void tick(game *game_obj);

    void clear_tickers();

    virtual ~screen();
};

class start_screen : public screen
{
    drawing_options title_opts;
    default_btn *settings_button;
    default_btn *start_button;
    default_btn *cont_button;

    start_screen();

public:
    /**
     * @returns The pointer to the screen singleton.
     */
    static start_screen *instance();

    /**
     * Make all changes to the screen's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    void update(game *game_object) override;
    /**
     * Display the screen to the current window.
     *
     * @param game_object A pointer giving access to the game.
     */
    void render(game *game_object) override;

    ~start_screen() override;
};

class settings_screen : public screen
{
    bool bg_light;
    rectangle bg_toggle_hitbox;
    rectangle back_hitbox;
    symbol_btn *back_button;

    settings_screen();

public:
    /**
     * @returns The pointer to the screen singleton.
     */
    static settings_screen *instance();

    /**
     * Make all changes to the screen's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    void update(game *game_object) override;
    /**
     * Display the screen to the current window.
     *
     * @param game_object A pointer giving access to the game.
     */
    void render(game *game_object) override;

    ~settings_screen() override;
};

class game_overlay : public screen
{
    drawing_options ui_draw_opts;
    symbol_btn *pause_button;

    game_overlay();

    void handle_item_update(item_update_result result, int pos, game *game_obj);

public:
    /**
     * @returns The pointer to the screen singleton.
     */
    static game_overlay *instance();

    /**
     * Make all changes to the screen's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game
     */
    void update(game *game_object) override;
    /**
     * Display the screen to the current window.
     *
     * @param game_object A pointer giving access to the game.
     */
    void render(game *game_object) override;

    ~game_overlay() override;
};

class battle_upper : public screen
{
    default_btn *flee_button;

    battle_upper();

public:
    /**
     * @returns The pointer to the screen singleton.
     */
    static battle_upper *instance();

    /**
     * Make all changes to the screen's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    void update(game *game_object) override;
    /**
     * Display the screen to the current window.
     *
     * @param game_object A pointer giving access to the game.
     */
    void render(game *game_object) override;

    ~battle_upper() override;
};

class battle_lower : public screen
{

    battle_lower();

public:
    /**
     * @returns The pointer to the screen singleton.
     */
    static battle_lower *instance();

    /**
     * Make all changes to the screen's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    void update(game *game_object) override;
    /**
     * Display the screen to the current window.
     *
     * @param game_object A pointer giving access to the game.
     */
    void render(game *game_object) override;

    ~battle_lower() override;
};

class pause_screen : public screen
{
    default_btn *resume_button;
    default_btn *saq_button;

    pause_screen();

public:
    /**
     * @returns The pointer to the screen singleton.
     */
    static pause_screen *instance();

    /**
     * Make all changes to the screen's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    void update(game *game_object) override;
    /**
     * Display the screen to the current window.
     *
     * @param game_object A pointer giving access to the game.
     */
    void render(game *game_object) override;

    ~pause_screen() override;
};

/**
 * The lower screen allowing the player to select an enemy to attack.
 */
class attack_lower : public screen
{
    dynamic_array<beside_btn *> buttons;
    point_2d button_anchor;
    point_2d button_gap;
    drawing_options button_draw_opts;
    beside_btn *back_button;

    attack_lower();
    void regenerate(game *game_obj, inv_slot slot);

public:
    /**
     * @returns The pointer to the screen singleton. The contents update according to the game state every time this is called.
     */
    static attack_lower *instance(game *game_obj, inv_slot slot);

    /**
     * Make all changes to the screen's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    void update(game *game_object) override;
    /**
     * Display the screen to the current window.
     *
     * @param game_object A pointer giving access to the game.
     */
    void render(game *game_object) override;

    ~attack_lower() override;
};

class post_battle_upper : public screen
{
    post_battle_upper();

public:
    /**
     * @returns The pointer to the screen singleton.
     */
    static post_battle_upper *instance();

    /**
     * Make all changes to the screen's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    void update(game *game_object) override;
    /**
     * Display the screen to the current window.
     *
     * @param game_object A pointer giving access to the game.
     */
    void render(game *game_object) override;

    ~post_battle_upper() override;
};

class game_over : public screen
{
    default_btn *to_menu_button;

    game_over();

public:
    static game_over *instance();
    /**
     * Make all changes to the screen's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    void update(game *game_object) override;
    /**
     * Display the screen to the current window.
     *
     * @param game_object A pointer giving access to the game.
     */
    void render(game *game_object) override;

    ~game_over() override;
};

class npc_upper : public screen
{
    default_btn *next_button;
    npc_upper();

public:
    static npc_upper *instance();
    /**
     * Make all changes to the screen's state based on the game's state.
     *
     * @param game_object A pointer giving access to the game.
     */
    void update(game *game_object) override;
    /**
     * Display the screen to the current window.
     *
     * @param game_object A pointer giving access to the game.
     */
    void render(game *game_object) override;

    ~npc_upper() override;
};