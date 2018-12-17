//--------------------------------------------------------------------------
// Questa libreria realizza la parte relativa all'interfaccia Control Panel 
// comunica con il display tramite seriale UART ed invia a questo stringhe 	
// che vengono interpretate dal dispositivo come comandi
// I metodi invocabili riguardano la fase di inizializzazione, controllo 	
// eventi e update delle informazioni
//--------------------------------------------------------------------------

#ifndef DISPLAY_H
#define DISPLAY_H

#include "MySystem.h"

#define DEBUG_DISPLAY

// per definire il range, usati dalle rispettive icone
#define MAX_TEMP 	MAX_DESIRED_TEMP
#define MIN_TEMP 	MIN_DESIRED_TEMP

#define MAX_HUM		100
#define MIN_HUM		0

#define N_PAGES		4

// pages id
#define LOCKID		0
#define HOMEID		1
#define SETTID		2
#define LOGID		3

//--------------------------------------------------------------------------
// per gli oggetti comuni utilizziamo degli array, ma per poter definire 	
// l'id dell'elemento, dobbiamo utilizzare un offset perchè la lockpage 	
// non ha oggetti in comune							 						
//--------------------------------------------------------------------------
#define OFFSET_PAGE			HOMEID

//--------------------------------------------------------------------------
// di seguito i nomi e gli id relativi agli oggetti del display 			
//--------------------------------------------------------------------------

//--------------------------Elementi comuni---------------------------------
#define BACKGROUNDID		0

#define HOME_BUTTID			1			// Bottone Home
#define HOME_BUTTNAME		"bh"

#define SETT_BUTTID			2			// Bottone Settings
#define SETT_BUTTNAME		"bs"

#define LOG_BUTTID			3			// Bottone Log
#define LOG_BUTTNAME		"bd"

#define THERMO_BUTTID		4			// Bottone Riscaldamento
#define THERMO_BUTTNAME		"bf"

#define LIGHT_BUTTID		5			// Bottone Illuminazione
#define LIGHT_BUTTNAME		"bl"

#define T_PROGID			6			// Temperatura Progress Bar
#define T_PROGNAME			"jt"

#define T_TEXTID			7			// Temperatura Text
#define T_TEXTNAME			"tt"

#define H_GAUGEID			8			// Umidità icona
#define H_GAUGENAME			"zh"

#define H_TEXTID			9			// Umidità Text
#define H_TEXTNAME			"th"


//-----------------elementi pagina Lock-------------------------------------
#define SLIDERID			1			// slider per unlock
#define SLIDERNAME			"hl"

//-----------------elementi aggiuntivi pagina Home--------------------------
#define LOCK_BUTTID			10			// bottone di lock
#define LOCK_BUTTNAME		"blck"

//-----------------elementi aggiuntivi pagina Settings----------------------
#define T_PLUSID			10		// bottone per incrementare la Temperatura
#define T_PLUSNAME			"btp"

#define T_MINUSID			11		// bottone per decrementare la Temperatura
#define T_MINUSNAME			"btm"

#define T_AUTOID			12		// bottone Riscaldamanto Automatico
#define T_AUTONAME			"bfa"

#define L_AUTOID			13		// bottone Illuminazione Automatica
#define L_AUTONAME			"bla"

//-----------------elementi aggiuntivi pagina Log---------------------------
#define LOGLINE1_ID			4			// Linea 1 log Text
#define LOGLINE1_NAME		"l0"

#define LOGLINE2_ID			5			// Linea 2 log Text
#define LOGLINE2_NAME		"l1"

#define LOGLINE3_ID			6			// Linea 3 log Text
#define LOGLINE3_NAME		"l2"

#define LOGLINE4_ID			7			// Linea 4 log Text
#define LOGLINE4_NAME		"l3"

//------------------Parametri relativi ai messagi e al buffer--------------

#define NUM_LOG_LINE		4			// numero righe di log

#define NUM_BUFFER_LINE		64			//dimensione buffer messaggi

#define LINE_LEN			24			// lunghezza testo messaggi

#define TEXT_INTRUSION		"Intrusion Detected!!"

#define TEXT_GAS_ALARM		"Gas Detected!!"

//------------------------Display Commands0---------------------------------
#define GOTO_LOCKPAGE		"page 0"
#define GOTO_HOMEPAGE		"page 1"
#define GOTO_SETTPAGE		"page 2"
#define GOTO_LOGPAGE		"page 3"

#define RETURN_MSG_CMD		"bkcmd=1"
//------------------------Operations----------------------------------------

//--------------------------------------------------------------------------
// Inizializzazione del display, vengono associate le operazioni di CallBack
// per gli eventi e viene inizializzato il display alla pagina di Lock 		
//--------------------------------------------------------------------------
void InitDisplay();

//--------------------------------------------------------------------------
// funzione invocata dal loop, controlla se ci sono eventi e se presenti 	
// invoca la funzione associata all'evento.									
// Aggiorna  le informazioni relative alla pagina in cui si trova 			 
//--------------------------------------------------------------------------
void DisplayIteration();

//--------------------------------------------------------------------------
// funzione per cambiare pagina ed andare nella HomePage 					
//--------------------------------------------------------------------------
void DisplayShowHomePage();

void DisplayShowLockPage();

#endif