#include "entities.h"
#include "splashkit.h"
#include "splashkit-arrays.h"
#include "game.h"
#include "items.h"
#include "screens.h"
#include <cstdarg>
#include <vector>
#include <functional>
#include <iterator>

timer create_or_get_timer(string name)
{
    if (has_timer(name))
    {
        return timer_named(name);
    }
    else
    {
        timer timer = create_timer(name);
        start_timer(timer);
        return timer;
    }
}

ticker disappearing_text(string text, color start_colour, point_2d pos, int delay, string font, int font_size)
{
    string id = to_string(uid());
    return ticker{id, [id, text, pos, font, font_size, start_colour, delay](game *game_obj)
                  {
                      timer timer_obj = create_or_get_timer(id);
                      int time = timer_ticks(timer_obj);
                      color colour = start_colour;
                      colour.a = 1 - ((float)time / (float)delay);
                      if (time < delay)
                      {
                          draw_text(text, colour, font, font_size, pos.x, pos.y, option_defaults());
                          return true;
                      }
                      return false;
                  }};
}

#pragma region npc
npc::npc(string name, string sprite, drawing_options draw_opts, point_2d loc)
{
    this->name = name;
    this->sprite = sprite;
    this->draw_opts = draw_opts;
    this->hitbox = scale_rectangle(this->sprite, loc, this->draw_opts);
    this->render_at = scale_render_pos(this->sprite, loc, this->draw_opts);
}

void npc::update(game *game_obj) {}

void npc::render(game *game_obj)
{
    draw_bitmap(this->sprite, this->render_at.x, this->render_at.y, this->draw_opts);
}

npc::~npc() {}
#pragma endregion

#pragma region chest
chest::chest() : npc("Chest", "chest_closed", option_scale_bmp(0.8, 0.8), point_at(250, 180))
{
    this->opened = false;
    auto text_colour = []()
    { return game::instance()->colours.text; };
    this->on_hover = (new tooltip())
                         ->add("Chest", 18, text_colour)
                         ->newline()
                         ->blank_line()
                         ->add("Click to open", 15, text_colour)
                         ->newline()
                         ->generate();
}

void chest::update(game *game_obj)
{
    if (!this->opened)
    {
        if (point_in_rectangle(game_obj->mouse_pos, this->hitbox))
        {
            game_obj->set_tooltip(this->on_hover);
            game_obj->render_tooltip();

            if (game_obj->prev_mouse_left && !game_obj->cur_mouse_left)
            {
                this->sprite = "chest_open";
                this->hitbox = scale_rectangle(this->sprite, point_at(250, 113), this->draw_opts);
                this->render_at = scale_render_pos(this->sprite, point_at(250, 113), this->draw_opts);
                this->opened = true;
            }
        }
    }
}

json chest::save()
{
    json save = create_json();
    return save;
}

void chest::load(json from)
{
}

chest::~chest()
{
    delete this->on_hover;
}
#pragma endregion

#pragma region merchant
merchant_schema merchant::schema{};

merchant::merchant() : npc("Joash", "merchant", option_scale_bmp(0.7, 0.7), point_at(100, 75))
{
    std::function<color(void)> text_colour = []()
    { return game::instance()->colours.text; };
    this->hover = (new tooltip())
                      ->add(this->name, 18, text_colour)
                      ->newline()
                      ->generate();
    point_2d menu_pos = point_at(315, 80);
    this->menu_draw_opts = option_scale_bmp(0.8, 0.8);
    this->menu_render_at = scale_render_pos("shop_ui", menu_pos, this->menu_draw_opts);
    this->wares = dynamic_array<std::pair<item_stack *, item_stack *>>();
    drawing_options wares_draw_opts = option_scale_bmp(0.7, 0.7);
    drawing_options price_draw_opts = option_scale_bmp(0.5, 0.5);
    for (int i = 0; i < merchant::WARE_COUNT; i++)
    {
        int ind = rnd(0, item::registry.size() - 1);
        auto iter = item::registry.begin();
        std::advance(iter, ind);
        item *item_obj = iter->second;
        this->wares.add({new item_stack(item_obj, rnd(1, item_obj->max), wares_draw_opts, point_at(menu_pos.x + 40 + (i % 4) * 72, menu_pos.y + 75 + (i / 4) * 88)), new item_stack(coin, rnd(1, 2), price_draw_opts, point_at(menu_pos.x + 41 + (i % 4) * 71, menu_pos.y + 112 + (i / 4) * 88))});
    }
}

void merchant::update(game *game_obj)
{
    if (point_in_rectangle(game_obj->mouse_pos, this->hitbox))
    {
        game_obj->set_tooltip(this->hover);
        game_obj->render_tooltip();
    }
    for (int i = 0; i < this->wares.length(); i++)
    {
        item_update_result result = this->wares[i].first->update(game_obj);
        if (result == item_update_result::LCLICK)
        {
            for (int j = 0; j < game_obj->player_object->inv.length(); j++)
            {
                item_stack *stack = game_obj->player_object->inv[j];
                if (stack->get_contains() == coin)
                {
                    bool success = stack->increment(-this->wares[i].second->get_count());
                    game_obj->player_object->move_to_storage(this->wares[i].first);
                }
            }
        }
        this->wares[i].second->update(game_obj);
    }
}

json merchant::save()
{
    json save = create_json();

    std::vector<json> ware_jsons;
    for (int i = 0; i < this->wares.length(); i++)
    {
        ware_jsons.push_back(this->wares[i].first->save());
    }
    json_set_array(save, merchant::schema.items, ware_jsons);

    std::vector<json> cost_jsons;
    for (int i = 0; i < this->wares.length(); i++)
    {
        cost_jsons.push_back(this->wares[i].second->save());
    }
    json_set_array(save, merchant::schema.costs, cost_jsons);

    return save;
}

void merchant::load(json from)
{
    std::vector<json> ware_jsons;
    json_read_array(from, merchant::schema.items, ware_jsons);
    std::vector<json> cost_jsons;
    json_read_array(from, merchant::schema.costs, cost_jsons);
    for (int i = 0; i < this->wares.length(); i++)
    {
        this->wares[i].first->load(ware_jsons[i]);
        this->wares[i].second->load(cost_jsons[i]);
    }
}

void merchant::render(game *game_obj)
{
    npc::render(game_obj);
    draw_bitmap("shop_ui", this->menu_render_at.x, this->menu_render_at.y, this->menu_draw_opts);
    for (int i = 0; i < this->wares.length(); i++)
    {
        this->wares[i].first->render(game_obj);
        this->wares[i].second->render(game_obj);
    }
}

merchant::~merchant()
{
    delete this->hover;
    for (int i = 0; i < this->wares.length(); i++)
    {
        delete this->wares[i].first;
        delete this->wares[i].second;
    }
}
#pragma endregion

#pragma region entity
entity::entity(string name, string sprite, drawing_options draw_opts, point_2d loc, attrmap starting_attributes)
{
    this->name = name;
    this->sprite = sprite;
    this->draw_opts = draw_opts;
    this->hitbox = scale_rectangle(this->sprite, loc, this->draw_opts);
    this->render_at = scale_render_pos(this->sprite, loc, this->draw_opts);
    this->attributes = starting_attributes;
    this->original_attributes = starting_attributes;
    this->statuses = std::map<status, int>{};
}

void entity::render(game *game_obj)
{
    draw_bitmap(this->sprite, this->render_at.x, this->render_at.y, this->draw_opts);
}

std::pair<bool, int> entity::take_damage(game *game_obj, entity *origin, int damage, attribute res_type)
{
    std::pair<bool, int> taken{false, damage * (1 - ((float)this->attributes[res_type] / 100.0))};
    this->current_hp -= taken.second;
    if (this->current_hp <= 0)
    {
        this->current_hp = 0;
        taken.first = true;
    }
    return taken;
}

void entity::add_attributes(attrmap &to_add)
{
    for (int i = 0; i != (int)attribute::DUMMY_LAST; i++)
    {
        attribute attr = (attribute)i;
        this->attributes[attr] += to_add[attr];
    }
}

void entity::remove_attributes(attrmap &to_remove)
{
    for (int i = 0; i != (int)attribute::DUMMY_LAST; i++)
    {
        attribute attr = (attribute)i;
        this->attributes[attr] -= to_remove[attr];
    }
}

void entity::sync_attributes()
{
    this->attributes = this->original_attributes;
    for (const auto [stat, turns] : this->statuses)
    {
        switch (stat)
        {
        case status::BLINDED:
            this->attributes[attribute::MELEE_MOD] -= 20;
            this->attributes[attribute::RANGED_MOD] -= 20;
            break;
        }
    }
}

void entity::end_of_turn(game *game_obj)
{
    std::vector<status> to_remove;
    for (const auto [stat, turns] : this->statuses)
    {
        this->statuses[stat] -= 1;
        if (turns - 1 <= 0)
        {
            to_remove.push_back(stat);
        }
    }
    for (status stat : to_remove)
    {
        this->statuses.erase(stat);
    }
    this->sync_attributes();
}

int entity::calc_outgoing_dmg(item *weapon)
{
    int damage;
    if (weapon->type == item_type::MELEE)
    {
        damage = (this->attributes[attribute::MELEE_DMG] + weapon->attributes[attribute::MELEE_DMG]) * (1 + (float)this->attributes[attribute::MELEE_MOD] / 100.0);
    }
    else
    {
        damage = (this->attributes[attribute::RANGED_DMG] + weapon->attributes[attribute::RANGED_DMG]) * (1 + (float)this->attributes[attribute::RANGED_MOD] / 100.0);
    }
    return damage;
}

entity::~entity()
{
}
#pragma endregion

#pragma region player
player_schema player::schema{};

void player::handle_item_update(item_update_result result, int pos, game *game_obj)
{
    item_stack *stack = this->inv[pos];
    switch (result)
    {
    case item_update_result::LCLICK:
        switch (stack->get_contains()->type)
        {
        case item_type::CONSUMABLE:
            if (!game_obj->in_battle || game_obj->turn == -1)
            {
                stack->get_contains()->consume(this);
                stack->increment(-1);
            }
            break;

        case item_type::ACCESSORY:
            if (!game_obj->in_battle)
            {
                if (pos < (int)inv_slot::STORAGE)
                {
                    this->move_to_storage(stack);
                }
                else
                {
                    this->move_to_accessories(stack);
                }
                this->sync_attributes();
            }
            break;

        case item_type::MELEE:
            if (game_obj->in_battle && game_obj->turn == -1 && pos == (int)inv_slot::MELEE)
            {
                this->start_attack(game_obj, inv_slot::MELEE);
            }
            else if (!game_obj->in_battle)
            {
                if (pos == (int)inv_slot::MELEE)
                {
                    this->move_to_storage(stack);
                }
                else
                {
                    stack->swap(this->inv[(int)inv_slot::MELEE]);
                }
                this->sync_attributes();
            }
            break;

        case item_type::STAFF:
            if (game_obj->in_battle && game_obj->turn == -1 && pos == (int)inv_slot::RANGED)
            {
                if (this->current_mana >= this->attributes[attribute::MANA_COST])
                {
                    this->start_attack(game_obj, inv_slot::RANGED);
                }
                else
                {
                }
            }
            else if (!game_obj->in_battle)
            {
                if (pos == (int)inv_slot::RANGED)
                {
                    this->move_to_storage(stack);
                }
                else
                {
                    stack->swap(this->inv[(int)inv_slot::RANGED]);
                }
                this->sync_attributes();
            }
            break;

        default:
            break;
        }
        break;

    case item_update_result::RCLICK:
        game_obj->cur_mouse_right = false;
        game_obj->drop(stack);
        this->sync_attributes();
        break;

    case item_update_result::SRCLICK:
        game_obj->cur_mouse_right = false;
        stack->burn();
        this->sync_attributes();
        break;

    default:
        break;
    }
}

player::player() : entity("You", "lower_boil1_0", option_scale_bmp(0.81, 0.81), point_at(31, 481))
{
    this->hp_bar = rectangle_from(40, 520, 175, 30);
    this->mana_bar = rectangle_from(40, 605, 178, 30);
    this->boil_bar = rectangle_from(40, 690, 175, 30);

    this->melee_slot = point_at(310, 525);
    this->ranged_slot = point_at(310, 645);
    this->acc_slot_anchor = point_at(400, 500);
    this->acc_slot_gap = 95;
    this->inv_anchor = point_at(395, 595);
    this->inv_x_gap = 50;
    this->inv_y_gap = 50;
    this->large_slot_opts = option_scale_bmp(1.15, 1.15);
    this->small_slot_opts = option_scale_bmp(0.85, 0.85);

    this->max_hp = 200;
    this->current_hp = 200;
    this->max_mana = 100;
    this->current_mana = 100;
    this->max_boil = 100;
    this->current_boil = 0;
    this->mana_regen = 10;
    this->inv = fixed_array<item_stack *, 20>();
    this->inv[(int)inv_slot::MELEE] = new item_stack(dagger, 1, this->large_slot_opts, this->melee_slot);
    this->inv[(int)inv_slot::RANGED] = new item_stack(basic_staff, 1, this->large_slot_opts, this->ranged_slot);
    for (int i = 0; i < 3; i++)
    {
        this->inv[(int)inv_slot::ACCESSORY + i] = new item_stack(this->large_slot_opts, point_at(this->acc_slot_anchor.x + this->acc_slot_gap * i, this->acc_slot_anchor.y));
    }
    for (int i = 0; i < 15; i++)
    {
        this->inv[(int)inv_slot::STORAGE + i] = new item_stack(this->small_slot_opts, point_at(this->inv_anchor.x + this->inv_x_gap * (i % 5), this->inv_anchor.y + this->inv_y_gap * (i / 5)));
    }
    // delete this->inv[(int)inv_slot::STORAGE];
    // this->inv[(int)inv_slot::STORAGE] = new item_stack(felknight_chestplate, 1, this->small_slot_opts, point_at(this->inv_anchor.x + this->inv_x_gap * (0 % 5), this->inv_anchor.y + this->inv_y_gap * (0 / 5)));
    this->sync_attributes();
}

void player::update(game *game_obj)
{
    item_update_result melee_result = this->inv[(int)inv_slot::MELEE]->update(game_obj);
    this->handle_item_update(melee_result, (int)inv_slot::MELEE, game_obj);

    item_update_result ranged_result = this->inv[(int)inv_slot::RANGED]->update(game_obj);
    this->handle_item_update(ranged_result, (int)inv_slot::RANGED, game_obj);

    for (int i = 0; i < 3; i++)
    {
        int slot = (int)inv_slot::ACCESSORY + i;
        item_update_result acc_result = this->inv[slot]->update(game_obj);
        this->handle_item_update(acc_result, slot, game_obj);
    }

    for (int i = 0; i < 15; i++)
    {
        int slot = (int)inv_slot::STORAGE + i;
        item_update_result storage_result = this->inv[slot]->update(game_obj);
        this->handle_item_update(storage_result, slot, game_obj);
    }
}

void player::render(game *game_obj)
{
    draw_bitmap(this->sprite, this->render_at.x, this->render_at.y, this->draw_opts);

    draw_text("Health", game_obj->colours.text, "font_reg", 36, 40, 480);
    fill_rectangle(palette::hp_colour, this->hp_bar.x, this->hp_bar.y, this->hp_bar.width * ((float)this->current_hp / (float)this->max_hp), this->hp_bar.height);
    draw_text(to_string(this->current_hp) + "/" + to_string(this->max_hp), game_obj->colours.text, "font_bold", 24, 80, 520);

    draw_text("Mana", game_obj->colours.text, "font_reg", 36, 40, 565);
    fill_rectangle(palette::mana_colour, this->mana_bar.x, this->mana_bar.y, this->mana_bar.width * ((float)this->current_mana / (float)this->max_mana), this->mana_bar.height);
    draw_text(to_string(this->current_mana) + "/" + to_string(this->max_mana), game_obj->colours.text, "font_bold", 24, 80, 608);

    draw_text("Boil", game_obj->colours.text, "font_reg", 36, 40, 650);
    fill_rectangle(palette::line_colour, this->boil_bar.x, this->boil_bar.y, this->boil_bar.width * ((float)this->current_boil / (float)this->max_boil), this->boil_bar.height);
    draw_text(to_string(this->current_boil) + "/" + to_string(this->max_boil), game_obj->colours.text, "font_bold", 24, 80, 690);

    int inv_size = this->inv.length();
    for (int i = 0; i < inv_size; i++)
    {
        this->inv[i]->render(game_obj);
    }

    int status_render_y = 100;
    for (const auto [stat, turns] : this->statuses)
    {
        draw_bitmap("status_icon_" + to_string((int)stat), 610, status_render_y);
        draw_text(to_string(turns), game_obj->colours.text, "font_bold", 18, 640, status_render_y + 40);
        status_render_y += 60;
    }
}

void player::attack(game *game_obj, enemy *target, inv_slot slot)
{
    game_obj->change_screen(battle_lower::instance());
    bool dead = this->inv[(int)slot]->get_contains()->attack(game_obj, this, target, this->calc_outgoing_dmg(this->inv[(int)slot]->get_contains()));
    if (slot == inv_slot::RANGED)
    {
        int mana_decrement = this->attributes[attribute::MANA_COST] + this->inv[(int)slot]->get_contains()->attributes[attribute::MANA_COST];
        this->current_mana -= mana_decrement;
        string cost_text = "-" + to_string(mana_decrement);
        battle_lower::instance()->tickers.add(disappearing_text(cost_text, game_obj->colours.mana_colour, point_at(this->mana_bar.x + this->mana_bar.width + 10, this->mana_bar.y + this->mana_bar.height / 2 - text_height(cost_text, "font_bold", 32) / 2)));
    }

    if (dead)
    {
        this->current_boil -= target->attributes[attribute::BOIL_REDUCTION];
        if (this->current_boil < 0)
            this->current_boil = 0;
        target->on_death(game_obj, this);
    }
    if (game_obj->enemies.length() == 0)
    {
        game_obj->end_room();
    }

    game_obj->progress_turn();
    string timer_id = to_string(uid());
    battle_upper::instance()->tickers.add(ticker{timer_id, [timer_id](game *game_obj)
                                                 {
                                                     timer timer_obj = create_or_get_timer(timer_id);
                                                     int time = timer_ticks(timer_obj);
                                                     if (time < 2100)
                                                     {
                                                         return true;
                                                     }
                                                     game_obj->next_attack();
                                                     return false;
                                                 }});
}

std::pair<bool, int> player::take_damage(game *game_obj, entity *origin, int damage, attribute res_type)
{
    auto dead = entity::take_damage(game_obj, origin, damage, res_type);
    int plus_boil = origin->attributes[attribute::BOIL_INCREASE] * (1 - ((float)this->attributes[attribute::BOIL_DEFENSE]) / 100.0);
    this->current_boil += plus_boil;
    if (this->current_boil >= this->max_boil)
    {
        return std::pair<bool, int>{true, dead.second};
    }

    string dmg_text = "-" + to_string(dead.second);
    battle_lower::instance()->tickers.add(disappearing_text(dmg_text, game_obj->colours.hp_colour, point_at(this->hp_bar.x + this->hp_bar.width + 10, this->hp_bar.y + this->hp_bar.height / 2 - text_height(dmg_text, "font_bold", 32) / 2)));

    string boil_text = "+" + to_string(plus_boil);
    battle_lower::instance()->tickers.add(disappearing_text(boil_text, game_obj->colours.line_colour, point_at(this->boil_bar.x + this->boil_bar.width + 10, this->boil_bar.y + this->boil_bar.height / 2 - text_height(boil_text, "font_bold", 32) / 2)));

    return dead;
}

void player::on_death(game *game_obj, entity *origin)
{
    game_obj->end_room();
    game_obj->change_screen(game_over::instance());
    game_obj->complete = true;
}

bool player::move_to_storage(item_stack *stack)
{
    int size = this->inv.length();
    for (int i = (int)inv_slot::STORAGE; i < size; i++)
    {
        if (this->inv[i]->add_stack(stack))
        {
            return true;
        }
    }
    return false;
}

bool player::move_to_accessories(item_stack *stack)
{
    for (int i = (int)inv_slot::ACCESSORY; i < (int)inv_slot::STORAGE; i++)
    {
        if (this->inv[i]->add_stack(stack))
        {
            return true;
        }
    }
    return false;
}

void player::sync_attributes()
{
    entity::sync_attributes();
    for (int i = 0; i < (int)inv_slot::STORAGE; i++)
    {
        if (!this->inv[i]->is_empty())
        {
            this->add_attributes(this->inv[i]->get_contains()->modifiers);
        }
    }
}

void player::start_attack(game *game_obj, inv_slot slot)
{
    game_obj->change_screen(attack_lower::instance(game_obj, slot));
}

json player::save()
{
    json save = create_json();
    json_set_number(save, player::schema.current_hp, this->current_hp);
    json_set_number(save, player::schema.current_mana, this->current_mana);
    json_set_number(save, player::schema.current_boil, this->current_boil);

    std::vector<json> inv_data;
    for (int i = 0; i < this->inv.length(); i++)
    {
        inv_data.push_back(this->inv[i]->save());
    }
    json_set_array(save, player::schema.inv, inv_data);
    return save;
}

void player::load(json from)
{
    this->current_hp = json_read_number_as_int(from, player::schema.current_hp);
    this->current_mana = json_read_number_as_int(from, player::schema.current_mana);
    this->current_boil = json_read_number_as_int(from, player::schema.current_boil);

    vector<json> inv_data;
    json_read_array(from, player::schema.inv, inv_data);
    for (size_t i = 0; i < inv_data.size(); i++)
    {
        this->inv[i]->load(inv_data[i]);
    }

    this->sync_attributes();
}

player::~player()
{
    for (int i = 0; i < this->inv.length(); i++)
    {
        delete this->inv[i];
    }
}
#pragma endregion

#pragma region enemy
std::map<int, enemy_ctor> enemy::registry{};
std::map<int, std::vector<enemy_ctor>> enemy::buckets{};
std::vector<enemy_ctor> enemy::bosses{};

void enemy::init()
{
    for (int i = 1; i < enemy::BUCKET_COUNT + 1; i++)
    {
        enemy::buckets.emplace(i * enemy::BUCKET_WIDTH, std::vector<enemy_ctor>{});
    }
    enemy::registrate({ectorer(dagger_man), ectorer(recruit), ectorer(glorngus), ectorer(gravedigger), ectorer(nightwatch), ectorer(amalgam), ectorer(glorngus_evolved), ectorer(felknight), ectorer(glorngus_ex)});
}

void enemy::registrate(std::vector<enemy_ctor> registrees)
{
    static point_2d origin = point_at(0, 0);
    for (enemy_ctor registree : registrees)
    {
        enemy *inst = registree(origin);
        int value = inst->value;
        enemy::registry.emplace(inst->get_typeid(), registree);

        bool placed = false;
        for (auto &[max, list] : enemy::buckets)
        {
            if (value <= max)
            {
                list.push_back(registree);
                placed = true;
                break;
            }
        }
        if (!placed)
        {
            write_line("\e[0;31mWARNING: Enemy with value over max found: " + inst->name + "\e[0m");
        }

        delete inst;
    }
}

enemy::enemy(string name, string sprite, drawing_options draw_opts, point_2d loc, int max_hp, int coin_min, int coin_max, int value, attrmap attributes) : entity(name, sprite, draw_opts, loc, attributes)
{
    this->weapon = new item_stack();
    this->max_hp = max_hp;
    this->current_hp = max_hp;
    this->info_size = 24;
    this->info_font = "font_bold";
    this->coin_min = coin_min;
    this->coin_max = coin_max;
    this->value = value;

    this->hp_coords = point_at(this->hitbox.x + this->hitbox.width / 2 - text_width(to_string(this->current_hp) + "/" + to_string(this->max_hp), this->info_font, this->info_size) / 2, this->hitbox.y - text_height(to_string(this->current_hp) + "/" + to_string(this->max_hp), this->info_font, this->info_size));

    this->name_coords = point_at(this->hitbox.x + this->hitbox.width / 2 - text_width(this->name, this->info_font, this->info_size) / 2, this->hp_coords.y - text_height(this->name, this->info_font, this->info_size));
}

void enemy::render(game *game_obj)
{
    entity::render(game_obj);
    draw_text(to_string(this->current_hp) + "/" + to_string(this->max_hp), game_obj->colours.hp_colour, this->info_font, this->info_size, this->hp_coords.x, this->hp_coords.y);
    draw_text(this->name, game_obj->colours.text, this->info_font, this->info_size, this->name_coords.x, this->name_coords.y);
}

std::pair<bool, int> enemy::take_damage(game *game_obj, entity *origin, int damage, attribute res_type)
{
    auto result = entity::take_damage(game_obj, origin, damage, res_type);
    battle_upper::instance()->tickers.add(disappearing_text("-" + to_string(result.second), game_obj->colours.hp_colour, point_at(this->hp_coords.x + 70, this->hp_coords.y), 2000, "font_bold", 24));
    return result;
}

void enemy::on_death(game *game_obj, entity *origin)
{
    this->drop(game_obj);
    game_obj->remove_entity(this);
}

void enemy::attack(game *game_obj, entity *target)
{
    if (this->weapon->get_contains()->attack(game_obj, this, target, this->calc_outgoing_dmg(this->weapon->get_contains())))
    {
        target->on_death(game_obj, this);
    }
    game_obj->progress_turn();
    string timer_id = to_string(uid());
    battle_upper::instance()->tickers.add(ticker{timer_id, [timer_id](game *game_obj)
                                                 {
                                                     timer timer_obj = create_or_get_timer(timer_id);
                                                     int time = timer_ticks(timer_obj);
                                                     if (time < 2100)
                                                     {
                                                         return true;
                                                     }
                                                     game_obj->next_attack();
                                                     return false;
                                                 }});
}

void enemy::start_attack(game *game_obj)
{
    this->attack(game_obj, game_obj->player_object);
}

void enemy::sync_attributes()
{
    entity::sync_attributes();
    if (!this->weapon->is_empty())
    {
        this->add_attributes(this->weapon->get_contains()->modifiers);
    }
}

void enemy::drop(game *game_obj)
{
    int coin_count = rnd(this->coin_min, this->coin_max);
    if (coin_count > 0)
    {
        game_obj->drop(coin, coin_count);
    }
}

int enemy::get_typeid()
{
    throw "";
}

enemy::~enemy()
{
    delete this->weapon;
}
#pragma endregion

#pragma region enemies
dagger_man::dagger_man(point_2d loc) : enemy("Dagger Man", "dagger_man_0", option_scale_bmp(0.6, 0.6), loc, 10, 10, 30, 5, make_attrmap({{attribute::BOIL_INCREASE, 10}, {attribute::BOIL_REDUCTION, 20}}))
{
    this->weapon->alter(dagger, 1);
    this->sync_attributes();
}

void dagger_man::drop(game *game_obj)
{
    enemy::drop(game_obj);
    game_obj->drop(this->weapon);
}

int dagger_man::get_typeid()
{
    return dagger_man::type_id;
}

recruit::recruit(point_2d loc) : enemy("Recruit", "recruit_0", option_scale_bmp(0.6, 0.5), loc, 14, 12, 30, 10, make_attrmap({{attribute::BOIL_INCREASE, 10}, {attribute::BOIL_REDUCTION, 30}, {attribute::MELEE_DEF, 20}}))
{
    this->weapon->alter(pike, 1);
    this->sync_attributes();
}

void recruit::drop(game *game_obj)
{
    enemy::drop(game_obj);
    game_obj->drop(this->weapon);
}

int recruit::get_typeid()
{
    return recruit::type_id;
}

glorngus::glorngus(point_2d loc) : enemy("Glorngus", "glorngus_0", option_scale_bmp(0.6, 0.6), loc, 20, 0, 5, 15, make_attrmap({{attribute::BOIL_INCREASE, 3}, {attribute::BOIL_REDUCTION, 30}}))
{
    this->weapon->alter(glorngus_fists, 1);
    this->sync_attributes();
}

int glorngus::get_typeid()
{
    return glorngus::type_id;
}

gravedigger::gravedigger(point_2d loc) : enemy("Gravedigger", "gravedigger_0", option_scale_bmp(0.6, 0.6), loc, 12, 25, 50, 20, make_attrmap({{attribute::BOIL_INCREASE, 15}, {attribute::BOIL_REDUCTION, 30}}))
{
    this->weapon->alter(shovel, 1);
    this->sync_attributes();
}

void gravedigger::attack(game *game_obj, entity *target)
{
    enemy::attack(game_obj, target);
    string timer_id = to_string(uid());
    int x = rnd(0, WIN_WIDTH);
    int y = rnd(0, WIN_HEIGHT);
    game_overlay::instance()->tickers.add(ticker{timer_id, [timer_id, x, y](game *game_obj)
                                                 {
                                                     timer timer_obj = create_or_get_timer(timer_id);
                                                     int time = timer_ticks(timer_obj);
                                                     if (time < 5000)
                                                     {
                                                         draw_bitmap("dirt_splatter_0", x, y);
                                                         return true;
                                                     }
                                                     return false;
                                                 }});
}

void gravedigger::drop(game *game_obj)
{
    enemy::drop(game_obj);
    game_obj->drop(this->weapon);
}

int gravedigger::get_typeid()
{
    return gravedigger::type_id;
}

nightwatch::nightwatch(point_2d loc) : enemy("Nightwatch", "nightwatch_0", option_scale_bmp(0.6, 0.6), loc, 25, 30, 55, 25, make_attrmap({{attribute::BOIL_INCREASE, 10}, {attribute::BOIL_REDUCTION, 30}}))
{
    this->weapon->alter(shotgun, 1);
    this->sync_attributes();
}

void nightwatch::attack(game *game_obj, entity *target)
{
    if (this->weapon->get_contains() == shotgun)
    {
        this->weapon->alter(nightwatch_lantern::instance(), 1);
    }
    else
    {
        this->weapon->alter(shotgun, 1);
    }
    this->sync_attributes();
    enemy::attack(game_obj, target);
}

void nightwatch::drop(game *game_obj)
{
    enemy::drop(game_obj);
    if (rnd(1, 2) == 1)
    {
        game_obj->drop(nightwatch_lantern::instance(), 1);
    }
    if (rnd(1, 2) == 2)
    {
        game_obj->drop(shotgun, 1);
    }
}

int nightwatch::get_typeid()
{
    return nightwatch::type_id;
}

amalgam::amalgam(point_2d loc) : enemy("Amalgam", "amalgam_0", option_scale_bmp(0.7, 0.7), loc, 40, 0, 50, 30, make_attrmap({{attribute::BOIL_INCREASE, 5}, {attribute::BOIL_REDUCTION, 10}}))
{
    this->weapon->alter(amalgam_strike, 1);
    this->sync_attributes();
}

int amalgam::get_typeid()
{
    return amalgam::type_id;
}

glorngus_evolved::glorngus_evolved(point_2d loc) : enemy("Glorngus Evolved", "glorngus_evolved_0", option_scale_bmp(0.6, 0.6), loc, 40, 0, 15, 35, make_attrmap({{attribute::BOIL_INCREASE, 9}, {attribute::BOIL_REDUCTION, 60}}))
{
    this->weapon->alter(glorngus_fists_evolved, 1);
    this->sync_attributes();
}

int glorngus_evolved::get_typeid()
{
    return glorngus_evolved::type_id;
}

felknight::felknight(point_2d loc) : enemy("Felknight", "felknight_0", option_scale_bmp(0.5, 0.5), loc, 50, 50, 80, 40, make_attrmap({{attribute::MELEE_DEF, 33}, {attribute::RANGED_DEF, 33}, {attribute::BOIL_INCREASE, 12}, {attribute::BOIL_REDUCTION, 40}}))
{
    this->weapon->alter(felknight_greatsword, 1);
    this->sync_attributes();
}

void felknight::drop(game *game_obj)
{
    enemy::drop(game_obj);
    game_obj->drop(this->weapon);
}

int felknight::get_typeid()
{
    return felknight::type_id;
}

glorngus_ex::glorngus_ex(point_2d loc) : enemy("Glorngus EX", "glorngus_ex_0", option_scale_bmp(0.6, 0.6), loc, 50, 40, 75, 45, make_attrmap({{attribute::MELEE_DEF, 50}, {attribute::RANGED_DEF, 25}, {attribute::BOIL_INCREASE, 15}, {attribute::BOIL_REDUCTION, 100}}))
{
    this->weapon->alter(glorngus_claymore::instance(), 1);
    this->sync_attributes();
}

void glorngus_ex::drop(game *game_obj)
{
    enemy::drop(game_obj);
    game_obj->drop(this->weapon);
}

int glorngus_ex::get_typeid()
{
    return glorngus_ex::type_id;
}

#pragma endregion

/**
 * Remaining enemies per bucket:
 * 1. Boiling Urchin, Charlatan
 * 2. Forager, Lone Star
 * 3. Aura Farmer, Magician
 * 4. Rejuvenator, Arquebusier
 * 5. Foot, Terrible Lizard
 * 6. Amalgam, Boss Bandit, Sorceror
 * 7. Golem, Rocketeer
 * 8. Felknight, Paladin, Warlock
 * 9. Hecatoncheires
 * 10. (Boss) Ades, Culex, Anophele
 */