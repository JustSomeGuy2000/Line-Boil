#include "ui.h"
#include "game.h"
#include "splashkit.h"
#include <stdexcept>
#include <functional>

point_2d scale_render_pos(string bitmap, point_2d loc, const drawing_options &draw_opts)
{
    rectangle scaled = scale_rectangle(bitmap, loc, draw_opts);
    rectangle original = bitmap_bounding_rectangle(bitmap_named(bitmap), loc.x, loc.y);
    return point_at(loc.x - (original.width - scaled.width) / 2, loc.y - (original.height - scaled.height) / 2);
}

rectangle scale_rectangle(string bitmap, point_2d loc, const drawing_options &draw_opts)
{
    rectangle rect = bitmap_bounding_rectangle(bitmap_named(bitmap), loc.x, loc.y);
    return rectangle_from(loc.x, loc.y, rect.width * draw_opts.scale_x, rect.height * draw_opts.scale_y);
}

#pragma region default_btn
default_btn::default_btn(rectangle hitbox, std::function<void(void)> on_click, string text, point_2d text_coords, int font_size, string font, string normal_bitmap, string held_bitmap, drawing_options draw_opts)
{
    this->held = false;
    this->hitbox = hitbox;
    this->on_click = on_click;
    this->normal_bitmap = normal_bitmap;
    this->held_bitmap = held_bitmap;
    this->text = text;
    this->font = font;
    this->font_size = font_size;
    this->text_coords = text_coords;
    this->draw_opts = draw_opts;
    this->render_at = point_at(this->hitbox.x, this->hitbox.y);
}

default_btn::default_btn(point_2d pos, std::function<void(void)> on_click, string text, int font_size, string font, string normal_bitmap, string held_bitmap, drawing_options draw_opts)
{
    this->held = false;
    this->hitbox = scale_rectangle(normal_bitmap, pos, draw_opts);
    this->on_click = on_click;
    this->normal_bitmap = normal_bitmap;
    this->held_bitmap = held_bitmap;
    this->text = text;
    this->font = font;
    this->font_size = font_size;
    this->text_coords = point_at(this->hitbox.x + this->hitbox.width / 2 - text_width(this->text, this->font, this->font_size) / 2, this->hitbox.y + this->hitbox.height / 2 - text_height(this->text, this->font, this->font_size) / 2);
    this->draw_opts = draw_opts;
    this->render_at = scale_render_pos(this->normal_bitmap, pos, this->draw_opts);
}

void default_btn::update(game *game_obj)
{
    if (game_obj->cur_mouse_left && !game_obj->prev_mouse_left && point_in_rectangle(game_obj->mouse_pos, this->hitbox))
    {
        this->held = true;
    }
    else if (this->held && mouse_up(mouse_button::LEFT_BUTTON))
    {
        this->held = false;
        if (point_in_rectangle(game_obj->mouse_pos, this->hitbox))
        {
            this->on_click();
        }
    }
}

void default_btn::render(game *game_obj)
{
    string button_mode = this->held ? this->held_bitmap : this->normal_bitmap;
    draw_bitmap(button_mode, this->render_at.x, this->render_at.y, this->draw_opts);
    draw_text(this->text, game_obj->colours.text, this->font, this->font_size, this->text_coords.x, this->text_coords.y);
}
#pragma endregion

#pragma region symbol_btn
symbol_btn::symbol_btn(rectangle hitbox, string bitmap_name, void (*on_click)(), drawing_options draw_opts)
{
    this->hitbox = hitbox;
    this->bitmap_name = bitmap_name;
    this->on_click = on_click;
    this->draw_opts = draw_opts;
}

void symbol_btn::update(game *game_obj)
{
    if (game_obj->cur_mouse_left && !game_obj->prev_mouse_left && point_in_rectangle(game_obj->mouse_pos, this->hitbox))
    {
        this->held = true;
    }
    else if (this->held && mouse_up(mouse_button::LEFT_BUTTON))
    {
        this->held = false;
        if (point_in_rectangle(game_obj->mouse_pos, this->hitbox))
        {
            this->on_click();
        }
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
void symbol_btn::render(game *game_obj)
#pragma clang diagnostic pop
{
    draw_bitmap(this->bitmap_name, this->hitbox.x, this->hitbox.y, this->draw_opts);
}
#pragma endregion

#pragma region beside_btn
beside_btn::beside_btn(std::function<void(void)> on_click, string text, point_2d pos, string symbol, drawing_options draw_opts, string font_name, int font_size)
{
    this->on_click = on_click;
    this->text = text;
    this->font_name = font_name;
    this->font_size = font_size;
    this->symbol = symbol;
    this->draw_opts = draw_opts;
    this->hitbox = rectangle_from(pos.x, pos.y, text_width(this->text, this->font_name, this->font_size), text_height(this->text, this->font_name, this->font_size));
    rectangle scaled = scale_rectangle(this->symbol, pos, this->draw_opts);
    this->render_at = scale_render_pos(this->symbol, point_at(pos.x - scaled.width, pos.y + this->hitbox.height / 2 - scaled.height / 2), this->draw_opts);
    this->hover = false;
}

void beside_btn::update(game *game_obj)
{
    if (point_in_rectangle(game_obj->mouse_pos, this->hitbox))
    {
        this->hover = true;
        if (game_obj->prev_mouse_left && !game_obj->cur_mouse_left)
        {
            this->on_click();
        }
    }
    else
    {
        this->hover = false;
    }
}

void beside_btn::render(game *game_obj)
{
    draw_text(this->text, game_obj->colours.text, this->font_name, this->font_size, this->hitbox.x, this->hitbox.y);
    if (this->hover)
    {
        draw_bitmap(this->symbol, this->render_at.x, this->render_at.y, this->draw_opts);
    }
}
#pragma endregion

#pragma region tooltip
tooltip::tooltip()
{
    this->bmp = create_bitmap(to_string(uid()), 1, 1);
    this->parts = dynamic_array<tooltip_part>();
}

void tooltip::render(game *game_obj)
{
    point_2d start = game_obj->mouse_pos;
    start.y -= bitmap_height(this->bmp);
    if (start.x + bitmap_width(this->bmp) > WIN_WIDTH)
    {
        start.x -= bitmap_width(this->bmp);
    }
    if (start.y < 0)
    {
        start.y += bitmap_height(this->bmp);
    }

    draw_bitmap(this->bmp, start.x, start.y);
}

void tooltip::regenerate()
{
    game *game_obj = game::instance();
    const int BORDER = 3;
    int current_x = BORDER;
    int current_y = BORDER;
    int temp_width = 0;
    int temp_height = 0;
    int line_max_height = 0;
    int max_width = 0;
    for (int i = 0; i < this->parts.length(); i++)
    {
        tooltip_part &part = this->parts[i];
        if (part.type == tpt::CONST_STR || part.type == tpt::FUNC_STR)
        {
            if (part.type == tpt::CONST_STR)
            {
                part.final_text = part.const_str;
            }
            else
            {
                part.final_text = part.func_str();
            }
            temp_width = text_width(part.final_text, part.font, part.size);
            temp_height = text_height(part.final_text, part.font, part.size);
            part.x = current_x;
            part.y = current_y;
            current_x += temp_width;
            if (current_x > max_width)
            {
                max_width = current_x;
            }
            if (temp_height > line_max_height)
            {
                line_max_height = temp_height;
            }
        }
        else
        {
            current_y += ((part.type == tpt::NEWLINE) ? line_max_height : text_height(tooltip::BLANK_REF, part.font, part.size));
            line_max_height = 0;
            current_x = BORDER;
        }
    }

    free_bitmap(this->bmp);
    this->bmp = create_bitmap(to_string(uid()), max_width + BORDER, current_y + BORDER);
    clear_bitmap(this->bmp, game_obj->colours.bg);
    draw_rectangle_on_bitmap(this->bmp, game_obj->colours.line_colour, 0, 0, max_width + BORDER, current_y + BORDER, option_line_width(BORDER));

    for (int i = 0; i < this->parts.length(); i++)
    {
        tooltip_part &part = this->parts[i];
        draw_text_on_bitmap(this->bmp, part.final_text, part.colour(), part.font, part.size, part.x, part.y);
    }
}

tooltip *tooltip::generate()
{
    this->regenerate();
    return this;
}

tooltip *tooltip::add(string str, int size, std::function<color(void)> colour, string font)
{
    this->parts.add({tpt::CONST_STR, colour, size, font, str});
    return this;
}

tooltip *tooltip::add(std::function<string(void)> func, int size, std::function<color(void)> colour, string font)
{
    this->parts.add({tpt::FUNC_STR, colour, size, font, "", func});
    return this;
}

tooltip *tooltip::newline()
{
    this->parts.add({tpt::NEWLINE, []()
                     { return COLOR_WHITE; }, 0, "font_reg"});
    return this;
}

tooltip *tooltip::blank_line(int size, string font)
{
    this->parts.add({tpt::BLANK_LINE, []()
                     { return COLOR_WHITE; }, size, font});
    return this;
}

tooltip::~tooltip()
{
    free_bitmap(this->bmp);
}
#pragma endregion