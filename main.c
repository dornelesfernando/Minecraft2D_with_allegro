// --- BIBLIOTECAS UTILIZADAS ---
#include <stdio.h>
#include <stdbool.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <math.h>
#include <time.h>

// --- VARIÁVEIS DE CONFIGURAÇÃO ---
#define FPS 90.0f
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 576
#define BLOCO_TAMANHO 32

// --- CONSTANTES DE FÍSICA ---
#define GRAVITY 0.5f
#define JUMP_STRENGTH -6.0f
#define PLAYER_SPEED 3.0f

// --- MATRIZ MUNDO ---
#define WORLD_COLS (SCREEN_WIDTH / BLOCO_TAMANHO)
#define WORLD_ROWS (SCREEN_HEIGHT / BLOCO_TAMANHO)

// --- CONFIGURAÇÕES ---
ALLEGRO_DISPLAY *display = NULL;
ALLEGRO_EVENT_QUEUE *event_queue = NULL;
ALLEGRO_TIMER *timer = NULL;

// --- SPRITES ---
ALLEGRO_BITMAP *spr_grass = NULL;
ALLEGRO_BITMAP *spr_dirt = NULL;
ALLEGRO_BITMAP *spr_stone = NULL;

// --- CONTROLE DE ESTADO DO TECLADO ---
bool key_pressed[ALLEGRO_KEY_MAX];

// --- CONTROLE DE POSIÇÃO DO MOUSE  ---
float mouse_x = 0;
float mouse_y = 0;

// --- VARIÁVEIS GLOBAIS ---
bool redraw = false;
bool is_fullscreen = false;
int world_map[WORLD_COLS][WORLD_ROWS]; // Matriz 2D para armazenar o mundo
int windowed_width = SCREEN_WIDTH;
int windowed_height = SCREEN_HEIGHT;

// --- CONSTANTES PARA  BOTÃO DE TELA CHEIA ---
#define FULLSCREEN_BUTTON_X (SCREEN_WIDTH - 120)
#define FULLSCREEN_BUTTON_Y 10
#define FULLSCREEN_BUTTON_WIDTH 100
#define FULLSCREEN_BUTTON_HEIGHT 30

// --------------------------------------------------------------------------------- //

// --- PROTÓTIPOS DE FUNÇÕES ---
ALLEGRO_BITMAP* load_and_scale_bitmap(const char* filename, int width, int height);
int choose_random(const int arr[], int size);
bool config();
void destroyBitmapSprites();
void draw_player();
void draw_world_from_map();
bool init_player();
void register_events();
bool renderSprites();
void reload_world();
void toggle_fullscreen();
void world_generation();
// --------------------------------------------------------------------------------- //

// --- STRUCT DO JOGADOR
typedef struct Player {
    float x;
    float y;
    float hand_x;
    float hand_y;
    float speed_x; // Velocidade em X
    float speed_y; // Velocidade em Y
    int width;
    int height;
    ALLEGRO_BITMAP *sprite; // sprite do corpo (spr_steve)
    ALLEGRO_BITMAP *hand_sprite; // sprite da mão (spr_steve_hand)
    bool on_ground;
    int facing_direction;
} Player;

Player player; // instância global

// --------------------------------------------------------------------------------- //

ALLEGRO_BITMAP* load_and_scale_bitmap(const char* filename, int width, int height) {
    ALLEGRO_BITMAP *loaded_bitmap = al_load_bitmap(filename);
    if (!loaded_bitmap) {
        fprintf(stderr, "Falha ao carregar o bitmap: %s\n", filename);
        return NULL;
    }

    ALLEGRO_BITMAP *scaled_bitmap = al_create_bitmap(width, height);
    if (!scaled_bitmap) {
        fprintf(stderr, "Falha ao criar o bitmap escalado para: %s\n", filename);
        al_destroy_bitmap(loaded_bitmap);
        return NULL;
    }

    ALLEGRO_BITMAP *original_target = al_get_target_bitmap();
    al_set_target_bitmap(scaled_bitmap);

    al_draw_scaled_bitmap(loaded_bitmap,
                         0, 0,                               // Posição de origem no bitmap carregado
                         al_get_bitmap_width(loaded_bitmap), // Largura do bitmap carregado
                         al_get_bitmap_height(loaded_bitmap),// Altura do bitmap carregado
                         0, 0,                               // Posição de destino no novo bitmap
                         width, height,                      // Largura e altura de destino (BLOCO_TAMANHO)
                         0);                                 // Flags

    al_set_target_bitmap(original_target);

    al_destroy_bitmap(loaded_bitmap);
    return scaled_bitmap;
}

int choose_random(const int arr[], int size) {
    int index = rand() % size;
    return arr[index];
}

bool config () {
    if(!al_init()) {
        fprintf(stderr, "Falha ao inicializar o Allegro!\n");
        return false;
    }

    timer = al_create_timer(1.0 / FPS);
    if(!timer) {
        fprintf(stderr, "Falha ao criar o timer!\n");
        return false;
    }

    redraw = true;

    al_set_new_display_flags(ALLEGRO_OPENGL | ALLEGRO_WINDOWED);
    display = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);
    if(!display) {
        fprintf(stderr, "Falha ao criar o display!\n");
        al_destroy_timer(timer);
        return false;
    }

    if(!al_install_keyboard()) {
        fprintf(stderr, "Falha ao inicializar o keyboard!\n");
        al_destroy_display(display);
        al_destroy_timer(timer);
        return false;
    }

    if(!al_install_mouse()) {
        fprintf(stderr, "Falha ao inicializar o mouse!\n");
        al_destroy_display(display);
        al_destroy_timer(timer);
        al_uninstall_keyboard();
        return false;
    }

    if(!al_init_image_addon()) {
        fprintf(stderr, "Falha ao inicializar o image addon!\n");
        al_destroy_display(display);
        al_destroy_timer(timer);
        al_uninstall_keyboard();
        al_uninstall_mouse();
        return false;
    }

    if (!renderSprites()) {
        // renderSprites já lida com a liberação de recursos em caso de falha
        return false;
    }

    al_set_target_bitmap(al_get_backbuffer(display));

    event_queue = al_create_event_queue();
    if(!event_queue) {
        fprintf(stderr, "Falha ao criar o event_queue!\n");
        al_destroy_bitmap(spr_grass);
        al_destroy_bitmap(spr_dirt);
        al_destroy_bitmap(spr_stone);
        al_destroy_bitmap(player.sprite);
        al_destroy_bitmap(player.hand_sprite);
        al_destroy_display(display);
        al_destroy_timer(timer);
        al_uninstall_keyboard();
        al_uninstall_mouse();
        al_shutdown_image_addon();
        return false;
    }

    return true;
}

void destroyBitmapSprites() {
    if (spr_grass) al_destroy_bitmap(spr_grass);
    if (spr_dirt) al_destroy_bitmap(spr_dirt);
    if (spr_stone) al_destroy_bitmap(spr_stone);
    if (player.sprite) al_destroy_bitmap(player.sprite);
    if (player.hand_sprite) al_destroy_bitmap(player.hand_sprite);

    al_destroy_display(display);
    al_destroy_timer(timer);
    al_uninstall_keyboard();
    al_uninstall_mouse();
    al_shutdown_image_addon();
}

void draw_player() {
    int flip_flags = 0;
    if (player.facing_direction == -1) { 
        flip_flags = ALLEGRO_FLIP_HORIZONTAL;
    }

    // --- DESENHA O JOGADOR ---
    al_draw_bitmap(player.sprite, player.x, player.y, flip_flags);

    // --- DESENHA A MÃO DO JOGADOR ---
    float pivot_x = player.x + 12;
    float pivot_y = player.y + 20;

    float sprite_pivot_offset_x = 0;
    float sprite_pivot_offset_y = 0;


    float angle = atan2f(mouse_y - pivot_y, mouse_x - pivot_x);

    int hand_flip_flags = flip_flags;

    if (player.facing_direction == -1 && mouse_x > pivot_x) {
        hand_flip_flags |= ALLEGRO_FLIP_HORIZONTAL; 
    } else if (player.facing_direction == 1 && mouse_x < pivot_x) {
        hand_flip_flags |= ALLEGRO_FLIP_HORIZONTAL;
    }

    al_draw_rotated_bitmap(player.hand_sprite,
                           sprite_pivot_offset_x,
                           sprite_pivot_offset_y,
                           pivot_x,              
                           pivot_y,
                           angle,                 
                           hand_flip_flags);    
}

void draw_world_from_map() {
    int i;
    for (i = 0; i < WORLD_COLS; i++) {
        int j;
        for (j = 0; j < WORLD_ROWS; j++) {
            ALLEGRO_BITMAP *block_sprite = NULL;
            switch (world_map[i][j]) {
                case 1: block_sprite = spr_grass; break;
                case 2: block_sprite = spr_dirt; break;
                case 3: block_sprite = spr_stone; break;
            }
            if (block_sprite) {
                al_draw_bitmap(block_sprite, (float)i * BLOCO_TAMANHO, (float)j * BLOCO_TAMANHO, 0);
            }
        }
    }
}

bool init_player() {
    player.width = BLOCO_TAMANHO - 4;
    player.height = (BLOCO_TAMANHO * 2) - 4;
    player.x = SCREEN_WIDTH / 2.0 - player.width / 2.0;
    player.y = 0;
    player.speed_x = 0;
    player.speed_y = 0;
    player.on_ground = false;
    player.facing_direction = 1;

    player.sprite = load_and_scale_bitmap("sprites/spr_steve.png", player.width, player.height);
    if (!player.sprite) {
        fprintf(stderr, "falha ao criar player.sprite!\n");
        destroyBitmapSprites();
        return false;
    }

    player.hand_sprite = load_and_scale_bitmap("sprites/spr_steve_hand.png", 14, 22);
    if (!player.hand_sprite) {
        fprintf(stderr, "falha ao criar player.hand_sprite!\n");
        destroyBitmapSprites();
        return false;
    }

    return true;
}

void register_events() {
    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());
}

bool renderSprites() {
    spr_grass = load_and_scale_bitmap("sprites/spr_grass.png", BLOCO_TAMANHO, BLOCO_TAMANHO);
    spr_dirt  = load_and_scale_bitmap("sprites/spr_dirt.png", BLOCO_TAMANHO, BLOCO_TAMANHO);
    spr_stone = load_and_scale_bitmap("sprites/spr_stone.png", BLOCO_TAMANHO, BLOCO_TAMANHO);

    // grass
    if(!spr_grass) {
        fprintf(stderr, "falha ao criar spr_grass!\n");
        destroyBitmapSprites();
        return false;
    }

    // dirt
    if(!spr_dirt) {
        fprintf(stderr, "falha ao criar spr_dirt!\n");
        destroyBitmapSprites();
        return false;
    }

    // stone
    if(!spr_stone) {
        fprintf(stderr, "falha ao criar spr_stone!\n");
        destroyBitmapSprites();
        return false;
    }
    
    return true;
}

void reload_world() {
    world_generation();
    redraw = true;
}

void toggle_fullscreen() {
    if (is_fullscreen) {
        if (!al_set_display_flag(display, ALLEGRO_FULLSCREEN_WINDOW, false)) {
            fprintf(stderr, "Falha ao desativar a flag ALLEGRO_FULLSCREEN_WINDOW.\n");
            return; 
        }

        if (!al_resize_display(display, windowed_width, windowed_height)) {
            fprintf(stderr, "Falha ao redimensionar display para modo janela!\n");
        }

        is_fullscreen = false;
        printf("Modo tela cheia: DESATIVADO \n");

    } else {
        windowed_width = al_get_display_width(display);
        windowed_height = al_get_display_height(display);

        if (al_set_display_flag(display, ALLEGRO_FULLSCREEN_WINDOW, true)) {
            is_fullscreen = true;
            printf("Modo tela cheia: ATIVADO\n");
        } else {
            fprintf(stderr, "Falha ao ativar modo tela cheia (ALLEGRO_FULLSCREEN_WINDOW)!\n");
        }
    }
}

void update_player_physics() {
    player.speed_y += GRAVITY;

    // --- MOVIMENTO HORIZONTAL ---
    player.speed_x = 0;

    // A
    if (key_pressed[ALLEGRO_KEY_A]) {
        player.speed_x = -PLAYER_SPEED;
        player.facing_direction = -1;
    }

    // D
    if (key_pressed[ALLEGRO_KEY_D]) {
        player.speed_x = PLAYER_SPEED;
        player.facing_direction = 1;
    }
    
    // A && D
    if (key_pressed[ALLEGRO_KEY_A] && key_pressed[ALLEGRO_KEY_D]) {
        player.speed_x = 0;
    }
    
    // --- PULO (SPACE || W) ---
    if (key_pressed[ALLEGRO_KEY_SPACE] && player.on_ground || key_pressed[ALLEGRO_KEY_W] && player.on_ground) {
        player.speed_y = JUMP_STRENGTH;
        player.on_ground = false;
        key_pressed[ALLEGRO_KEY_SPACE] = false;
        key_pressed[ALLEGRO_KEY_W] = false;
    }

    // --- Movimento Tentativo e Detecção de Colisão ---
    float next_x = player.x + player.speed_x;
    float next_y = player.y + player.speed_y;

    // --- Colisão Horizontal (eixo X) ---
    int player_left_tile_next_x = (int)(next_x) / BLOCO_TAMANHO;
    int player_right_tile_next_x = (int)(next_x + player.width - 1) / BLOCO_TAMANHO;
    int player_top_tile_y = (int)(player.y) / BLOCO_TAMANHO;
    int player_bottom_tile_y = (int)(player.y + player.height - 1) / BLOCO_TAMANHO;

    bool collided_horizontally = false;

    int r;
    for (r = player_top_tile_y; r <= player_bottom_tile_y; r++) {

        if (r < 0 || r >= WORLD_ROWS) continue;

        if (player.speed_x > 0) { // Movendo para a direita
            if (player_right_tile_next_x >= 0 && player_right_tile_next_x < WORLD_COLS && world_map[player_right_tile_next_x][r] != 0) {
                collided_horizontally = true;
                player.x = player_right_tile_next_x * BLOCO_TAMANHO - player.width;
                player.speed_x = 0;
                break; 
            }
        } else if (player.speed_x < 0) { // Movendo para a esquerda
            if (player_left_tile_next_x >= 0 && player_left_tile_next_x < WORLD_COLS && world_map[player_left_tile_next_x][r] != 0) {
                collided_horizontally = true;
                player.x = (player_left_tile_next_x + 1) * BLOCO_TAMANHO;
                player.speed_x = 0;
                break;
            }
        }
    }

    // COMMIT
    player.x += player.speed_x;

    // --- COLISÃO VERTICAL ---
    player.on_ground = false; 

    int player_left_tile_x = (int)(player.x) / BLOCO_TAMANHO;
    int player_right_tile_x = (int)(player.x + player.width - 1) / BLOCO_TAMANHO;
    int player_top_tile_next_y = (int)(next_y) / BLOCO_TAMANHO;
    int player_bottom_tile_next_y = (int)(next_y + player.height - 1) / BLOCO_TAMANHO;

    bool collided_vertically = false;

    int c;
    for (c = player_left_tile_x; c <= player_right_tile_x; c++) {
        if (c < 0 || c >= WORLD_COLS) continue;

        if (player.speed_y >= 0) {
            if (player_bottom_tile_next_y >= 0 && player_bottom_tile_next_y < WORLD_ROWS && world_map[c][player_bottom_tile_next_y] != 0) {
                collided_vertically = true;
                player.y = player_bottom_tile_next_y * BLOCO_TAMANHO - player.height;
                player.speed_y = 0;
                player.on_ground = true;
                break;
            }
        } else { // PULO
            if (player_top_tile_next_y >= 0 && player_top_tile_next_y < WORLD_ROWS && world_map[c][player_top_tile_next_y] != 0) {
                collided_vertically = true;
                player.y = (player_top_tile_next_y + 1) * BLOCO_TAMANHO;
                player.speed_y = 0;
                break;
            }
        }
    }
    
    player.y += player.speed_y;

    if (player.x < 0) player.x = 0;
    if (player.x + player.width > SCREEN_WIDTH) player.x = SCREEN_WIDTH - player.width;
    
    if (player.y + player.height > SCREEN_HEIGHT) {
        player.y = SCREEN_HEIGHT - player.height;
        player.speed_y = 0;
        player.on_ground = true;
    }
    if (player.y < 0) { 
        player.y = 0;
        player.speed_y = 0;
    }
}

void world_generation() {
    // 'heights_blocks' retrata a altura inicial dos blocos na geração dos mundos
    // esse valor é escolhido aleatóriamente a partir de um array de 5 números pré-definidos
    // pela função 'choose_random()'. Após, como é um objeto dinâmico que fica alteradndo
    // a partir do tempo do sistema, essa variável é salva para não sofrer alterações
    // e é utilizada para a criação dos blocos do mundo.
   
    // INDICE DE BLOCOS:
    // 1 = spr_grass
    // 2 = spr_dirt
    // 3 = spr_stone

    // Limpa o array mundo
    int i;
    for (i = 0; i < WORLD_COLS; i++) {
        int j;
        for (j = 0; j < WORLD_ROWS; j++) {
            world_map[i][j] = 0; // 0 representa um bloco vazio
        }
    }

    const int heights_block[] = {176 + 32, 208 + 32, 240 + 32, 272 + 32, 304 + 32};
    const int height_block = sizeof(heights_block) / sizeof(heights_block[0]);
    int starting_height_block = choose_random(heights_block, height_block);
    int alternative_height_block = starting_height_block;

    int dirt_level;
    int stone_level;
    int alternative_dirt_level = alternative_height_block; // Inicializado para evitar lixo

    int x;
    for(x = 0; x < SCREEN_WIDTH; x += BLOCO_TAMANHO) {
        int current_col = x / BLOCO_TAMANHO;
        if (current_col >= WORLD_COLS) continue;

        int arr_aux[] = {2, 3, 3, 3, 4, 4};
        int arr_aux_length = sizeof(arr_aux) / sizeof(arr_aux[0]);
        dirt_level = alternative_height_block + BLOCO_TAMANHO * choose_random(arr_aux, arr_aux_length);

        stone_level = SCREEN_HEIGHT;

        // al_draw_bitmap(spr_grass, x, alternative_height_block, 0);
        int grass_level = alternative_height_block / BLOCO_TAMANHO;
        if (grass_level >= 0 && grass_level < WORLD_ROWS) {
            world_map[current_col][grass_level] = 1;
        }

        int y;
        for(y = alternative_height_block + BLOCO_TAMANHO; y < dirt_level; y += BLOCO_TAMANHO) {
            int dirt_level = y / BLOCO_TAMANHO;
            if (dirt_level >= 0 && dirt_level < WORLD_ROWS) {
                world_map[current_col][dirt_level] = 2;
            }
            // al_draw_bitmap(spr_dirt, x, y, 0);
            alternative_dirt_level = y;
        }

        for(y = alternative_dirt_level + BLOCO_TAMANHO; y < stone_level; y += BLOCO_TAMANHO) {
            int stone_level = y / BLOCO_TAMANHO;
            if (stone_level >= 0 && stone_level < WORLD_ROWS) {
                world_map[current_col][stone_level] = 3;
            }
            //al_draw_bitmap(spr_stone, x, y, 0);
        }

        const int arr_aux2[] = {0, 0, 0, 0, 1, 1, 2};
        const int arr_aux2_length = sizeof(arr_aux2) / sizeof(arr_aux2[0]);
        const int signs[] = {1, -1};
        alternative_height_block += BLOCO_TAMANHO * choose_random(arr_aux2, arr_aux2_length) * choose_random(signs, 2);

        // Limitar alternative_height_block para não sair da tela
        if (alternative_height_block < 100) alternative_height_block = 100;
        if (alternative_height_block > SCREEN_HEIGHT - 200) alternative_height_block = SCREEN_HEIGHT - 200;
    }
}

// --------------------------------------------------------------------------------- //
//
//                              LOOP PRINCIPAL DO JOGO
//  
// --------------------------------------------------------------------------------- //

int main(int argc, char **argv) {

    srand(time(NULL));

    if (!config()) {
        fprintf(stderr, "Falha na inicializacao do Allegro. Encerrando o programa.\n");
        return -1;
    }

    register_events();

    init_player();
    if (!player.sprite || !player.hand_sprite) {
        fprintf(stderr, "Falha ao carregar os sprites do jogador. Encerrando.\n");
        al_destroy_timer(timer);
        al_destroy_display(display);
        al_destroy_event_queue(event_queue);
        al_uninstall_keyboard();
        al_uninstall_mouse();
        al_shutdown_image_addon();
        return -1;
    }

    al_clear_to_color(al_map_rgb(255, 255, 255));
    al_flip_display();

    // Desenha o player
    al_draw_bitmap(player.sprite, player.x, player.y, 0);
    player.facing_direction = 1;

    player.hand_x = player.x + 8;
    player.hand_y = player.y + 16;
    al_draw_bitmap(player.hand_sprite, player.hand_x, player.hand_y, 0);

    // Gera o mundo
    world_generation();
    al_clear_to_color(al_map_rgb(135, 206, 235));
    draw_world_from_map();
    draw_player();

    al_draw_filled_rectangle(FULLSCREEN_BUTTON_X, FULLSCREEN_BUTTON_Y,
                             FULLSCREEN_BUTTON_X + FULLSCREEN_BUTTON_WIDTH,
                             FULLSCREEN_BUTTON_Y + FULLSCREEN_BUTTON_HEIGHT,
                             al_map_rgb(100, 100, 100));

    // COMMIT
    al_flip_display();

    al_start_timer(timer);

    while(1) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(event_queue, &ev);

        if(ev.type==ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            switch(ev.mouse.button){

                case 1:
                    printf("Clique do mouse em: x=%d, y=%d\n", ev.mouse.x, ev.mouse.y);
                
                    // Verifica se o clique foi no botão de tela cheia
                    if (ev.mouse.x >= FULLSCREEN_BUTTON_X &&
                        ev.mouse.x <= FULLSCREEN_BUTTON_X + FULLSCREEN_BUTTON_WIDTH &&
                        ev.mouse.y >= FULLSCREEN_BUTTON_Y &&
                        ev.mouse.y <= FULLSCREEN_BUTTON_Y + FULLSCREEN_BUTTON_HEIGHT)
                    {
                        toggle_fullscreen(); // Ativa/desativa tela cheia
                        reload_world(); 
                    }
                    else {
                        reload_world(); // Caso contrário, recarrega o mundo (conforme sua lógica original)
                    }
                    break;
            }
        } else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES) {
            mouse_x = ev.mouse.x;
            mouse_y = ev.mouse.y;
            redraw = true;
        } else if(ev.type == ALLEGRO_EVENT_TIMER) {
            update_player_physics();
            redraw = true;
        } else if(ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            key_pressed[ev.keyboard.keycode] = true;

            if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                goto end_game;
            }
        } else if(ev.type == ALLEGRO_EVENT_KEY_UP) {
            key_pressed[ev.keyboard.keycode] = false;

        } else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            goto end_game;
        }

        if(redraw && al_is_event_queue_empty(event_queue)) {
            redraw = false;

            al_clear_to_color(al_map_rgb(135, 206, 235));
            draw_world_from_map();
            draw_player();

            // --- DESENHO DO BOTÃO DE TELA CHEIA ---
            al_draw_filled_rectangle(FULLSCREEN_BUTTON_X, FULLSCREEN_BUTTON_Y,
                                     FULLSCREEN_BUTTON_X + FULLSCREEN_BUTTON_WIDTH,
                                     FULLSCREEN_BUTTON_Y + FULLSCREEN_BUTTON_HEIGHT,
                                     al_map_rgb(100, 100, 100));

            al_flip_display();
        }
    }

end_game:
    // Destruir bitmaps
    al_destroy_bitmap(spr_grass);
    al_destroy_bitmap(spr_dirt);
    al_destroy_bitmap(spr_stone);
    al_destroy_bitmap(player.sprite);
    al_destroy_bitmap(player.hand_sprite);

    // Destruir compontes do Allegro
    al_destroy_timer(timer);
    al_destroy_display(display);
    al_destroy_event_queue(event_queue);

    return 0;
}