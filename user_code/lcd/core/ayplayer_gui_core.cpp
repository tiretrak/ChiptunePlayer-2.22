#include "ayplayer_gui_core.h"

static StaticTask_t     ayplayer_gui_task_buffer;
static StackType_t      ayplayer_gui_task_stack[ AY_PLAYER_GUI_TASK_STACK_SIZE ];

// Задача GUI.
void ayplayer_gui_core_init ( void ) {
    xTaskCreateStatic( ayplayer_gui_core_task,
                       "gui",
                       AY_PLAYER_GUI_TASK_STACK_SIZE,
                       NULL,
                       3,
                       ayplayer_gui_task_stack,
                       &ayplayer_gui_task_buffer );
}

ayplayer_state ayplayer_control;

// Внутренние объекты (общие для всех окон).


char                   path_dir[512] = "0:/";
FATFS                  fat;

extern MHost           host;
// Окна.
MContainer             ayplayer_gui_win_play_list   = { &m_gui, nullptr, nullptr, nullptr, nullptr, nullptr };
MContainer             ayplayer_gui_win_play        = { &m_gui, nullptr, nullptr, nullptr, nullptr, nullptr };

// Элементы GUI.
MPlayList              pl;
MPlayList_Item         pl_item_array[4];

static void container_set_to_mhost () {
    switch ( M_EC_TO_U32( ayplayer_control.active_window_get() ) ) {
    case M_EC_TO_U32( EC_AY_ACTIVE_WINDOW::PLAY_LIST ):
        host.host = &ayplayer_gui_win_play_list;
        break;

    case M_EC_TO_U32( EC_AY_ACTIVE_WINDOW::MAIN ):
        host.host = &ayplayer_gui_win_play;
        break;
    }
}

extern  USER_OS_STATIC_QUEUE         ay_b_queue;
extern  USER_OS_STATIC_MUTEX         spi2_mutex;

MPlayBar gui_e_pb;

extern const MakiseFont F_minecraft_rus_regular_8;

MakiseStyle_PlayBar gui_pb_style = {
    .bg_color           = MC_White,
    .border_color       = MC_Black,
    .duty_color         = MC_Black,
    .time_color         = MC_Black,
    .font               = &F_minecraft_rus_regular_8
};

//**********************************************************************
// Через данную задачу будут происходить все монипуляции с GUI.
//**********************************************************************
void ayplayer_gui_core_task ( void* param ) {
    ( void )param;
    // Потом вынести!
    m_create_play_bar( &gui_e_pb,
                       &ayplayer_gui_win_play,
                       mp_rel( 0,   0,
                               128, 7 ),
                       0,
                       &gui_pb_style );

    // Готовим низкий уровень GUI и все необходимые структуры.
    ayplayer_gui_low_init();
    container_set_to_mhost();

    uint8_t b_buf_nember;

    // Инициализация FAT объекта (общий на обе карты).
    FRESULT fr = f_mount( &fat, "", 0 );
    if ( fr != FR_OK ) while( true );

    // Составить список PSG файлов, если нет такого на карте.
    FIL file_list;
    USER_OS_TAKE_MUTEX( spi2_mutex, portMAX_DELAY );
    fr = f_open( &file_list, "psg_list.txt", FA_READ );
    USER_OS_GIVE_MUTEX( spi2_mutex );
    if ( fr == FR_NO_FILE )     ayplayer_sd_card_scan( path_dir, &ayplayer_gui_win_play_list );

    ayplayer_gui_window_file_list_creature( &ayplayer_gui_win_play_list, &pl, pl_item_array, path_dir );
    makise_g_focus( &pl.e, M_G_FOCUS_GET );
    gui_update();

    while ( true ) {
        USER_OS_QUEUE_RECEIVE( ay_b_queue, &b_buf_nember, portMAX_DELAY );

        switch ( M_EC_TO_U32( ayplayer_control.active_window_get() ) ) {
        case M_EC_TO_U32( EC_AY_ACTIVE_WINDOW::PLAY_LIST ):
            switch ( b_buf_nember ) {
            case M_EC_TO_U8( EC_BUTTON_NAME::DOWN ):
                makise_gui_input_send_button( &host, M_KEY_DOWN, M_INPUT_CLICK, 0 );
                break;

            case M_EC_TO_U8( EC_BUTTON_NAME::UP ):
                makise_gui_input_send_button( &host, M_KEY_UP, M_INPUT_CLICK, 0 );
                break;

            case M_EC_TO_U8( EC_BUTTON_NAME::ENTER ):
                makise_gui_input_send_button( &host, M_KEY_OK, M_INPUT_CLICK, 0 );
                break;

            case M_EC_TO_U8( EC_BUTTON_NAME::RIGHT_CLICK ):
            case M_EC_TO_U8( EC_BUTTON_NAME::LONG_RIGHT_CLICK ):
                ayplayer_control.active_window_set( EC_AY_ACTIVE_WINDOW::MAIN );
                container_set_to_mhost();
                break;
            }
        case M_EC_TO_U32( EC_AY_ACTIVE_WINDOW::MAIN ):
            switch ( b_buf_nember ) {
            case M_EC_TO_U8( EC_BUTTON_NAME::LEFT_CLICK ):
            case M_EC_TO_U8( EC_BUTTON_NAME::LONG_LEFT_CLICK ):
                ayplayer_control.active_window_set( EC_AY_ACTIVE_WINDOW::PLAY_LIST );
                container_set_to_mhost();
                break;
            }
        }

        makise_gui_input_perform( &host );
        gui_update();
    }
}
