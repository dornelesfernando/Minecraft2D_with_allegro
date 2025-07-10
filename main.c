// --- BIBLIOTECAS UTILIZADAS ---
#include <stdio.h>
#include <stdbool.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <math.h>
#include <time.h>

// --- VARIÁVEIS DE CONFIGURAÇÃO ---
#define FPS 90.0f
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define BLOCO_TAMANHO 32

// --- CONSTANTES DE FÍSICA ---
#define GRAVITY 0.5f
#define JUMP_STRENGTH -8.0f
#define PLAYER_SPEED 3.0f

// --- CONFIGURAÇÕES ---
ALLEGRO_DISPLAY *display = NULL;
ALLEGRO_EVENT_QUEUE *event_queue = NULL;
ALLEGRO_TIMER *timer = NULL;

// --- SPRITES ---
ALLEGRO_BITMAP *spr_grass = NULL;
ALLEGRO_BITMAP *spr_dirt = NULL;
ALLEGRO_BITMAP *spr_stone = NULL;

// --- VARIÁVEIS GLOBAIS ---
bool redraw = false;
bool is_fullscreen = false;

// --- CONSTANTES PARA  BOTÃO DE TELA CHEIA ---
#define FULLSCREEN_BUTTON_X (SCREEN_WIDTH - 120)
#define FULLSCREEN_BUTTON_Y 10
#define FULLSCREEN_BUTTON_WIDTH 100
#define FULLSCREEN_BUTTON_HEIGHT 30

// --------------------------------------------------------------------------------- //

// --- PROTÓTIPOS DE FUNÇÕES ---
bool config();
bool renderSprites();
void destroyBitmapSprites();
int choose_random(const int arr[], int size);
void world_generation();
void reload_world();
bool init_player();
ALLEGRO_BITMAP* load_and_scale_bitmap(const char* filename, int width, int height);

// --------------------------------------------------------------------------------- //

// --- STRUCT DO JOGADOR
typedef struct Player {
    float x;
    float y;
    float hand_x;
    float hand_y;
    float dx; // Velocidade em X
    float dy; // Velocidade em Y
    int width;
    int height;
    ALLEGRO_BITMAP *sprite; // sprite do corpo (spr_steve)
    ALLEGRO_BITMAP *hand_sprite; // sprite da mão (spr_steve_hand)
    bool on_ground;
    int facing_direction;
} Player;

Player player; // instância global

// --------------------------------------------------------------------------------- //

bool init_player() {
    player.width = BLOCO_TAMANHO;
    player.height = BLOCO_TAMANHO * 2;

    player.sprite = load_and_scale_bitmap("sprites/spr_steve.png", player.width, player.height);
    if (!player.sprite) {
        fprintf(stderr, "falha ao criar player.sprite!\n");
        destroyBitmapSprites();
        return false;
    }

    player.hand_sprite = load_and_scale_bitmap("sprites/spr_steve_hand.png", 16, 24);
    if (!player.hand_sprite) {
        fprintf(stderr, "falha ao criar player.hand_sprite!\n");
        destroyBitmapSprites();
        return false;
    }

    return true;
}

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

void world_generation() {
    // 'heights_blocks' retrata a altura inicial dos blocos na geração dos mundos
    // esse valor é escolhido aleatóriamente a partir de um array de 5 números pré-definidos
    // pela função 'choose_random()'. Após, como é um objeto dinâmico que fica alteradndo
    // a partir do tempo do sistema, essa variável é salva para não sofrer alterações
    // e é utilizada para a criação dos blocos do mundo.

    const int heights_block[] = {176 + 32, 208 + 32, 240 + 32, 272 + 32, 304 + 32};
    const int height_block = sizeof(heights_block) / sizeof(heights_block[0]);
    int starting_height_block = choose_random(heights_block, height_block);
    int alternative_height_block = starting_height_block;

    int dirt_level;
    int stone_level;
    int alternative_dirt_level = alternative_height_block; // Inicializado para evitar lixo

    int x;
    for(x = 0; x < SCREEN_WIDTH; x += BLOCO_TAMANHO) {

        int arr_aux[] = {2, 3, 3, 3, 4, 4};
        int arr_aux_length = sizeof(arr_aux) / sizeof(arr_aux[0]);
        dirt_level = alternative_height_block + BLOCO_TAMANHO * choose_random(arr_aux, arr_aux_length);

        stone_level = SCREEN_HEIGHT;

        al_draw_bitmap(spr_grass, x, alternative_height_block, 0);

        int y;
        for(y = alternative_height_block + BLOCO_TAMANHO; y < dirt_level; y += BLOCO_TAMANHO) {
            al_draw_bitmap(spr_dirt, x, y, 0);
            alternative_dirt_level = y;
        }

        for(y = alternative_dirt_level + BLOCO_TAMANHO; y < stone_level; y += BLOCO_TAMANHO) {
            al_draw_bitmap(spr_stone, x, y, 0);
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

int choose_random(const int arr[], int size) {
    int index = rand() % size;
    return arr[index];
}

void reload_world() {
    al_clear_to_color(al_map_rgb(135, 206, 235));
    world_generation();
    al_flip_display();
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

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());

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

    al_start_timer(timer);

    al_clear_to_color(al_map_rgb(135, 206, 235));
    world_generation();
    al_draw_bitmap(player.sprite, player.x, player.y, 0);
    player.facing_direction = 1;

    // desenho dos braços do steve
    player.hand_x = player.x + 8;
    player.hand_y = player.y + 16;

    al_draw_bitmap(player.hand_sprite, player.hand_x, player.hand_y, 0);

    al_flip_display();

    while(1) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(event_queue, &ev);

        if(ev.type==ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            switch(ev.mouse.button){

                case 1:
                    printf("x:%d y:%d\n",ev.mouse.x, ev.mouse.y);
                    reload_world();
                    break;
            }
        } else if(ev.type == ALLEGRO_EVENT_TIMER) {
            // A cada iteração do código
            redraw = true;
        }

        else if(ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            //Tecla apertada
            switch(ev.keyboard.keycode) {

                // ↑
                case ALLEGRO_KEY_A:
                    break;

                // ↓
                case ALLEGRO_KEY_D:
                    break;

                // W
                case ALLEGRO_KEY_W:
                    break;

                // S
                case ALLEGRO_KEY_S:
                    break;

                // ESC
                case ALLEGRO_KEY_ESCAPE:
                    goto end_game;
            }
        } else if(ev.type == ALLEGRO_EVENT_KEY_UP) {
            // Soltar a tecla
            switch(ev.keyboard.keycode) {

                // ↑
                case ALLEGRO_KEY_A:
                    player.dx = -3;
                    player.facing_direction = -1; // Virar para a esquerda
                    break;

                // ↓
                case ALLEGRO_KEY_D:
                    player.dx = 3;
                    player.facing_direction = 1; // Virar para a esquerda
                    break;

                // W
                case ALLEGRO_KEY_W:
                    break;

                // S
                case ALLEGRO_KEY_S:
                    break;
            }

        } else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            goto end_game;
        }

        if(redraw && al_is_event_queue_empty(event_queue)) {
            redraw = false;
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