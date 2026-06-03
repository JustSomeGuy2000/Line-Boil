#include "splashkit.h"
#include "game.h"

int main()
{
    open_window("Line Boil", WIN_WIDTH, WIN_HEIGHT);
    load_resource_bundle("resources", "resources.txt");
    item::init();
    enemy::init();
    game *game_object = game::instance();
    game_object->reset();
    game_object->load();

    while (!quit_requested())
    {
        process_events();
        clear_screen(game_object->colours.bg);
        game_object->update();
        game_object->render();
        refresh_screen(60);
    }

    free_resource_bundle("resources");
    close_all_windows();
    delete game::instance();
    return 0;
}

/*
TODO:
1. Add settings button to pause menu
2. Add chroma keying to allow palettes to change graphics colour. Consider a highly compressed encoded format to reduce the overhead of iterating through and replacing pixels one by one. Having a copy of each asset for each colour is unaccpetably inelegant.
*/