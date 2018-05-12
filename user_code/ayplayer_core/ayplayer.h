#pragma once

#include "fsm.h"
#include "wdt.h"
#include "uart.h"
#include "rcc.h"
#include "spi.h"
#include "adc.h"
#include "pwr.h"
#include "timer.h"

#include "module_digital_potentiometer_ad5204.h"
#include "microsd_card_spi.h"
#include "makise_gui.h"
#include "makise.h"

#include <string>

#include "run_time_logger.h"
#include "ay_ym_file_mode.h"
#include "buttons_through_shift_register_one_input_pin.h"
#include "ayplayer_gpio.h"
#include "shift_register.h"
#include "ayplayer_os_object.h"
#include "ayplayer_microsd_card.h"
#include "mono_lcd_lib_st7565.h"

#include "ayplayer_fat_error_string.h"

#include "makise_e_message_window.h"
#include "fsviewer.h"
#include "play_bar.h"
#include "play_list.h"
#include "player_status_bar.h"
#include "progress_bar.h"
#include "slist.h"

/*!
 * Количество режимов тактирования контроллера.
 */
#define	AYPLAYER_RCC_CFG_COUNT					4

#define HANDLER_FSM_STEP(NAME_STEP)				static int NAME_STEP ( const fsmStep< AyPlayer >* previousStep, AyPlayer* obj )
#define HANDLER_FSM_INPUT_DATA					__attribute__((unused)) const fsmStep< AyPlayer >* previousStep, AyPlayer* obj

#define	TB_MAIN_TASK_SIZE						3000
#define	TB_ILLUMINATION_CONTROL_TASK_SIZE		64
#define	TB_BUTTON_CLICK_HANDLER_TASK_SIZE		1000

#define	MAIN_TASK_PRIO							3
#define	ILLUMINATION_CONTROL_TASK_PRIO			1
#define	BUTTON_CLICK_HANDLER_TASK_PRIO			1

#define LIST_NO_SORT_FAT_NAME					".fileList.list"
#define LIST_SORT_NAME_FAT_NAME					".fileListNameSort.list"
#define LIST_SORT_LEN_FAT_NAME					".fileListLenSort.list"

enum class AYPLAYER_STATUS {
	STOP				=	0,
	PLAY				=	1,
	PAUSE				=	2
};

struct ayplayerMcStrcut {
	WdtBase*											wdt;
	ayplayerGpio*										gpio;
	GlobalPortBase*										gp;
	UartBase*											debugUart;
	RccBase*											rcc;
	SpiMaster8BitBase*									spi1;
	SpiMaster8BitBase*									spi2;
	SpiMaster8BitBase*									spi3;
	AdcOneChannelBase*									adc1;
	TimCompOneChannelBase*								ayClkTim;
	TimPwmOneChannelBase*								lcdPwmTim;
	TimInterruptBase*									interruptAyTim;
	Pwr*												pwr;

#ifdef configGENERATE_RUN_TIME_STATS
	TimCounter*											timRunTimeStats;
#endif
};

struct ayplayerPcbStrcut {
	ShiftRegister*										srAy;
	ShiftRegister*										srButton;
	ButtonsThroughShiftRegisterOneInputPin*				button;
	ad5204< 2 >*										dp;
	AyYmLowLavel*										ay;
	ST7565*												lcd;
	MicrosdBase** 										sd;
};

struct ayplayerGuiCfg {
	MakiseStyle_SMessageWindow							smw;
	MakiseStyle_SList									ssl;
	MakiseStyle_SListItem								sslItem;
	MakiseStyle_SMPlayerStatusBar						statusBarCfg;
	MPlayerStatusBar_CallbackFunc						statusBarCallbackCfg;
	MakiseStyle_PlayBar									playBarStyle;
};

struct ayPlayerCfg {
	ayplayerMcStrcut*									mcu;
	RunTimeLogger*										l;
	ayplayerPcbStrcut*									pcb;
	AyYmFileMode*										ayF;
	freeRtosObj*										os;
	const ayplayerGuiCfg*								const gui;
};


struct ayPlayerGui {
	MMessageWindow										mw;
	MSList												sl;
	MSList_Item											slItem[ 4 ];
	MPlayerStatusBar									sb;
	MPlayBar											pb;
};

enum class AY_FORMAT {
	PSG				=	0,
};

#define ITEM_FILE_IN_FAT_FILE_NAME_LEN			256

/// Структура одного элемента в файле <<.fileList.txt>>.
struct itemFileInFat {
	char			fileName[ ITEM_FILE_IN_FAT_FILE_NAME_LEN ];
	AY_FORMAT		format;
	uint32_t		lenTick;
};

#define AYPLAYER_MICROSD_COUNT							2

struct ayPlayerFatFs {
	FATFS						f[ AYPLAYER_MICROSD_COUNT ];

	/// Информация о файле, который в данный момент воспроизводится.
	itemFileInFat				currentFileInfo;

	/// Директория, в которой находится файловый менеджер.
	DIR							playDir;
};

enum class FILE_LIST_TYPE {
	NO_SORT			=	0,
	NAME_SORT		=	1,
	LEN_SORT		=	2,
};

class AyPlayer {
public:
	AyPlayer( ayPlayerCfg* cfg ) :
		mcu			( cfg->mcu ),
		l			( cfg->l ),
		pcb			( cfg->pcb ),
		ayFile		( cfg->ayF ),
		os			( cfg->os ),
		gui			( cfg->gui )
	{}

	void			start						( void );

	/*!
	 * Возвращает текущий режим работы RCC.
	 */
	uint32_t		getRccMode					( void );

	uint32_t		getStatePlay				( void );
	uint32_t		getPercentBattery			( void );

	/// Шаги FSM. Инициализация.
	HANDLER_FSM_STEP( fsmStepFuncFreeRtosObjInit );
	HANDLER_FSM_STEP( fsmStepFuncHardwareMcInit );
	HANDLER_FSM_STEP( fsmStepFuncHardwarePcbInit );
	HANDLER_FSM_STEP( fsmStepFuncGuiInit );
	HANDLER_FSM_STEP( fsmStepFuncMicroSdInit );
	HANDLER_FSM_STEP( fsmStepFuncIndexingSupportedFiles );
	HANDLER_FSM_STEP( fsmStepFuncSortingFileList );
	HANDLER_FSM_STEP( fsmStepFuncCheckingChangeFatVolume );
	HANDLER_FSM_STEP( fsmStepFuncCleanFlagChangeFatVolume );
	HANDLER_FSM_STEP( fsmStepFuncInitMainWindow );

private:
	static	void	mainTask						( void* obj );
	static	void	illuminationControlTask			( void* obj );
	static	void	playTask						( void* obj );
	static	void	buttonClickHandlerTask			( void* obj );


	/*!
	 * Останавливает все аппаратные модули, зависящие от тактового сигнала,
	 * пытается установить заданную тактовую частоту на шинах, после чего
	 * конфигурирует заново все объекты, зависящие от частоты тактового сигнала
	 * ( вызывается метод reinitObjDependingRcc ).
	 *
	 * Замечание: в случае, если не удалось настроить заданную конфигурацию - микроконтроллер переходит
	 * в режим работы по умолчанию.
	 */
	RCC_RESULT		setRccCfg							( uint32_t numberCfg );

	/*!
	 * Отключает все объекты, зависящие
	 * от частоты тактового сигнала.
	 */
	void			offObjDependingRcc					( void );

	/*!
	 * Метод переконфигурирует все объекты, зависящие
	 * от частоты тактового сигнала.
	 */
	void			reinitObjDependingRcc				( void );

	/*!
	 * Запускает базовые интерфейсы:
	 * SPI, UART, ADC (канал аккумулятора).
	 */
	void			startBaseInterfaces					( void );

	/*!
	 * Инициализирует RCC с максимально возможной скоростью.
	 */
	void			rccMaxFrequancyInit					( void );

	/*!
	 * Возвращает флаг наличия запрошенной microsd в слоте.
	 * \param[in]	sd	-	проверяемая карта.
	 * \return		{	true	-	карта обнаружена..
	 * 					false	-	в противном случае.}
	 */
	bool			checkSd								( AY_MICROSD sd );

	/*!
	 * Ожидает, пока будут вставлены обе microsd карты.
	 * Выводит на экран и в лог соответвующие выводы.
	 */
	void			waitSdCardInsert					( void );

	/*!
	 * Перерисовывает экран и обновляет буфер в экране..
	 */
	void			guiUpdate							( void );

	/*!
	 * Попытка инициализировать FatFS для выбранной карты.
	 */
	FRESULT			fatFsReinit							( AY_MICROSD sd );

	/*!
	 * Рисует сообщение об ошибке microsd.
	 */
	void			errorMicroSdDraw					( const AY_MICROSD sd, const FRESULT r );

	/*!
	 * Ждем, пока отсоединят флешку.
	 */
	void			waitSdCardDisconnect				( const AY_MICROSD sd );

	/*!
	 * Проходится по всем каталогам и составляет список поддерживаемых файлов.
	 */
	FRESULT			indexingSupportedFiles				( char* path );

	void			printMessageAndArg					( RTL_TYPE_M type, const char* const message, const char* const arg );

	void			initWindowIndexingSupportedFiles	( char* stateIndexing = nullptr );
	void			removeWindowIndexingSupportedFiles	( void );


	void			initGuiStatusBar					( void );
	void			slItemShiftDown							( uint32_t cout, char* newSt );

	int				scanDir								( char* path );

	/// 0 - успех. -1 провал
	int				writeItemFileList					( FIL* f, const uint32_t* const len, const AY_FORMAT format );

	void			slItemClean							( uint32_t cout );
	itemFileInFat*	structureItemFileListFilling		( const char* const nameTrack, const uint32_t lenTickTrack, const AY_FORMAT format );
	int				sortFileList						( char* path );
	FRESULT			findingFileListAndSort				( char* path );
	int				sortFileListCreateFile				( const char* const path, FIL** fNoSort, FIL** fNameSort, FIL** fLenSort );
	int				sortFileListCloseFile				( const char* const path, DIR* d, FILINFO* fi, FIL* fNoSort, FIL* fNameSort, FIL* fLenSort );
	int				writeSortFile						( FIL* output, FIL* input, uint16_t* sortArray, uint32_t count );
	void			initWindowSortingFileList			( void );
	void			removeWindowSortingFileList			( void );
	void			initPointArrayToSort				( uint16_t* array, uint32_t count );
	int				sortForNameFileList					( const char* const path, uint16_t* fl, uint32_t countFileInDir, FILINFO* fi, DIR* d, FIL* fNoSort, FIL* fNameSort );
	int				sortForLenFileList					( const char* const path, uint16_t* fl, uint32_t countFileInDir, FILINFO* fi, DIR* d, FIL* fNoSort, FIL* fLenSort );
	void			removeSystemFileInRootDir			( AY_MICROSD sdName, const char* fatValome );

	/// Проверяем наличие файла в директории 0 - нет, 1 есть, -1 флешке плохо.
	int checkingFile( AY_MICROSD sdName, const char* path, const char* nameFile, FILINFO* fi  );

	int removeFile( AY_MICROSD sdName, const  char* path, const char* nameFile );

	int removeDirRecurisve( AY_MICROSD sdName, const char* path, const char* nameDir );

	/// Рабочие окна приложения (отрисовки).
	/// Статусы дерева переходов ставятся через переменные состояния ниже.
	/// Окно воспроизведения трека
	void initPlayWindow ( void );
	void removePlayWindow ( void );

	/*!
	 * Находим системные файлы в директории.
	 * Например, карзина или индексер.
	 * Причем от разныех ОС.
	 */
	int			checkingSystemFileInRootDir				( AY_MICROSD sdName, const char* fatValome );

	/// Заполняет внутренную переменную currentFileInfo
	/// данными из файла-списка на флешке.
	int			getFileInfoFromListCurDir ( FILE_LIST_TYPE listType, uint32_t numberFileInList );


	/// В объекте fat для sd1 должена быть установлена актуальная директория с помощью f_chdir
	/// Ну то есть мы указываем имя файла относительно текущей директории.
	int		startPlayFile							( void );

	/// Текущий режим работы RCC.
	uint32_t											rccIndex = 0;

	fsmClass< AyPlayer >								fsm;

	ayplayerMcStrcut*									const mcu;
	RunTimeLogger*										const l;
	ayplayerPcbStrcut*									const pcb;
	AyYmFileMode*										const ayFile;
	freeRtosObj*										const os;
	const ayplayerGuiCfg*								const gui;
	ayPlayerGui											g;

	USER_OS_STATIC_STACK_TYPE							tbMainTask[ TB_MAIN_TASK_SIZE ];
	USER_OS_STATIC_TASK_STRUCT_TYPE						tsMainTask;
	USER_OS_STATIC_STACK_TYPE							tbIlluminationControlTask[ TB_ILLUMINATION_CONTROL_TASK_SIZE ];
	USER_OS_STATIC_TASK_STRUCT_TYPE						tsIlluminationControlTask;
	USER_OS_STATIC_STACK_TYPE							tbButtonClickHandlerTask[ TB_BUTTON_CLICK_HANDLER_TASK_SIZE ];
	USER_OS_STATIC_TASK_STRUCT_TYPE						tsButtonClickHandlerTask;
	/// Яркость подсветки.
	float												illuminationDuty = 1;

	AYPLAYER_STATUS										playState;
	FILE_LIST_TYPE										lType;
	uint32_t											currentFile;

	ayPlayerFatFs										fat;
};
