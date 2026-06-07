#include "game.h"
#include "screens.h"
#include <cmath>
#include <vector>

int WIN_WIDTH = 700;
int WIN_HEIGHT = 800;

int uid()
{
    static int counter = 0;
    counter++;
    return counter;
}

double cndf(double value, double mean, double stdev)
{
    static double mult = std::sqrt((double)0.5);
    return std::erfc((-value + mean) / stdev * mult) / 2;
}

#pragma region palette
color palette::hp_colour = string_to_color("#E90808FF");
color palette::mana_colour = string_to_color("#AC49FFFF");
color palette::line_colour = string_to_color("#0036E9FF");
color palette::next_room_colour = string_to_color("#7FC800FF");

palette::palette(string name, color bg, color main, color text)
{
    this->name = name;
    this->bg = bg;
    this->main = main;
    this->text = text;
}

palette palette::light()
{
    static palette light_palette = palette("light", COLOR_WHITE, string_to_color("#0036e9ff"), COLOR_BLACK);
    return light_palette;
}

palette palette::dark()
{
    static palette dark_palette = palette("dark", COLOR_BLACK, string_to_color("#0036e9ff"), COLOR_WHITE);
    return dark_palette;
}
#pragma endregion

#pragma region room
room_schema room::schema{};

std::map<room_type::types, float> room_type::room_chances{
    {room_type::BATTLE_LV1, 0.20},
    {room_type::BATTLE_LV2, 0.16},
    {room_type::BATTLE_LV3, 0.16},
    {room_type::BATTLE_LV4, 0.16},
    {room_type::MERCHANT, 0.16},
    {room_type::CHEST, 0.16}};

room_type::types room_type::random_type()
{
    float value = rnd();
    for (const auto &[type, chance] : room_type::room_chances)
    {
        if (value <= chance)
            return type;
        value -= chance;
    }
    return room_type::BATTLE_LV1;
}

room *room::load(game *game_obj, json save, std::vector<json> &saves)
{
    room *new_room = new room((room_type::types)json_read_number_as_int(save, room::schema.type), option_scale_bmp(json_read_number_as_double(save, room::schema.scale_x), json_read_number_as_double(save, room::schema.scale_y)), point_at(json_read_number_as_int(save, room::schema.x), json_read_number_as_int(save, room::schema.y)));
    game_obj->map.add(new_room);
    new_room->id = json_read_number_as_int(save, room::schema.id);

    std::vector<double> next_rooms;
    json_read_array(save, room::schema.next, next_rooms);
    for (int id : next_rooms)
    {
        bool loaded = false;
        // Check if already loaded
        for (int i = 0; i < game_obj->map.length(); i++)
        {
            if (game_obj->map[i]->id == id)
            {
                new_room->next.add(game_obj->map[i]);
                loaded = true;
                break;
            }
        }

        if (loaded)
            continue;

        // Load if unloaded
        for (json data : saves)
        {
            if (json_read_number_as_int(data, room::schema.id) == id)
            {
                new_room->next.add(room::load(game_obj, data, saves));
                break;
            }
        }
    }

    return new_room;
}

room::room(room_type::types type, drawing_options draw_opts)
{
    this->id = uid();
    this->type = type;
    this->next = dynamic_array<room *>();
    this->draw_opts = draw_opts;
    this->sprite = "room_type_" + to_string(this->type);
    this->hover_sprite = this->sprite + "_hover";
    this->hitbox = rectangle_from(0, 0, 0, 0);
    this->render_at = point_at(0, 0);
    this->active = false;
    this->hover = false;
    this->line_start = point_at(0, 0);
    this->line_end = point_at(0, 0);
    this->moved_this_frame = false;
    this->current = false;
};

room::room(room_type::types type, drawing_options draw_opts, point_2d pos) : room(type, draw_opts)
{
    this->set_pos(pos);
}

void room::update(game *game_obj)
{
    this->moved_this_frame = false;
    this->hover = point_in_rectangle(game_obj->mouse_pos, this->hitbox);
    if (this->active && this->hover && game_obj->prev_mouse_left && !game_obj->cur_mouse_left)
    {
        game_obj->start_room(this);
    }
}

void room::render(game *game_obj)
{
    draw_bitmap(bitmap_named(this->hover ? this->hover_sprite : this->sprite), this->render_at.x, this->render_at.y, this->draw_opts);

    for (int i = 0; i < this->next.length(); i++)
    {
        draw_line(this->current ? palette::next_room_colour : this->hover ? game_obj->colours.hp_colour
                                                                          : game_obj->colours.line_colour,
                  this->line_start, this->next[i]->line_end, option_line_width(3));
    }
};

void room::set_pos(point_2d pos)
{
    this->hitbox = scale_rectangle(this->sprite, pos, this->draw_opts);
    this->render_at = scale_render_pos(this->sprite, pos, this->draw_opts);
    this->line_start = point_at(this->hitbox.x + this->hitbox.width, this->hitbox.y + this->hitbox.height / 2);
    this->line_end = point_at(this->hitbox.x, this->hitbox.y + this->hitbox.height / 2);
};

void room::move(int x, int y)
{
    if (!this->moved_this_frame)
    {
        this->moved_this_frame = true;

        this->hitbox.x += x;
        this->hitbox.y += y;

        this->render_at.x += x;
        this->render_at.y += y;

        this->line_start.x += x;
        this->line_start.y += y;

        this->line_end.x += x;
        this->line_end.y += y;
    }
}

json room::save()
{
    json save = create_json();
    json_set_number(save, room::schema.id, this->id);
    json_set_number(save, room::schema.type, (int)this->type);
    json_set_number(save, room::schema.scale_x, this->draw_opts.scale_x);
    json_set_number(save, room::schema.scale_y, this->draw_opts.scale_y);
    json_set_number(save, room::schema.x, this->hitbox.x);
    json_set_number(save, room::schema.y, this->hitbox.y);

    std::vector<double> next_ids;
    for (int i = 0; i < this->next.length(); i++)
    {
        next_ids.push_back(this->next[i]->id);
    }
    json_set_array(save, room::schema.next, next_ids);

    return save;
}
#pragma endregion

#pragma region game
const game_schema game::schema{};

game::game()
{
    this->full_screen_active = true;
    this->full_screen = start_screen::instance();
    this->upper_screen = nullptr;
    this->lower_screen = nullptr;
    this->overlay = nullptr;
    this->player_object = nullptr;
    this->enemies = dynamic_array<enemy *>();
    this->npcs = dynamic_array<npc *>();
    this->backstack = dynamic_array<backstack_entry>();
    this->ground_anchor = point_2d(40, 350);
    this->ground_gap = point_2d(55, 55);
    this->ground_opts = option_defaults();
    this->ground = fixed_array<item_stack *, game::GROUND_SIZE>();
    this->in_battle = false;
    this->turn = -1;
    this->current_tooltip = nullptr;
    this->tooltip_active = false;
    this->colours = palette::light();
    this->prev_mouse_left = false;
    this->prev_mouse_right = false;
    this->complete = true;
    this->layer = -1;
    this->update_mouse_vars();
    this->generate_map();
}

game *game::instance()
{
    static game *inst = new game();
    return inst;
}

void game::update_mouse_vars()
{
    this->prev_mouse_left = this->cur_mouse_left;
    this->cur_mouse_left = mouse_down(mouse_button::LEFT_BUTTON);
    this->prev_mouse_right = this->cur_mouse_right;
    this->cur_mouse_right = mouse_down(mouse_button::RIGHT_BUTTON);
    this->prev_mouse_pos = this->mouse_pos;
    this->mouse_pos = mouse_position();
}

void game::populate_ground()
{
    for (int i = 0; i < game::GROUND_SIZE; i++)
    {
        this->ground[i] = new item_stack(this->ground_opts, point_at(this->ground_anchor.x + this->ground_gap.x * (i % 10), this->ground_anchor.y + this->ground_gap.y * (i / 10)));
    }
}

void game::generate_map()
{
    int midline = 0;
    int x_start = 0;
    drawing_options opts = option_defaults();
    this->map = dynamic_array<room *>();
    room *starting_room = new room(room_type::BATTLE_LV1, opts, point_at(x_start, midline - room::ROOM_HEIGHT / 2));
    dynamic_array<dynamic_array<room *>> layers;
    this->current_room = starting_room;
    starting_room->current = true;
    this->map.add(starting_room);

    dynamic_array<room *> first;
    first.add(starting_room);
    layers.add(first);

    // Generate rooms
    for (int i = 0; i < game::MAP_LENGTH - 2; i++)
    {
        dynamic_array<room *> layer;
        int amount = rnd(1, game::MAX_MAP_HEIGHT);
        int bottom_y = midline - (amount - 1) / 2 * room::Y_GAP;
        for (int j = 0; j < amount; j++)
        {
            room *new_room = new room(room_type::random_type(), opts, point_at(x_start + (i + 1) * (room::X_GAP + room::ROOM_WIDTH), bottom_y + j * room::Y_GAP - room::ROOM_HEIGHT / 2));
            layer.add(new_room);
            this->map.add(new_room);
        }
        layers.add(layer);
    }

    dynamic_array<room *> last;
    last.add(new room(room_type::BOSS, opts, point_at(x_start + 10 * (room::X_GAP + room::ROOM_WIDTH), midline - room::ROOM_HEIGHT / 2)));
    layers.add(last);
    this->map.add(last[0]);

    // Link rooms
    for (int i = 0; i < layers.length() - 1; i++)
    {
        auto current = layers[i];
        auto next = layers[i + 1];

        dynamic_array<int> available_to_layer;
        for (int j = 0; j < next.length(); j++)
        {
            available_to_layer.add(j);
        }

        for (int j = 0; j < current.length(); j++)
        {
            room *current_node = current[j];
            dynamic_array<int> available_to_room;
            for (int k = 0; k < next.length(); k++)
            {
                available_to_room.add(k);
            }

            do
            {
                int connect_to = rnd(0, available_to_room.length() - 1);
                current_node->next.add(next[connect_to]);
                available_to_room.remove_at(connect_to);
                for (int k = 0; k < available_to_layer.length(); k++)
                {
                    if (available_to_layer[k] == connect_to)
                    {
                        available_to_layer.remove_at(k);
                    }
                }
            } while (rnd(1, 2) == 1 && available_to_room.length() > 0);
        }

        for (int j = 0; j < available_to_layer.length(); j++)
        {
            room *current_node = next[available_to_layer[j]];
            dynamic_array<int> available_to_room;
            for (int k = 0; k < current.length(); k++)
            {
                available_to_room.add(k);
            }

            do
            {
                int connect_to = rnd(0, available_to_room.length() - 1);
                current[available_to_room[connect_to]]->next.add(current_node);
                available_to_room.remove_at(connect_to);
            } while (rnd(1, 2) == 1 && available_to_room.length() > 0);
        }
    }
}

// Handle all changes to the game.
void game::update()
{
    this->update_mouse_vars();
    if (this->full_screen_active)
    {
        this->full_screen->update(this);
        this->full_screen->tick(this);
    }
    else
    {
        this->overlay->update(this);
        this->overlay->tick(this);
        this->upper_screen->update(this);
        this->upper_screen->tick(this);
        this->lower_screen->update(this);
        this->lower_screen->tick(this);
    }
}

void game::render()
{
    if (this->full_screen_active)
    {
        this->full_screen->render(this);
    }
    else
    {
        this->overlay->render(this);
        this->upper_screen->render(this);
        this->lower_screen->render(this);
    }
    draw_text("(" + to_string(this->mouse_pos.x, 0) + "," + to_string(this->mouse_pos.y, 0) + ")", this->colours.text, "font_bold", 24, 100, 0);
    if (this->tooltip_active)
    {
        this->current_tooltip->render(this);
    }
    this->tooltip_active = false;
}

void game::change_screen(screen *to)
{
    if (this->full_screen_active)
    {
        this->backstack.add(backstack_entry{true, this->full_screen, nullptr, nullptr, nullptr});
    }
    else
    {
        this->backstack.add(backstack_entry{false, nullptr, this->overlay, this->upper_screen, this->lower_screen});
    }

    if (to->type == screen_type::FULL)
    {
        if (!this->full_screen_active)
        {
        }
        this->full_screen_active = true;
        this->full_screen = to;
        this->in_battle = this->full_screen->is_battle_screen;
    }
    else
    {
        this->full_screen_active = false;
        if (to->type == screen_type::UPPER)
        {
            this->upper_screen = to;
            this->in_battle = this->upper_screen->is_battle_screen;
        }
        else if (to->type == screen_type::LOWER)
        {
            this->lower_screen = to;
        }
        else
        {
            this->overlay = to;
        }
    }
}

void game::change_screen(screen *overlay, screen *upper, screen *lower)
{
    if (this->full_screen_active)
    {
        this->backstack.add(backstack_entry{true, this->full_screen, nullptr, nullptr, nullptr});
    }
    else
    {
        this->backstack.add(backstack_entry{false, nullptr, this->overlay, this->upper_screen, this->lower_screen});
    }

    this->overlay = overlay;
    this->upper_screen = upper;
    this->in_battle = this->upper_screen->is_battle_screen;
    this->lower_screen = lower;
    this->full_screen_active = false;
}

void game::back()
{
    int last = this->backstack.length() - 1;
    if (last == -1)
    {
        return;
    }

    backstack_entry entry = this->backstack[last];
    this->backstack.remove(last);

    this->full_screen_active = entry.full;
    if (this->full_screen_active)
    {
        this->full_screen = entry.full;
        this->in_battle = this->full_screen->is_battle_screen;
    }
    else
    {
        this->overlay = entry.overlay;
        this->upper_screen = entry.upper;
        this->in_battle = this->upper_screen->is_battle_screen;
        this->lower_screen = entry.lower;
    }
}

void game::spawn_enemies()
{
    int target = 5 + 5 * this->layer;
    int current_x = 50;
    // this->enemies.add(new felknight(point_at(current_x, 140)));

    while (target > 0)
    {
        double value = rnd(); // There is a chance this falls out of range (since the distribution has an infinite tail), but then it just reruns. This is also unlikely to happen in earlier levels due to limited precision.
        for (const auto &[bucket_max, list] : enemy::buckets)
        {
            double prob = cndf(bucket_max, (this->layer + 0.5) * enemy::BUCKET_WIDTH, enemy::STDEV);
            if (value <= prob)
            {
                enemy *to_add = list[rnd(0, list.size() - 1)](point_at(current_x, 140));
                target -= to_add->value;
                current_x += to_add->hitbox.width;
                this->enemies.add(to_add);
                break;
            }
        }
    }
}

void game::remove_entity(entity *target)
{
    for (int i = 0; i < this->enemies.length(); i++)
    {
        if (this->enemies[i] == target)
        {
            delete this->enemies[i];
            this->enemies.remove_at(i);
            return;
        }
    }
}

void game::end_room()
{
    battle_upper::instance()->clear_tickers();
    this->change_screen(game_overlay::instance(), post_battle_upper::instance(), battle_lower::instance());

    for (int i = 0; i < this->map.length(); i++)
    {
        room *node = this->map[i];
        if (node->id == this->current_room->id)
        {
            for (int j = 0; j < node->next.length(); j++)
            {
                node->next[j]->active = true;
            }
            break;
        }
        else
        {
            node->active = false;
        }
    }

    int enemy_count = this->enemies.length();
    for (int i = 0; i < enemy_count; i++)
    {
        delete this->enemies[i];
    }
    this->enemies.clear();

    int npc_count = this->npcs.length();
    for (int i = 0; i < npc_count; i++)
    {
        delete this->npcs[i];
    }
    this->npcs.clear();
    this->turn = -1;
}

void game::progress_turn()
{
    if (this->turn < this->enemies.length() - 1)
    {
        turn += 1;
    }
    else
    {
        for (int i = 0; i < this->enemies.length(); i++)
        {
            this->enemies[i]->end_of_turn(this);
        }
        this->player_object->end_of_turn(this);
        this->turn = -1;
    }
}

void game::next_attack()
{
    if (this->turn != -1)
    {
        this->enemies[this->turn]->start_attack(this);
    }
}

void game::start_room(room *next)
{
    this->current_room->current = false;
    this->current_room = next;
    this->current_room->current = true;
    this->layer++;
    for (int i = 0; i < this->ground.length(); i++)
    {
        this->ground[i]->clear();
    }
    switch (this->current_room->type)
    {
    case room_type::BATTLE_LV1:
    case room_type::BATTLE_LV2:
    case room_type::BATTLE_LV3:
    case room_type::BATTLE_LV4:
        this->spawn_enemies();
        this->change_screen(game_overlay::instance(), battle_upper::instance(), battle_lower::instance());
        break;
    case room_type::MERCHANT:
        this->npcs.add(new merchant());
        this->change_screen(game_overlay::instance(), npc_upper::instance(), battle_lower::instance());
        break;
    case room_type::CHEST:
        this->npcs.add(new chest());
        this->change_screen(game_overlay::instance(), npc_upper::instance(), battle_lower::instance());
        break;
    case room_type::BOSS:
        break;
    }
}

void game::drop(item_stack *stack)
{
    for (int i = 0; i < game::GROUND_SIZE; i++)
    {
        if (this->ground[i]->add_stack(stack))
        {
            return;
        }
    }
    stack->burn();
}

void game::drop(item *thing, int count)
{
    item_stack *stack = new item_stack();
    stack->alter(thing, count);
    this->drop(stack);
    delete stack;
}

void game::add_ticker(ticker ticker_obj, screen_type target)
{
    switch (target)
    {
    case screen_type::FULL:
        this->full_screen->tickers.add(ticker_obj);
        break;
    case screen_type::UPPER:
        this->upper_screen->tickers.add(ticker_obj);
        break;
    case screen_type::LOWER:
        this->lower_screen->tickers.add(ticker_obj);
        break;
    case screen_type::OVERLAY:
        this->overlay->tickers.add(ticker_obj);
        break;
    }
}

void game::set_tooltip(tooltip *to)
{
    this->current_tooltip = to;
}

void game::render_tooltip()
{
    this->tooltip_active = true;
}

void game::save()
{
    json save = create_json();
    json_set_string(save, game::schema.palette, this->colours.name);
    json_set_bool(save, game::schema.complete, this->complete);

    std::vector<json> map_json;
    for (int i = 0; i < this->map.length(); i++)
    {
        map_json.push_back(this->map[i]->save());
    }
    json_set_array(save, game::schema.map, map_json);

    std::vector<json> ground_json;
    for (int i = 0; i < this->ground.length(); i++)
    {
        ground_json.push_back(this->ground[i]->save());
    }
    json_set_array(save, game::schema.ground, ground_json);

    json_set_number(save, game::schema.current_room, this->current_room->id);
    json_set_object(save, game::schema.player, this->player_object->save());
    json_to_file(save, "save.json");
    free_json(save);
}

void game::load()
{
    json save = json_from_file("save.json");

    if (json_has_key(save, game::schema.palette) && json_read_string(save, game::schema.palette) == "dark")
    {
        this->colours = palette::dark();
    }

    if (json_has_key(save, game::schema.complete))
    {
        this->complete = json_read_bool(save, game::schema.complete);
    }

    if (this->complete)
        return;

    if (json_has_key(save, game::schema.map))
    {
        for (int i = 0; i < this->map.length(); i++)
        {
            delete this->map[i];
        }
        this->map.clear();

        std::vector<json> map_json;
        json_read_array(save, game::schema.map, map_json);
        room::load(this, map_json[0], map_json);
    }

    if (json_has_key(save, game::schema.ground))
    {
        std::vector<json> ground_json;
        json_read_array(save, game::schema.ground, ground_json);
        for (size_t i = 0; i < ground_json.size(); i++)
        {
            this->ground[i]->load(ground_json[i]);
        }
    }

    if (json_has_key(save, game::schema.current_room))
    {
        int current_id = json_read_number_as_int(save, game::schema.current_room);
        for (int i = 0; i < this->map.length(); i++)
        {
            if (this->map[i]->id == current_id)
            {
                this->current_room = this->map[i];
            }
        }
    }
    else
    {
        this->current_room = this->map[0];
    }

    if (json_has_key(save, game::schema.player))
    {
        this->player_object->load(json_read_object(save, game::schema.player));
    }
    free_json(save);
}

void game::clear()
{
    delete this->player_object;

    for (int i = 0; i < this->map.length(); i++)
    {
        delete this->map[i];
    }
    this->map.clear();

    int enemy_count = this->enemies.length();
    for (int i = 0; i < enemy_count; i++)
    {
        delete this->enemies[i];
    }
    this->enemies.clear();

    int npc_count = this->npcs.length();
    for (int i = 0; i < npc_count; i++)
    {
        delete this->npcs[i];
    }
    this->npcs.clear();

    for (int i = 0; i < game::GROUND_SIZE; i++)
    {
        delete this->ground[i];
    }
}

void game::reset()
{
    this->layer = -1;
    this->player_object = new player();
    this->populate_ground();
    this->generate_map();
}

game::~game()
{
    this->save();
    this->clear();
}
#pragma endregion

/*
TODO:
- Change first room back to battle lv1 instead of merchant
- Docstring everything.
- Clean unused functions.
- Value-based enemy spawning system.
- More enemies and items.
- More drops from enemies (accessories, consumables, etc.)
- Line boiling.
- Saving battle state.
- Optimise map to render to bitmap and render relevant part of bitmap.
- Remove save scumming.
- Parametrise enemy drops.
- Make expanded tooltips more informative.
*/