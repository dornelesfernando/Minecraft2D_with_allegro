#ifndef GAME_H
    #define GAME_H

    #include <allegro5/allegro.h>
    #include <stdbool.h> // Para variáveis do tipo 'bool'

    // Variáveis globais do Allegro
    extern ALLEGRO_DISPLAY *display;
    extern ALLEGRO_EVENT_QUEUE *event_queue;
    extern ALLEGRO_TIMER *timer;
    extern bool redraw;

    // Funções
    bool game_init();
    void game_loop();
    void game_shutdown();
#endif
