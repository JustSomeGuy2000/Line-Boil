#include "screens.h"
#include "splashkit.h"
#include "splashkit-arrays.h"
#include "game.h"
#include "ui.h"

#pragma region screen
rectangle screen::upper_mask = rectangle_from(30, 30, 635, 320);

screen::screen(screen_type type, bool is_battle_screen)
{
    this->type = type;
    this->is_battle_screen = is_battle_screen;
}

screen *screen::instance()
{
    static screen *inst = new screen(screen_type::FULL);
    return inst;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
void screen::update(game *game_object)
#pragma clang diagnostic pop
{
}

void screen::render(game *game_object)
{
    draw_text("You are seeing the base screen. Something went wrong...", game_object->colours.text, 0, 0);
}

void screen::tick(game *game_obj)
{
    static auto temp = dynamic_array<ticker>();
    for (int i = 0; i < this->tickers.length(); i++)
    {
        if (this->tickers[i].func(game_obj))
        {
            temp.add(this->tickers[i]);
        }
        else
        {
            free_timer(timer_named(this->tickers[i].timer));
        }
    }
    this->tickers = temp;
    temp.clear();
}

void screen::clear_tickers()
{
    for (int i = 0; i < this->tickers.length(); i++)
    {
        free_timer(timer_named(this->tickers[i].timer));
    }
    this->tickers.clear();
}

screen::~screen()
{
}
#pragma endregion

#pragma region start screen
start_screen::start_screen() : screen(screen_type::FULL)
{
    this->start_button = new default_btn(point_at(210, 350), []()
                                         {
        game *game_obj = game::instance();
        game_obj->complete = false;
        game_obj->clear();
        game_obj->reset();
        game_obj->start_room(game_obj->current_room); }, "New Game");
    this->cont_button = new default_btn(point_at(210, 470), []()
                                        {
        game *game_obj  =game::instance();
        game_obj->complete = false;
        game_obj->clear();
        game_obj->reset();
        game_obj->load();
        game_obj->start_room(game_obj->current_room, false); }, "Continue");
    this->title_opts = option_scale_bmp(0.7, 0.7);
    this->settings_button = new default_btn(point_at(210, 590), []()
                                            { game::instance()->change_screen(settings_screen::instance()); }, "Settings");
}

start_screen *start_screen::instance()
{
    static start_screen *inst = new start_screen();
    return inst;
}

void start_screen::update(game *game_object)
{
    this->start_button->update(game_object);
    this->cont_button->update(game_object);
    this->settings_button->update(game_object);
}

void start_screen::render(game *game_object)
{
    draw_bitmap("title", 100, 30, this->title_opts);

    this->start_button->render(game_object);
    this->cont_button->render(game_object);
    this->settings_button->render(game_object);

    draw_text("© 2026 Joshua Eng Han Ruong. All rights reserved.", game_object->colours.text, "font_reg", 15, 190, 770);
}

start_screen::~start_screen()
{
    delete this->start_button;
    delete this->settings_button;
    delete this->cont_button;
}
#pragma endregion

#pragma region settings screen
settings_screen::settings_screen() : screen(screen_type::FULL)
{
    this->bg_light = true;
    this->bg_toggle_hitbox = rectangle_from(100, 150, 116, 65);
    this->back_button = new symbol_btn(rectangle_from(0, -15, 71, 86), "back", []()
                                       { game::instance()->back(); }, option_scale_bmp(0.6, 0.6));
}

settings_screen *settings_screen::instance()
{
    static settings_screen *inst = new settings_screen();
    return inst;
}

void settings_screen::update(game *game_object)
{

    this->back_button->update(game_object);

    if (mouse_clicked(mouse_button::LEFT_BUTTON))
    {
        if (point_in_rectangle(game_object->mouse_pos, this->bg_toggle_hitbox))
        {
            this->bg_light = !this->bg_light;
            if (this->bg_light)
            {
                game_object->colours = palette::light();
            }
            else
            {
                game_object->colours = palette::dark();
            }
        }
        else if (point_in_rectangle(game_object->mouse_pos, this->back_hitbox))
        {
            game_object->change_screen(start_screen::instance());
        }
    }
    else
    {
        this->bg_light = game_object->colours.name == "light";
    }
}

void settings_screen::render(game *game_object)
{
    draw_text("Settings", game_object->colours.text, "font_bold", 60, 250, 10);
    draw_text("Colour", game_object->colours.text, "font_bold", 48, 20, 85);
    draw_text("Light", game_object->colours.text, "font_bold", 36, 20, 160);
    draw_text("Dark", game_object->colours.text, "font_bold", 36, 220, 160);

    string toggle_name = this->bg_light ? "toggle_left" : "toggle_right";
    draw_bitmap(toggle_name, this->bg_toggle_hitbox.x, this->bg_toggle_hitbox.y);

    this->back_button->render(game_object);
}

settings_screen::~settings_screen()
{
    delete this->back_button;
}
#pragma endregion

#pragma region game overlay
game_overlay::game_overlay() : screen(screen_type::OVERLAY)
{
    this->ui_draw_opts = option_scale_bmp(0.81, 0.82);
    this->pause_button = new symbol_btn(rectangle_from(600, 20, 53, 53), "pause", []()
                                        { game::instance()->change_screen(pause_screen::instance()); }, option_scale_bmp(0.75, 0.75));
}

void game_overlay::handle_item_update(item_update_result result, int pos, game *game_obj)
{
    item_stack *stack = game_obj->ground[pos];
    switch (result)
    {
    case item_update_result::RCLICK:
        game_obj->cur_mouse_right = false;
        game_obj->player_object->move_to_storage(stack);
        game_obj->player_object->sync_attributes();
        break;

    case item_update_result::SRCLICK:
        game_obj->cur_mouse_right = false;
        stack->burn();
        break;

    default:
        break;
    }
}

game_overlay *game_overlay::instance()
{
    static game_overlay *inst = new game_overlay();
    return inst;
}

void game_overlay::update(game *game_object)
{
    this->pause_button->update(game_object);
    for (int i = 0; i < game_object->ground.length(); i++)
    {
        item_update_result result = game_object->ground[i]->update(game_object);
        this->handle_item_update(result, i, game_object);
    }
}

void game_overlay::render(game *game_object)
{
    draw_bitmap("ui_boil1_0", -90, -105, this->ui_draw_opts);
    this->pause_button->render(game_object);
    for (int i = 0; i < game_object->ground.length(); i++)
    {
        game_object->ground[i]->render(game_object);
    }
}

game_overlay::~game_overlay()
{
    delete this->pause_button;
}
#pragma endregion

#pragma region battle upper
battle_upper::battle_upper() : screen(screen_type::UPPER, true)
{
    this->flee_button = new default_btn(point_at(20, 20), []()
                                        { game::instance()->end_room(); }, "Flee", 30, "font_bold", "button", "button_pressed", option_scale_bmp(0.5, 0.5));
}

battle_upper *battle_upper::instance()
{
    static battle_upper *inst = new battle_upper();
    return inst;
}

void battle_upper::update(game *game_object)
{
    this->flee_button->update(game_object);
}

void battle_upper::render(game *game_object)
{
    this->flee_button->render(game_object);
    for (int i = 0; i < game_object->enemies.length(); i++)
    {
        game_object->enemies[i]->render(game_object);
    }
}

battle_upper::~battle_upper()
{
}
#pragma endregion

#pragma region battle lower
battle_lower::battle_lower() : screen(screen_type::LOWER)
{
}

battle_lower *battle_lower::instance()
{
    static battle_lower *inst = new battle_lower();
    return inst;
}

void battle_lower::update(game *game_object)
{
    game_object->player_object->update(game_object);
}

void battle_lower::render(game *game_object)
{
    game_object->player_object->render(game_object);
}

battle_lower::~battle_lower()
{
}
#pragma endregion

#pragma region pause screen
pause_screen::pause_screen() : screen(screen_type::FULL)
{
    this->resume_button = new default_btn(rectangle_from(220, 200, 262, 112), []()
                                          { game::instance()->back(); }, "Resume", point_at(270, 225));
    this->saq_button = new default_btn(rectangle_from(220, 400, 262, 112), []()
                                       { 
                                game *inst = game::instance(); 
                                inst->clear();
                                inst->reset();
                                inst->change_screen(start_screen::instance()); }, "Save & Quit", point_at(245, 425), 40);
}

pause_screen *pause_screen::instance()
{
    static pause_screen *inst = new pause_screen();
    return inst;
}

void pause_screen::update(game *game_object)
{
    this->resume_button->update(game_object);
    this->saq_button->update(game_object);
}

void pause_screen::render(game *game_object)
{
    draw_text("Game Paused", game_object->colours.text, "font_bold", 60, 175, 10);
    this->resume_button->render(game_object);
    this->saq_button->render(game_object);
}

pause_screen::~pause_screen()
{
    delete this->resume_button;
    delete this->saq_button;
}
#pragma endregion

#pragma region attack lower
attack_lower::attack_lower() : screen(screen_type::LOWER)
{
    this->buttons = dynamic_array<beside_btn *>();
    this->button_anchor = point_at(65, 480);
    this->button_gap = point_at(0, 20);
    this->button_draw_opts = option_scale_bmp(0.7, 0.7);
    this->back_button = new beside_btn([]()
                                       { game::instance()->back(); }, "Back", point_at(600, 700), "select_triangle", this->button_draw_opts);
}

void attack_lower::regenerate(game *game_obj, inv_slot slot)
{
    for (int i = 0; i < this->buttons.length(); i++)
    {
        delete this->buttons[i];
    }
    this->buttons.clear();

    auto &entities = game_obj->enemies;
    int current_y = this->button_anchor.y;
    for (int i = 0; i < entities.length(); i++)
    {
        beside_btn *btn = new beside_btn([game_obj, entities, i, slot]()
                                         { game_obj->player_object->attack(game_obj, entities[i], slot); }, entities[i]->name, point_at(this->button_anchor.x, current_y), "select_triangle", this->button_draw_opts);
        current_y = btn->hitbox.y + btn->hitbox.height + this->button_gap.y;
        this->buttons.add(btn);
    }
}

attack_lower *attack_lower::instance(game *game_obj, inv_slot slot)
{
    static attack_lower *inst = new attack_lower();
    inst->regenerate(game_obj, slot);
    return inst;
}

void attack_lower::update(game *game_obj)
{
    for (int i = 0; i < this->buttons.length(); i++)
    {
        this->buttons[i]->update(game_obj);
    }
    this->back_button->update(game_obj);
}

void attack_lower::render(game *game_obj)
{
    for (int i = 0; i < this->buttons.length(); i++)
    {
        this->buttons[i]->render(game_obj);
    }
    this->back_button->render(game_obj);
}

attack_lower::~attack_lower()
{
    for (int i = 0; i < this->buttons.length(); i++)
    {
        delete this->buttons[i];
    }
    this->buttons.clear();

    delete this->back_button;
}
#pragma endregion

#pragma region post battle upper
post_battle_upper::post_battle_upper() : screen(screen_type::UPPER)
{
}

post_battle_upper *post_battle_upper::instance()
{
    post_battle_upper *inst = new post_battle_upper();
    return inst;
}

void post_battle_upper::update(game *game_object)
{
    if (game_object->prev_mouse_right && game_object->cur_mouse_right && point_in_rectangle(game_object->mouse_pos, screen::upper_mask))
    {
        int dx = game_object->mouse_pos.x - game_object->prev_mouse_pos.x;
        int dy = game_object->mouse_pos.y - game_object->prev_mouse_pos.y;
        for (int i = 0; i < game_object->map.length(); i++)
        {
            game_object->map[i]->move(dx, dy);
        }
    }

    for (int i = 0; i < game_object->map.length(); i++)
    {
        game_object->map[i]->update(game_object);
    }
}

void post_battle_upper::render(game *game_object)
{
    for (int i = 0; i < game_object->map.length(); i++)
    {
        game_object->map[i]->render(game_object);
    }
}

post_battle_upper::~post_battle_upper()
{
}
#pragma endregion

#pragma region game over
game_over::game_over() : screen(screen_type::FULL)
{
    this->to_menu_button = new default_btn(point_at(200, 500), []()
                                           {
        game *inst = game::instance();
        inst->clear();
        inst->reset();
        inst->change_screen(start_screen::instance()); }, "Continue");
}

game_over *game_over::instance()
{
    static game_over *inst = new game_over();
    return inst;
}

void game_over::update(game *game_object)
{
    this->to_menu_button->update(game_object);
}

void game_over::render(game *game_object)
{
    draw_text("You Died", game_object->colours.text, "font_bold", 72, 200, 210, option_defaults());
    this->to_menu_button->render(game_object);
}

game_over::~game_over()
{
    delete this->to_menu_button;
}
#pragma endregion

#pragma region npc upper
npc_upper::npc_upper() : screen(screen_type::UPPER)
{
    this->next_button = new default_btn(point_at(20, 20), []()
                                        { game::instance()->end_room(); }, "Next", 30, "font_bold", "button", "button_pressed", option_scale_bmp(0.5, 0.5));
}

npc_upper *npc_upper::instance()
{
    static npc_upper *inst = new npc_upper();
    return inst;
}

void npc_upper::update(game *game_obj)
{
    for (int i = 0; i < game_obj->npcs.length(); i++)
    {
        game_obj->npcs[i]->update(game_obj);
    }
    this->next_button->update(game_obj);
}

void npc_upper::render(game *game_obj)
{
    for (int i = 0; i < game_obj->npcs.length(); i++)
    {
        game_obj->npcs[i]->render(game_obj);
    }
    this->next_button->render(game_obj);
}

npc_upper::~npc_upper()
{
    delete this->next_button;
}
#pragma endregion