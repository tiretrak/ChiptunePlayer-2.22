#include "ayplayer.h"
#include "core_cm3.h"
#include "ayplayer_os_object.h"

int AyPlayer::fsmStepFuncHardwareMcInit ( HANDLER_FSM_INPUT_DATA ) {
	BASE_RESULT r;

	/*!
	 * WDT init.
	 */
	//r = obj->mcu->wdt->reinit( 0 );
	//assertParam( r == BASE_RESULT::OK );

	/*!
	 * GPIO init.
	 */
	///
	r = obj->mcu->gp->reinitAllPorts();
	assertParam( r == BASE_RESULT::OK );

	/*!
	 * RCC и все объекты, зависящие от него.
	 */
	obj->rccMaxFrequancyInit();

	/*!
	 * NVIC.
	 */
	NVIC_SetPriority( DMA1_Stream3_IRQn, 14 );
	NVIC_SetPriority( DMA1_Stream4_IRQn, 14 );
	NVIC_SetPriority( DMA1_Stream7_IRQn, 14 );
	NVIC_SetPriority( DMA2_Stream5_IRQn, 14 );
	NVIC_SetPriority( DMA2_Stream6_IRQn, 14 );

	NVIC_SetPriority( USART3_IRQn, 15 );

	NVIC_EnableIRQ( DMA1_Stream3_IRQn );
	NVIC_EnableIRQ( DMA1_Stream4_IRQn );
	NVIC_EnableIRQ( DMA1_Stream7_IRQn );
	NVIC_EnableIRQ( DMA2_Stream5_IRQn );
	NVIC_EnableIRQ( DMA2_Stream6_IRQn );

	NVIC_EnableIRQ( USART3_IRQn );

	/*!
	 * После инициализации запускаем все модули,
	 * которые должны всегда находиться в работе.
	 */
	obj->startBaseInterfaces();
	return 0;
}

int AyPlayer::fsmStepFuncFreeRtosObjInit ( HANDLER_FSM_INPUT_DATA ) {
	obj->os->qAyLow[0]		= USER_OS_STATIC_QUEUE_CREATE( QB_AY_LOW_SIZE, sizeof( ayLowOutDataStruct ), &obj->os->qbAyLow[0][0], &obj->os->qsAyLow[0] );
	obj->os->qAyLow[1]		= USER_OS_STATIC_QUEUE_CREATE( QB_AY_LOW_SIZE, sizeof( ayLowOutDataStruct ), &obj->os->qbAyLow[1][0], &obj->os->qsAyLow[1] );
	obj->os->qAyButton		= USER_OS_STATIC_QUEUE_CREATE( 1, sizeof( uint8_t ), obj->os->qbAyButton, &obj->os->qsAyButton );

	obj->os->sPlayTic		= USER_OS_STATIC_BIN_SEMAPHORE_CREATE( &obj->os->sbPlayTic );
	obj->os->sGuiUpdate		= USER_OS_STATIC_BIN_SEMAPHORE_CREATE( &obj->os->sbGuiUpdate );

	obj->os->mSpi3			= USER_OS_STATIC_MUTEX_CREATE( &obj->os->mbSpi3 );
	obj->os->mHost			= USER_OS_STATIC_MUTEX_CREATE( &obj->os->mbHost );

	return 0;
}

int AyPlayer::fsmStepFuncHardwarePcbInit ( HANDLER_FSM_INPUT_DATA ) {
	/// У ay, srButton и button одинаковая инициализация.
	/// Поэтому только 1 вызываем.
	obj->pcb->srAy->init();
	obj->pcb->ay->init();

	return 0;
}

extern "C" {
extern const MakiseGUI mGui;
}

int AyPlayer::fsmStepFuncGuiInit ( HANDLER_FSM_INPUT_DATA ) {
	obj->g.c.gui						= ( MakiseGUI* )&mGui;
	obj->g.h.host						= &obj->g.c;
	obj->g.h.host->gui					= ( MakiseGUI* )&mGui;

	int r;
	r = makise_start( obj->g.h.host->gui );
	assertParam( r == M_OK );

	obj->l->sendMessage( RTL_TYPE_M::INIT_OK, "MakiseGui started." );

	return 0;
}

int AyPlayer::fsmStepFuncMicroSdInit ( HANDLER_FSM_INPUT_DATA ) {
	/*!
	 * Проверяем наличие обеих флешек в слотах.
	 */
	obj->waitSdCardInsert();

	return 0;

	/*
	 * 	FRESULT fr;

	 * Пытаемся сразу же примонтировать флешки.

	fr = f_mount( &obj->fSd1, "0:", 1 );

	fr = f_mount( &obj->fSd2, "1:", 1 );
		if ( fr == FR_OK ) {
			if ( obj->l->send_message( RTL_TYPE_M::INIT_OK, "FatFS initialized successfully." ) != BASE_RESULT::OK ) return 2;
			return 0;
		} else {
			if ( obj->l->send_message( RTL_TYPE_M::INIT_ERROR, "FatFS was not initialized!" ) != BASE_RESULT::OK ) return 2;
			return 1;
		}
		*/
}



