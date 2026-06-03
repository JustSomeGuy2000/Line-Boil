#pragma once
#include "game.h"
#include "splashkit.h"
#include "splashkit-arrays.h"
#include <functional>
#include <utility>

#define print(val) write_line(to_string(val))

class game;

point_2d scale_render_pos(string bitmap, point_2d loc, const drawing_options &draw_opts);
rectangle scale_rectangle(string bitmap, point_2d loc, const drawing_options &draw_opts);

/**
 * A personal implementation of the button, since SplashKit buttons don't allow changing their appearance. Only use through pointers, otherwise incomplete type errors are likely to appear. Make sure to delete after!
 */
class default_btn final
{
    bool held;
    rectangle hitbox;
    string normal_bitmap;
    string held_bitmap;
    std::function<void(void)> on_click;
    string text;
    string font;
    int font_size;
    point_2d text_coords;
    drawing_options draw_opts;
    point_2d render_at;

public:
    default_btn() = default;
    default_btn(rectangle hitbox, std::function<void(void)> on_click, string text, point_2d text_coords, int font_size = 48, string font = "font_bold", string normal_bitmap = "button", string held_bitmap = "button_pressed", drawing_options draw_opts = option_defaults());
    default_btn(point_2d pos, std::function<void(void)> on_click, string text, int font_size = 48, string font = "font_bold", string normal_bitmap = "button", string held_bitmap = "button_pressed", drawing_options draw_opts = option_defaults());

    /**
     * Change the button's state according to the mouse state. Also call `on_click` if the button was pressed.
     *
     * @param game_obj A pointer giving access to the game.
     */
    void update(game *game_obj);
    /**
     * Display the button according to its current state,
     * @param game_obj A pointer giving access to the game.
     */
    void render(game *game_obj);
};

/**
 * A simpler button that only displays a single image and detects clicks. Like `custom_btn`, use through pointers.
 */
class symbol_btn final
{
    bool held;
    rectangle hitbox;
    string bitmap_name;
    void (*on_click)();
    drawing_options draw_opts;

public:
    symbol_btn() = default;
    symbol_btn(rectangle hitbox, string bitmap_name, void (*on_click)(), drawing_options draw_opts = option_defaults());

    /**
     * Change the button's state according to the mouse state. Also call `on_click` if the button was pressed.
     *
     * @param game_obj A pointer giving access to the game.
     */
    void update(game *game_obj);
    /**
     * Display the button according to its current state,
     * @param game_obj A pointer giving access to the game.
     */
    void render(game *game_obj);
};

class beside_btn final
{
    std::function<void(void)> on_click;
    string text;
    string font_name;
    int font_size;
    string symbol;
    drawing_options draw_opts;
    point_2d render_at;
    bool hover;

public:
    // Public so lists of buttons can do alignment.
    rectangle hitbox;
    beside_btn() = default;
    beside_btn(std::function<void(void)> on_click, string text, point_2d pos, string symbol, drawing_options draw_opts, string font_name = "font_reg", int font_size = 24);

    /**
     * Change the button's state according to the mouse state. Also call `on_click` if the button was pressed.
     *
     * @param game_obj A pointer giving access to the game.
     */
    void update(game *game_obj);
    /**
     * Display the button according to its current state,
     * @param game_obj A pointer giving access to the game.
     */
    void render(game *game_obj);
};

enum class tooltip_part_type
{
    CONST_STR,
    FUNC_STR,
    NEWLINE,
    BLANK_LINE,
};
typedef tooltip_part_type tpt;

struct tooltip_part
{
    tpt type;
    std::function<color(void)> colour;
    int size;
    string font;
    string const_str = "";
    std::function<string(void)> func_str = nullptr;
    string final_text = "";
    int x = 0;
    int y = 0;
};

class tooltip final
{
    static constexpr string BLANK_REF = "I";
    bitmap bmp;
    dynamic_array<tooltip_part> parts;

public:
    tooltip();

    void render(game *game_obj);

    void regenerate();
    tooltip *generate();

    tooltip *add(string str, int size, std::function<color(void)> colour, string font = "font_reg");
    tooltip *add(std::function<string(void)> func, int size, std::function<color(void)> colour, string font = "font_reg");
    tooltip *newline();
    tooltip *blank_line(int size = 12, string font = "font_reg");

    ~tooltip();
};