#include "items.h"
#include <map>
#include <cstdarg>

attrmap default_attrmap()
{
    attrmap m;
    for (int i = 0; i != (int)attribute::DUMMY_LAST; i++)
    {
        m.emplace((attribute)i, 0);
    }
    return m;
}

attrmap make_attrmap(attrmap to_start)
{
    for (int i = 0; i != (int)attribute::DUMMY_LAST; i++)
    {
        if (!to_start.contains((attribute)i))
        {
            to_start.emplace((attribute)i, 0);
        }
    }
    return to_start;
}

#pragma region item
std::map<int, item *> item::registry = std::map<int, item *>();

item::item(string name, int max, item_type type, string sprite, string desc, attrmap attributes, attrmap modifiers)
{
    this->name = name;
    this->max = max;
    this->sprite = sprite;
    this->type = type;
    this->desc = desc;
    this->attributes = attributes;
    this->modifiers = modifiers;
    this->id = item::counter();
}

void item::init()
{
    item::registry.clear();
    item::registrate(item::instance(), coin, dagger, basic_staff, pike, shovel, nightwatch_lantern::instance(), shotgun, felknight_greatsword, glorngus_claymore::instance(), felknight_stechhelm, felknight_chestplate, felknight_glove, nullptr);
}

item *item::instance()
{
    static item *inst = new item("Orb", 100, item_type::COLLECTIBLE, "orb_sprite", "A ball.", attrmap());
    return inst;
}

int item::counter()
{
    static int counter = 0;
    int old = counter;
    counter++;
    return old;
}

void item::registrate(item *instance, ...)
{
    std::va_list arguments;
    va_start(arguments, instance);
    while (instance != nullptr)
    {
        item::registry.emplace(instance->id, instance);
        instance = va_arg(arguments, item *);
    }
    va_end(arguments);
}

item *item::retrieve(int id)
{
    if (item::registry.contains(id))
    {
        return item::registry[id];
    }
    else
    {
        return item::instance();
    }
}

bool item::attack(game *game_obj, entity *user, entity *target, int damage)
{
    return target->take_damage(game_obj, user, damage, this->type == item_type::MELEE ? attribute::MELEE_DEF : attribute::RANGED_DEF).first;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
void item::consume(entity *owner)
#pragma clang diagnostic pop
{
    write_line("This item cannot be consumed.");
}

item::~item()
{
}
#pragma endregion

#pragma region item stack
item_stack_schema item_stack::schema{};

void item_stack::regenerate()
{
    if (this->contains != nullptr)
    {
        this->hover->regenerate();
        this->expanded->regenerate();
    }
}

item_stack::item_stack() : item_stack(nullptr, 0, option_defaults(), point_at(0, 0))
{
}

item_stack::item_stack(drawing_options draw_opts, point_2d loc) : item_stack(nullptr, 0, draw_opts, loc)
{
}

item_stack::item_stack(item *contains, int count, drawing_options draw_opts, point_2d loc)
{
    this->contains = contains;
    this->count = count;
    this->draw_opts = draw_opts;
    this->loc = loc;
    this->hitbox = rectangle_from(this->loc, 50 * this->draw_opts.scale_x, 50 * this->draw_opts.scale_y);
    std::function<color(void)> text_colour = []()
    { return game::instance()->colours.text; };
    this->hover = (new tooltip())
                      ->add([this]()
                            { return this->contains == nullptr ? "" : this->contains->name; }, 18, text_colour)
                      ->add([this]()
                            { return this->contains == nullptr ? "" : (" x" + to_string(this->count)); }, 18, text_colour)
                      ->newline()
                      ->add("Shift for more info", 12, text_colour)
                      ->newline()
                      ->generate();
    this->expanded = (new tooltip())
                         ->add([this]()
                               { return this->contains == nullptr ? "" : this->contains->name; }, 18, text_colour)
                         ->add([this]()
                               { return this->contains == nullptr ? "" : (" x" + to_string(this->count)); }, 18, text_colour)
                         ->newline()
                         ->blank_line()
                         ->add([this]()
                               { return this->contains == nullptr ? "" : this->contains->desc; }, 15, text_colour)
                         ->newline()
                         ->generate();
}

item_update_result item_stack::update(game *game_obj)
{
    if (this->contains != nullptr && point_in_rectangle(game_obj->mouse_pos, this->hitbox))
    {
        game_obj->set_tooltip(key_down(LEFT_SHIFT_KEY) ? this->expanded : this->hover);
        game_obj->render_tooltip();
        if (game_obj->cur_mouse_left && !game_obj->prev_mouse_left)
        {
            return item_update_result::LCLICK;
        }
        else if (game_obj->cur_mouse_right && !game_obj->prev_mouse_right)
        {
            return key_down(LEFT_SHIFT_KEY) ? item_update_result::SRCLICK : item_update_result::RCLICK;
        }
    }
    return item_update_result::NONE;
}

void item_stack::render(game *game_obj)
{
    if (this->contains != nullptr)
    {
        draw_bitmap(this->contains->sprite, this->loc.x, this->loc.y, this->draw_opts);
        if (this->count > 1)
        {
            draw_text(to_string(this->count), game_obj->colours.text, "font_bold", 15, this->loc.x + 10, this->loc.y + 30, this->draw_opts);
        }
    }
}

item *item_stack::get_contains()
{
    return this->contains;
}

int item_stack::get_count()
{
    return this->count;
}

void item_stack::alter(item *contains, int count)
{
    this->contains = contains;
    this->count = count;
    this->regenerate();
}

bool item_stack::increment(int by)
{
    if ((this->count + by > this->contains->max) || (this->count + by < 0))
    {
        return false;
    }
    this->count += by;
    if (this->count <= 0)
    {
        this->count = 0;
        this->contains = nullptr;
    }
    this->regenerate();
    return true;
}

bool item_stack::add_stack(item_stack *stack)
{
    if (this->contains == nullptr)
    {
        stack->relocate_item(this);
        return true;
    }
    if (stack->contains == this->contains && this->count < this->contains->max)
    {
        if (this->increment(stack->count))
        {
            stack->clear();
            return true;
        }
        else
        {
            int old = this->count;
            this->increment(this->contains->max - this->count);
            stack->increment(old - this->contains->max);
            if (stack->count == 0)
            {
                stack->clear();
                return true;
            }
            return false;
        }
    }
    this->regenerate();
    return false;
}

void item_stack::relocate_item(item_stack *to)
{
    to->alter(this->contains, this->count);
    this->clear();
}

void item_stack::swap(item_stack *with)
{
    item *temp_contains = with->contains;
    int temp_count = with->count;
    this->relocate_item(with);
    this->contains = temp_contains;
    this->count = temp_count;
    this->regenerate();
}

void item_stack::clear()
{
    this->contains = nullptr;
    this->count = 0;
}

void item_stack::burn()
{
    this->clear();
}

bool item_stack::is_empty()
{
    return (this->contains == nullptr) || (this->count <= 0);
}

json item_stack::save()
{
    json save = create_json();
    json_set_number(save, item_stack::schema.contains, this->contains == nullptr ? -1 : this->contains->id);
    json_set_number(save, item_stack::schema.count, this->count);
    json_set_number(save, item_stack::schema.scale_x, this->draw_opts.scale_x);
    json_set_number(save, item_stack::schema.scale_y, this->draw_opts.scale_y);
    json_set_number(save, item_stack::schema.x, this->loc.x);
    json_set_number(save, item_stack::schema.y, this->loc.y);
    return save;
}

void item_stack::load(json from)
{
    int contains_id = json_read_number_as_int(from, item_stack::schema.contains);
    this->contains = contains_id == -1 ? nullptr : item::registry[contains_id];
    this->count = json_read_number_as_int(from, item_stack::schema.count);
    this->draw_opts = option_scale_bmp(json_read_number_as_double(from, item_stack::schema.scale_x), json_read_number_as_double(from, item_stack::schema.scale_y));
    this->loc = point_at(json_read_number_as_int(from, item_stack::schema.x), json_read_number_as_int(from, item_stack::schema.y));
    this->hitbox = rectangle_from(this->loc, 50 * this->draw_opts.scale_x, 50 * this->draw_opts.scale_y);
    this->regenerate();
}

item_stack::~item_stack()
{
    delete this->hover;
    delete this->expanded;
}
#pragma endregion

#pragma region misc
item *coin = new item("Coin", 9999, item_type::COLLECTIBLE, "coin", "Quintessential human obssession.");

item *dagger = new item("Dagger", 1, item_type::MELEE, "dagger", "Pointy pokey.", make_attrmap({{attribute::MELEE_DMG, 5}}));

item *basic_staff = new item("Basic Staff", 1, item_type::STAFF, "basic_staff", "Magic missile!", make_attrmap({{attribute::RANGED_DMG, 8}}), make_attrmap({{attribute::MANA_COST, 5}}));

item *pike = new item("Pike", 1, item_type::MELEE, "pike", "Long stick.", make_attrmap({{attribute::MELEE_DMG, 7}}), make_attrmap({{attribute::MELEE_DEF, 20}}));

item *glorngus_fists = new item("Glorngus Fists", 1, item_type::MELEE, "blank", "INTERNAL USE ONLY", make_attrmap({{attribute::MELEE_DMG, 3}}));

item *shovel = new item("Shovel", 1, item_type::MELEE, "shovel_0", "Good for getting into holes.", make_attrmap({{attribute::MELEE_DMG, 10}}));

nightwatch_lantern::nightwatch_lantern() : item("Nightwatch Lantern", 1, item_type::STAFF, "nightwatch_lantern_0", "Illuminating.", make_attrmap({{attribute::RANGED_DMG, 1}}), make_attrmap({{attribute::MANA_COST, 3}}))
{
}

nightwatch_lantern *nightwatch_lantern::instance()
{
    static nightwatch_lantern *inst = new nightwatch_lantern();
    return inst;
}

bool nightwatch_lantern::attack(game *game_obj, entity *user, entity *target, int damage)
{
    target->statuses.emplace(status::BLINDED, 2);
    return item::attack(game_obj, user, target, damage);
}

item *shotgun = new item("Shotgun", 1, item_type::STAFF, "shotgun_0", "Great for hunting intruders.", make_attrmap({{attribute::RANGED_DMG, 10}}), make_attrmap({{attribute::MANA_COST, 5}}));

item *amalgam_strike = new item("Amalgam Strike", 1, item_type::MELEE, "blank", "INTERNAL USE ONLY", make_attrmap({{attribute::MELEE_DMG, 20}}));

item *glorngus_fists_evolved = new item("Glorngus Fists Evolved", 1, item_type::MELEE, "blank", "INTERNAL USE ONLY", make_attrmap({{attribute::MELEE_DMG, 9}}));

item *felknight_greatsword = new item("Felknight's Greatsword", 1, item_type::MELEE, "felknight_greatsword_0", "Something.", make_attrmap({{attribute::MELEE_DMG, 15}}));

glorngus_claymore::glorngus_claymore() : item("Glorngus Claymore", 1, item_type::MELEE, "glorngus_greatsword_0", "The Glorngi are back with a vengeance.", make_attrmap({{attribute::MELEE_DMG, 25}}))
{
}

glorngus_claymore *glorngus_claymore::instance()
{
    static glorngus_claymore *inst = new glorngus_claymore();
    return inst;
}

bool glorngus_claymore::attack(game *game_obj, entity *user, entity *target, int damage)
{
    if (!user->statuses.contains(status::GLORNGUS_GREATSWORD_CHARGE))
    {
        user->statuses.emplace(status::GLORNGUS_GREATSWORD_CHARGE, 2);
        return false;
    }
    else
    {
        return item::attack(game_obj, user, target, damage);
    }
}

item *felknight_stechhelm = new item("Felknight's Stechhelm", 1, item_type::ACCESSORY, "felknight_stechhelm_0", "The Felknight's impenetrable helmet.", default_attrmap(), make_attrmap({{attribute::MELEE_DEF, 20}}));

item *felknight_chestplate = new item("Felknight's Chestplate", 1, item_type::ACCESSORY, "felknight_chestplate_0", "The Felknight's trusty chestplate.", default_attrmap(), make_attrmap({{attribute::MELEE_DEF, 30}, {attribute::BOIL_DEFENSE, -10}}));

item *felknight_glove = new item("Felknight's Glove", 1, item_type::ACCESSORY, "felknight_glove_0", "The Felknight's unscathed glove.", default_attrmap(), make_attrmap({{attribute::MELEE_DEF, 5}, {attribute::MELEE_DMG, 3}}));
#pragma endregion