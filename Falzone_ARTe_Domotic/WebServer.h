//--------------------------------------------------------------------------
// Questa libreria realizza la parte relativa all'interfaccia Web,			
// come coppia WebServer WebSocket, i dati trasmessi tramite WebSocket sono 
// in formato JSON, gli id degli oggetti sono definiti come #define e devono
// coincidere con quelli delle pagine Web.									
// è stata pensata per inviare pagine salvate su SD, per questioni di 		
// velocità è stata poi modificata e queste	si trovano in WebPages.h  		
// I metodi invocabili sono relativi all'controllo eventi per i due Server, 
// alla fase di inizializzazione e alla fase di update delle informazioni.	
// Supporta sia pagine statiche che dinamiche, ma in quest'ultimo caso per 	
// ricevere un diverso set di informazioni tramite il webSocket, il Client 	
// deve prima inviare al Server un comando per informarlo del nuovo set		
// di informazioni richiesto 
//--------------------------------------------------------------------------
#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "MySystem.h"

//#define DEBUG_WEB

//-------------WebServer e WebSocket Parameters-----------------------------
#define SERVER_PORT			80
#define WEBSOCKET_PORT		8080
#define ROOT				"/"
#define MAX_CONNECTION		1

//--------------HTTP Parameters---------------------------------------------
#define HTTP_HEADER_DIM			3
#define HTTP_HEADER_LINE0		"HTTP/1.1 200 OK\0"
#define HTTP_HEADER_LINE1		"Content-Type: text/html\0"
#define HTTP_HEADER_LINE2		""

#define HTTP_HEADER404_DIM		3
#define HTTP_HEADER404_LINE0	"HTTP/1.1 404 Not Found\0"
#define HTTP_HEADER404_LINE1	"Connection: Closed\0"
#define HTTP_HEADER404_LINE2	""

//---------------Files Parameters-------------------------------------------
// 8.3 filename 8 char for name, 1 for dot and 3 for extension
#define MAXFILENAMESIZE		(8 + 1 + 3)

#define NPAGES				3

#define HOMEPAGE_FILE		"home.htm"
#define SENSORPAGE_FILE		"sensor.htm"
#define TASKPAGE_FILE		"task.htm"

//----------------JSON FORMAT Parameter-------------------------------------
#define JSON_ID				"{ \"id\":\"\0"
#define JSON_VALUE			"\",\"value\":\0"
#define JSON_END			" }\0"

//--------------------------------------------------------------------------
// gli elementi della pagina devono avere i seguenti id, il Websocket invia 
// i dati in formato JSON con l'id corrispondente							
//--------------------------------------------------------------------------

//-----------------HomePage Objects' Id-------------------------------------
#define ID_LIGHT_ON			"L\0"
#define ID_LIGHT_AUTO		"LA\0"
#define ID_THERMO_ON		"T\0"
#define ID_THERMO_AUTO		"TA\0"

#define ID_DESLUX			"DesLux\0"
#define ID_DESTEMP			"DesTemp\0"

//-----------------SenrosPage Objects' Id-----------------------------------
#define ID_LUX				"lux\0"
#define ID_GAS				"gas\0"
#define ID_TEMP				"temp\0"
#define ID_HUM				"hum\0"
#define ID_PIR				"pir\0"

//------------------TasksPage Objects' Id-----------------------------------
#define ID_PERIOD			"T%1dPeriod\0"		// formato TXPeriod
#define ID_MAXEXEC			"T%1dMaxExec\0"		// formato TXMaxExec
#define ID_DEADLS			"T%1dDeadmiss\0"	// formato TXDeadmiss
#define ID_EXECTIM 			"T%1dExecTime\0"	// formato TXExecTime
#define ID_AVG_EXECTIM		"T%1dAvgExecTime\0"	// formato TXAvgExecTime

//-----------------Command Char Inviati dal Client--------------------------

//--------------------------------------------------------------------------
// Un comando inviato dal client è una sequenza di caratteri, ad esempio:	
// L0 = Spegni le Luci, L1 = Accendi le luci,								
// L+ = aumenta la luminosità, L- diminuisci la luminosità					
// LA = cambia lo stato di gestione automatica delle luci					
// Il comando S è stato pensato per la gestione delle pagine dinamiche		
//--------------------------------------------------------------------------

#define OFF_CHAR			'0'
#define ON_CHAR				'1'
#define PLUS_CHAR			'+'
#define MINUS_CHAR			'-'
#define AUTO_CHAR			'A'

#define LIGHT_CHAR			'L'
#define THERMO_CHAR			'T'
#define STATE_CHAR			'S'


//--------------------------------------------------------------------------
// Inizializza il controller Ethernet, il WebServer, il WebSocket, registra 
// le funzioni di CallBack relative alle richieste provenienti dal WebSocket
//--------------------------------------------------------------------------
void Init_WebServer();

//--------------------------------------------------------------------------
// Da invocare nel loop, controlla se ci sono richieste provenienti dalla  	
// NIC, analizza la richiesta HTTP e se corretta invia la pagina richiesta	
// Viene servita un sola connessione per volta								
//--------------------------------------------------------------------------
void listenForEthernetClients();

//--------------------------------------------------------------------------
// Invocata dal loop, controlla se ci sono richieste o dati provenienti dal	
// webSocketServer, se vi sono richieste chiama le funzioni di CallBack 	
// relative alla richiesta, va invocata anche quando siamo in Lock per poter
// servire una richiesta di Disconnect										
//--------------------------------------------------------------------------
void ListenIncomingDataFromSocket();

//--------------------------------------------------------------------------
// invocata dal loop, se abbiamo un client connesso alla websocket, provvede
// ad aggiornare la paggina in cui si trova inviandogli le informazioni 	
// relative a quella pagina													
//--------------------------------------------------------------------------
void WebSocketIteration();

#endif