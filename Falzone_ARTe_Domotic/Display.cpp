#include "Display.h"
#include <Nextion.h>

//--------------------------------------------------------------------------
// Teniamo un Array circolare per mantenere le informazioni da rapprsentare 
// nella log page del dispositivo
//--------------------------------------------------------------------------
static char	BufferLine[NUM_BUFFER_LINE][LINE_LEN];
static int	toread = 0;			// messaggi inseriti e da leggere
static int	rear = 0;			// indice di inserimento
static int	front = 0;			// indice di prelievo

//--------------------------------------------------------------------------
// Utilizziamo un Buffer per i messaggi che vengono mostrati nella log Page 
//--------------------------------------------------------------------------
static char	DisplayBuffer[NUM_LOG_LINE][LINE_LEN];
static int	inDisplay = 0;			// messaggi presenti

static bool DisplayBufferChanged = true;

static bool	firstIntrusion = true;	// un messaggio per ogni intrusione
static bool	firstGasAlarm = true;	// un messaggio per ogni allarme Gas

static int	CurrentPage = 0;			// pagina sulla quale ci troviamo

static float old_temp = 0;

static int	old_hum = 0;
static int	old_page = 0;

//--------------------------------------------------------------------------
// gli oggetti presenti in più pagine vengono organizzati in array 
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//							LOCK PAGE OBJECTS								
//--------------------------------------------------------------------------

// oggetto relativo allo slider di Unlock
static NexSlider	LockSlider = NexSlider(LOCKID, SLIDERID, SLIDERNAME);

// oggetti relativi al bottone HOME
static NexButton	HomeButt[] = { 
	NexButton(HOMEID, HOME_BUTTID, HOME_BUTTNAME), 
	NexButton(SETTID, HOME_BUTTID, HOME_BUTTNAME), 
	NexButton(LOGID, HOME_BUTTID, HOME_BUTTNAME)
};

// oggetti relativi al bottone SETTINGS
static NexButton	SettButt[] = { 
	NexButton(HOMEID, SETT_BUTTID, SETT_BUTTNAME), 
	NexButton(SETTID, SETT_BUTTID, SETT_BUTTNAME), 
	NexButton(LOGID, SETT_BUTTID, SETT_BUTTNAME)
};

// oggetti relativi al bottone LOG
static NexButton	LogButt[] = {
	NexButton(HOMEID, LOG_BUTTID, LOG_BUTTNAME), 
	NexButton(SETTID, LOG_BUTTID, LOG_BUTTNAME), 
	NexButton(LOGID, LOG_BUTTID, LOG_BUTTNAME)
};

// oggetti relativi al bottone del riscaldamento (fiamma sul display)
static NexDSButton	ThermoButt[] = {
	NexDSButton(HOMEID, THERMO_BUTTID, THERMO_BUTTNAME),
	NexDSButton(SETTID, THERMO_BUTTID, THERMO_BUTTNAME)
};

// oggetti relativi al bottone dell'illuminazione (lampadina sul display)
static NexDSButton	LightButt[] = {
	NexDSButton(HOMEID, LIGHT_BUTTID, LIGHT_BUTTNAME),
	NexDSButton(SETTID, LIGHT_BUTTID, LIGHT_BUTTNAME)
};

//--------------------------------------------------------------------------
//							HOME PAGE's OBJECTS								
//--------------------------------------------------------------------------

// oggetto relativo al bottone di Lock
static NexButton	LockButton = NexButton(HOMEID, LOCK_BUTTID, LOCK_BUTTNAME);

//--------------------------------------------------------------------------
//							SETTINGS PAGE's OBJECTS							
//--------------------------------------------------------------------------

// oggetto relativo al bottone Riscaldamento automatico
static NexDSButton	ThermoAutoButt = NexDSButton(SETTID, T_AUTOID, T_AUTONAME);

// oggetto relativo al bottone Illuminazione automatica
static NexDSButton	LightAutoButt = NexDSButton(SETTID, L_AUTOID, L_AUTONAME);

// oggetto relativo al bottone Aumenta Temperatura desiderata
static NexButton	TempPlusButt = NexButton(SETTID, T_PLUSID, T_PLUSNAME);

// oggetto relativo al bottone riduci Temperatura desiderate
static NexButton	TempMinusButt = NexButton(SETTID, T_MINUSID, T_MINUSNAME);

//--------------------------------------------------------------------------
//							LOG PAGE's OBJECTS								
//--------------------------------------------------------------------------

// oggetto relativo alla prima riga del testo
static NexText	LogTextL1 = NexText(LOGID, LOGLINE1_ID, LOGLINE1_NAME);

// oggetto relativo alla seconda riga del testo
static NexText	LogTextL2 = NexText(LOGID, LOGLINE2_ID, LOGLINE2_NAME);

// oggetto relativo alla terza riga del testo
static NexText	LogTextL3 = NexText(LOGID, LOGLINE3_ID, LOGLINE3_NAME);

// oggetto relativo alla quarta riga del testo
static NexText	LogTextL4 = NexText(LOGID, LOGLINE4_ID, LOGLINE4_NAME);

//--------------------------------------------------------------------------
// i seguenti oggetti non sono di tipo touch e quindi non hanno un evento 	
// associato, e dal momento che a oggetti comuni per pagine diverse ho 		
// associato stesso id e nome, e dal momento che comunico con il dispositivo
// tramite stringhe di caratteri che identificano gli oggetti con id e nome,
// posso utilizzare un solo oggetto per tutti gli oggetti uguali 		 	
//--------------------------------------------------------------------------

// oggetto relativo alla temperatura mostrata sul display
static NexText			TempText = NexText(HOMEID, T_TEXTID, T_TEXTNAME);

// oggetto relativo alla rappresentazione grafica della temperatura
static NexProgressBar	TempPro = NexProgressBar(HOMEID, T_PROGID, T_PROGNAME);

// oggetto relativo all'umidità mostrata sul display
static NexText			HumText = NexText(HOMEID, H_TEXTID, H_TEXTNAME);

// oggetto relativo alla rappresentazione grafica dell'umidità
static NexGauge			HumGauge = NexGauge(HOMEID, H_GAUGEID, H_GAUGENAME);

//--------------------------------------------------------------------------
// contiene l'indirizzo di tutti gli oggetti di tipo Touch ai quali			
// è associato un evento
//--------------------------------------------------------------------------
// per gli oggetti comuni utilizziamo degli array, ma per poter definire 	
// l'id dell'elemento, dobbiamo utilizzare un offset perchè la lockpage 
// non ha oggetti in comune
//--------------------------------------------------------------------------
static NexTouch	*nex_listen_list[] = {
	&LockSlider,

	&HomeButt[HOMEID - OFFSET_PAGE],		// (HomeId - OffsetPage) = 0
	&HomeButt[SETTID - OFFSET_PAGE],		// (SettId - OffsetPage) = 1
	&HomeButt[LOGID - OFFSET_PAGE],			// (LogId - OffsetPage) = 2

	&SettButt[HOMEID - OFFSET_PAGE],
	&SettButt[SETTID - OFFSET_PAGE],
	&SettButt[LOGID - OFFSET_PAGE],

	&LogButt[HOMEID - OFFSET_PAGE],
	&LogButt[SETTID - OFFSET_PAGE],
	&LogButt[LOGID - OFFSET_PAGE],

	&ThermoButt[HOMEID - OFFSET_PAGE],
	&ThermoButt[SETTID - OFFSET_PAGE],

	&LightButt[HOMEID - OFFSET_PAGE],
	&LightButt[SETTID - OFFSET_PAGE],

	&ThermoAutoButt,
	&LightAutoButt,
	&TempPlusButt,
	&TempMinusButt,
	&LockButton,
	NULL
};


//--------------------------------------------------------------------------
// funzione invocata in seguito al presentarsi dell'evento relativo al lock 
// cambia pagina e invia il comando per cambiare pagina, Invoca la Lock 	
//--------------------------------------------------------------------------
void LockButt_Callback(void *ptr) {
	CurrentPage = LOCKID;
	sendCommand(GOTO_LOCKPAGE);
	System_Lock();
}

//--------------------------------------------------------------------------
// funzione invocata quando si presenta un evento di Unlock, slider della 	
// LockPage, passa alla pagina HOME e mette il sistema in stato Unlock 		
//--------------------------------------------------------------------------
void LockSlider_Callback(void *ptr) { //onRelease
	uint32_t	val;

	LockSlider.getValue(&val);
	if (val >= 90) {
		LockSlider.setValue(0);
		CurrentPage = HOMEID;
		sendCommand(GOTO_HOMEPAGE);
		System_Unlock();
	}
	else {
		LockSlider.setValue(0);
	}
}

//--------------------------------------------------------------------------
// evento associato al bottone per passare nella HomePage, cambia pagina	
//--------------------------------------------------------------------------
void HomeButt_Callback(void *ptr) {
	if (CurrentPage != HOMEID) {
		CurrentPage = HOMEID;
		sendCommand(GOTO_HOMEPAGE);
	}
}

//--------------------------------------------------------------------------
// evento associato al bottone per passare nella SettingsPage, cambia pagina
//--------------------------------------------------------------------------
void SettButt_Callback(void *ptr) {
	if (CurrentPage != SETTID) {
		CurrentPage = SETTID;
		sendCommand(GOTO_SETTPAGE);
	}
}

void RemoveMessage();

//--------------------------------------------------------------------------
// evento associato al bottone per passare nella LogPage, cambia pagina		
// se mi trovo già nella LogPage provvede ad eliminare il primo messaggio	
//--------------------------------------------------------------------------
void LogButt_Callback(void *ptr) {
	if (CurrentPage != LOGID) {
		CurrentPage = LOGID;
		sendCommand(GOTO_LOGPAGE);
	}
	else {
		RemoveMessage();
	}
}

//--------------------------------------------------------------------------
// evento associato al bottone del sistema di Riscaldamento 				
//--------------------------------------------------------------------------
void ThermoButt_Callback(void *ptr) {
	uint32_t	on;

	ThermoButt[CurrentPage - OFFSET_PAGE].getValue(&on);
	turnThermo((int)on);
}

//--------------------------------------------------------------------------
// evento associato al bottone del sistema d'Illuminazione					
//--------------------------------------------------------------------------
void LightButt_Callback(void *ptr) {
	uint32_t	on;

	LightButt[CurrentPage - OFFSET_PAGE].getValue(&on);
	turnLight((int)on);
}

//--------------------------------------------------------------------------
// evento associato al bottone per il Riscaldamento Automatico				
//--------------------------------------------------------------------------
void ThermoAuto_Callback(void *ptr) {
	uint32_t	on;

	ThermoAutoButt.getValue(&on);
	SetThermoAuto((int)on);
}

//--------------------------------------------------------------------------
// evento associato al bottone per illuminazione automatica					
//--------------------------------------------------------------------------
void LightAuto_Callback(void *ptr) {
	uint32_t	on;

	LightAutoButt.getValue(&on);
	SetLightAuto((int)on);
}

//--------------------------------------------------------------------------
// evento associato al bottone per incrementare la temperatura				
//--------------------------------------------------------------------------
void TempPlus_Callback(void *ptr) {
	IncDesiredTemp();
}

//--------------------------------------------------------------------------
// evento associato al bottone per decrementare la temperatura				
//--------------------------------------------------------------------------
void TempMinus_Callback(void *ptr) {
	DecDesiredTemp();
}

void setThermoState(int i, bool v) {
	ThermoButt[i].setValue(v);
}

void setLightState(int i, bool v) {
	LightButt[i].setValue(v);
}

void setThermoAuto(bool v) {
	ThermoAutoButt.setValue(v);
}

void setLightAuto(bool v) {
	LightAutoButt.setValue(v);
}

//--------------------------------------------------------------------------
// elimina il messaggio più vecchio sul display e fa la rotazione 			
//--------------------------------------------------------------------------
void ScrollTextUp() {
	if (inDisplay > 0) {
		inDisplay--;
		strcpy(DisplayBuffer[0], DisplayBuffer[1]);
		strcpy(DisplayBuffer[1], DisplayBuffer[2]);
		strcpy(DisplayBuffer[2], DisplayBuffer[3]);
		memset(DisplayBuffer[3], '\0', LINE_LEN);
	}
}

//--------------------------------------------------------------------------
// elimina il messaggio più vecchio sul display e se presenti messaggi da 	
// rappresentare ne inserisce uno nell'ultima riga del display 				
//--------------------------------------------------------------------------
void RemoveMessage() {
	DisplayBufferChanged = true;
	ScrollTextUp();

	if (toread > 0) {
		strcpy(DisplayBuffer[inDisplay], BufferLine[front]);
		inDisplay++;
		memset(BufferLine[front], '\0', LINE_LEN);
		front = ((front + 1) % NUM_BUFFER_LINE);
		toread--;
	}
}

//--------------------------------------------------------------------------
// inserisce il messaggio nel buffer circolare, se c'è spazio 				
//--------------------------------------------------------------------------
void PushInBuffer(const char str[]) {
	if (toread < NUM_BUFFER_LINE) {
		memset(BufferLine[rear], '\0', LINE_LEN);
		strcpy(BufferLine[rear], str);
		rear = ((rear + 1) % NUM_BUFFER_LINE);
		toread++;
	}
}

//--------------------------------------------------------------------------
// Se è stata rilevata un'intrusione, ed è la prima, inserisce il messaggio 
// relativo nel display, se questo è pieno nel buffer circolare 			
//--------------------------------------------------------------------------
void InsertMessageIntrusion(bool alarmOn) {
	if (alarmOn) {
		if (firstIntrusion) {
			firstIntrusion = false;
			if (inDisplay < NUM_LOG_LINE) {
				memset(DisplayBuffer[inDisplay], '\0', LINE_LEN);
				strcpy(DisplayBuffer[inDisplay], TEXT_INTRUSION);
				inDisplay++;
				DisplayBufferChanged = true;
			}
			else {
				PushInBuffer(TEXT_INTRUSION);
			}
		}
	}
	else {
		firstIntrusion = true;
	}
}

//--------------------------------------------------------------------------
// Se è stata rilevata presenza di gas, inserisce il messaggio relativo 	
// nel display, se questo è pieno nel buffer circolare 						
//--------------------------------------------------------------------------
void InsertMessageGasAlarm(bool alarmOn) {
	if (alarmOn) {
		if (firstGasAlarm) {
			firstGasAlarm = false;
			if (inDisplay < NUM_LOG_LINE) {
				memset(DisplayBuffer[inDisplay], '\0', LINE_LEN);
				strcpy(DisplayBuffer[inDisplay], TEXT_GAS_ALARM);
				inDisplay++;
				DisplayBufferChanged = true;
			}
			else {
				PushInBuffer(TEXT_GAS_ALARM);
			}
		}
	}
	else {
		firstGasAlarm = true;
	}
}

//--------------------------------------------------------------------------
// aggiorna la temperatura sul display e la progress bar in percentuale		
// rispetto il range definito dal sistema 									
//--------------------------------------------------------------------------
void UpdateTemperature(float t) {
	int		len=6;				// 99.9°
	char	str[len];

	if ((t != old_temp) || (CurrentPage != old_page)) {
		old_temp = t;
		memset(str, '\0', len);
		if ((t - (int)t) == 0) {
			snprintf(str, len, "%i", (int)t);
			if ((int)t > 9)			//	9°
				str[2]='°';
			else
				str[1]='°';			//	99°
		}
		else {				// 0.5
			snprintf(str, len, "%.1f", t);
			if ((int)t > 9)			//	9.5°
				str[4]='°';
			else
				str[3]='°';			//	99.5°
		}

		TempText.setText(str);
		t = (t / (MAX_TEMP - MIN_TEMP)) * 100;	//in percentual of range
		TempPro.setValue((uint32_t)t);
	}
}

//--------------------------------------------------------------------------
// aggiorna l'umidità sul display e l'icona relativa 
//--------------------------------------------------------------------------
void UpdateHumidity(int hum) {
	int		len=4;				//	99%
	char	str[len];

	if ((hum != old_hum) || (CurrentPage != old_page)) {
		old_hum = hum;
		memset(str, '\0', len);
		snprintf(str, len, "%2i%%", hum);
		HumText.setText(str);
		HumGauge.setValue(hum);
	}
}

//--------------------------------------------------------------------------
// Aggiorna le informazioni presenti nella HomePage 
//--------------------------------------------------------------------------
void UpdateHomaPage(System_Status cpySS) {
	setThermoState((HOMEID - OFFSET_PAGE), cpySS.ThermoOn);
	setLightState((HOMEID - OFFSET_PAGE), cpySS.LightOn);
	UpdateTemperature(cpySS.SensorsValue.Temperature);
	UpdateHumidity(cpySS.SensorsValue.Humidity);
}

//--------------------------------------------------------------------------
// Aggiorna le informazioni presenti nella SettingsPage	
//--------------------------------------------------------------------------
void UpdateSettings(System_Status cpySS) {
	setThermoState((SETTID - OFFSET_PAGE), cpySS.ThermoOn);
	setLightState((SETTID - OFFSET_PAGE), cpySS.LightOn);
	setThermoAuto(cpySS.ThermoAuto);
	setLightAuto(cpySS.LightAuto);
	UpdateTemperature(cpySS.desired_temp);
	UpdateHumidity(cpySS.SensorsValue.Humidity);
}

//--------------------------------------------------------------------------
// Aggiorna le informazioni presenti nella LogPage 
//--------------------------------------------------------------------------
void UpdateLogPage() {
	if (DisplayBufferChanged || (CurrentPage != old_page)) {
		DisplayBufferChanged = false;
		LogTextL1.setText(DisplayBuffer[0]);
		LogTextL2.setText(DisplayBuffer[1]);
		LogTextL3.setText(DisplayBuffer[2]);
		LogTextL4.setText(DisplayBuffer[3]);
	}
}

//--------------------------------------------------------------------------
// Aggiorna le informazioni presenti sul display, per la pagina in cui si 
// trova e controlla se sono presenti messaggi da rappresentare, 
// nella lockPage controlla solo i messaggi da rappresentare 
//--------------------------------------------------------------------------
void UpdateDisplay() {
	System_Status	cpySS;
	cpySS = getSystemCpy();

	switch (CurrentPage) {
		case LOCKID:
			InsertMessageIntrusion(cpySS.IntrusionAlarmOn);
			InsertMessageGasAlarm(cpySS.GasAlarmOn);
			break;

		case HOMEID:
			UpdateHomaPage(cpySS);
			InsertMessageGasAlarm(cpySS.GasAlarmOn);
			break;

		case SETTID:
			UpdateSettings(cpySS);
			InsertMessageGasAlarm(cpySS.GasAlarmOn);
			break;

		case LOGID:
			UpdateLogPage();
			InsertMessageGasAlarm(cpySS.GasAlarmOn);
			break;

		default:
			break;
	}
	old_page = CurrentPage;
}

//--------------------------------------------------------------------------
// funzione per cambiare pagina ed andare nella HomePage, non è un evento
//--------------------------------------------------------------------------
void DisplayShowHomePage() {
	CurrentPage = HOMEID;
	sendCommand(GOTO_HOMEPAGE);
	UpdateDisplay();
}

void DisplayShowLockPage() {
	CurrentPage = LOCKID;
	sendCommand(GOTO_LOCKPAGE);
	UpdateDisplay();
}

//--------------------------------------------------------------------------
// Inizializzazione del display, ad ogni oggetto di tipo Touch viene
// associata la funzione relativa al suo evento 
// il sistema si avvia con la pagina di lock 
//--------------------------------------------------------------------------
void InitDisplay() {
	int		i;

	if (nexInit()) {
		LockButton.attachPop(LockButt_Callback);
		LockSlider.attachPop(LockSlider_Callback);
	
		for (i = 0; i<3; i++) {		// 	HOMEPAGE, SETTINGS PAGE, LOCK PAGE
			HomeButt[i].attachPop(HomeButt_Callback);
			SettButt[i].attachPop(SettButt_Callback);
			LogButt[i].attachPop(LogButt_Callback);
	
			if (i<2) {				// ONLY HOME PAGE and SETTINGS PAGE
				ThermoButt[i].attachPop(ThermoButt_Callback);
				LightButt[i].attachPop(LightButt_Callback);
			}
		}
	
		ThermoAutoButt.attachPop(ThermoAuto_Callback);
		LightAutoButt.attachPop(LightAuto_Callback);
		TempPlusButt.attachPop(TempPlus_Callback);
		TempMinusButt.attachPop(TempMinus_Callback);

		sendCommand(RETURN_MSG_CMD);

		CurrentPage = LOCKID;
		sendCommand(GOTO_LOCKPAGE);
		UpdateDisplay();

		memset(DisplayBuffer, '\0', (NUM_LOG_LINE * LINE_LEN));
		memset(BufferLine, '\0', (NUM_BUFFER_LINE * LINE_LEN));
	}
	else {
		#ifdef DEBUG_DISPLAY
		Serial.println("Nextion Initialization problem ");
		#endif
	}
}

//--------------------------------------------------------------------------
// funzione invocata dal loop, controlla se ci sono eventi e se presenti 	
// invoca la funzione associata all'evento.
// Aggiorna  le informazioni relative alla pagina in cui si trova 
//--------------------------------------------------------------------------
void DisplayIteration() {
	nexLoop(nex_listen_list);
	UpdateDisplay();
}