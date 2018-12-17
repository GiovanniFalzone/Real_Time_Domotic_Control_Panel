//--------------------------------------------------------------------------
// Questa libreria realizza la parte relativa al Sistema Centrale
// per semplificare lo sketch principale ho inserito in questa le operazioni
// relative all'hardware per sensori ed attuatori		
// invocando gli opportuni metodi sotto descritti il sistema aggiorna il suo
// stato, i parametri di funzionamento sono definiti tramite #define	
// nel cambiare i parametri relativi ai periodi dei task controllare che 
// questi siano uguali a quelli dello sketch, problema del parser per i loop
//--------------------------------------------------------------------------

#ifndef MYSYSTEM_H
#define MYSYSTEM_H

#define DEBUG_MYSYSTEM

#define MAX_TASKS				5		// numero totale di task

#define PERIOD_TASK_T1			350		// loop1 Task System1
#define PERIOD_TASK_T2			4000	// loop2 Task System2
#define PERIOD_TASK_T3			350		// loop3 Task Control Panel
#define PERIOD_TASK_T4			4000	// loop4 Task WebServer
#define PERIOD_TASK_T5			500		// loop5 Task WebSocket Interface

//------------------Hardware pin---------------------------------------------
#define SENSOR_DHT_TYPE			DHT22
#define SENSOR_DHT22_PIN		48		// pin sensore DHT

#define SENSOR_MQ4_PIN_D		46		// pin digitale sensore gas
#define SENSOR_MQ4_PIN_A		A0		// pin analogico sensore gas

#define SENSOR_PIR_PIN			44		// pin sensore movimento
#define ALARM_LED_PIN			7		// pin allarme

#define RELE_PIN_1				50		// pin collegato al rele 1
#define RELE_PIN_2				51		// ...	2
#define RELE_PIN_3				52		// ...	3
#define RELE_PIN_4				53		// ...	4

#define SD_CS					4		// SD SPI Slave Selector
#define ETH_CS					10		// ETH SPI Slave Selector

#define LIGH_PIN				RELE_PIN_1		// luci rele 1
#define THERMO_PIN				RELE_PIN_2		// riscaldamento rele 2
#define LOCK_PIN				RELE_PIN_3		// serratura rele 3
#define ALARM_PIN				RELE_PIN_4		// allarme rele 4

#define SYSSTATUSFILENAME		"Sysbck.csv\0"			// salvataggio sistema

//-------------------Soglie e parametri di Default---------------------------
#define GAS_THREASHOLD			250			// soglia per allarme gas

#define INCDEC_LUX_UNIT			10			// unità luminosità
#define MAX_DESIRED_LUX			500			// massima luminosità
#define MIN_DESIRED_LUX			50			// minima luminosità
#define LUX_ERROR				10			// per evitare oscillazioni

#define INCDEC_TEMP_UNIT		0.25		// unità temperatura
#define MAX_DESIRED_TEMP		40			// massima temperatura desiderata
#define MIN_DESIRED_TEMP		10			// minima temperatura desiderata
#define TEMP_ERROR				0.25		// per evitare oscillazioni

#define DEFAULT_DESIRED_LUX		100			// luminosità di default
#define DEFAULT_DESIRED_TEMP	22			// temperatura di default

#define PIR_UP_TIME				5000		// durata del segnale

// numero minimo di rilevamenti per far scattare l'allarme intrusione
#define PIR_RATIO 				(PIR_UP_TIME / PERIOD_TASK_T2)
#define MIN_INTRUSION			(2 * PIR_RATIO)

// numero minimo di rilevamenti per far scattare l'allarme gas
#define MIN_GASALARM			4

// Struttura dati Sistema
typedef struct {
	int		Period;						//	periodo del task
	int		Start_execution;			//	Inizio esecuzione
	int		Deadline;					//	prossima deadline
	int		lastActivation;				//	ultima Attivazione
	int		Deadline_miss;				//	numero di deadline miss
	int		max_execution_time;			//	massimo tempo di esecuzione
	int		ExecutionTime;				//	ultimo tempo di esecuziione
	int		sumExecTime;				//	somma tempi di esecuzione
	int		n;							//	numero di job
	float	AverageExecutionTime;		//	media tempi di esecuzione
	int		FirstActivation;
} RT_Task;


//Valori letti dai sensori e salvati tramite i metodi in fondo
typedef struct {
	float	Temperature;			//	ultima temperatura
	float	Humidity;				//	ultima umidità
	int		Gas;					//	ultima presenza di gas
	int		Luminosity;				//	ultimo valore luminosità
	bool	Motion;					//	ultima presenza di movimento
} SensorsData;

//--------------------------------------------------------------------------
//Struttura relativa allo stato del sistema, mantiene informazioni			
//riguardo i valori dei sensori,dei vari parametri definiti dall'utente		
//e dello stato attuale interno, viene modificato tramite i metodi in fondo 
//--------------------------------------------------------------------------
typedef struct {
	SensorsData	SensorsValue;
	int			desired_lux;			//	luminosità minima
	float		desired_temp;			//	temperatura minima
	bool		LightOn;				//	stato illuminazione
	bool		LightAuto;				//	Luci Automatiche
	bool		ThermoOn;				//	stato Riscaldamento
	bool		ThermoAuto;				//	Riscaldamento Automatico
	bool		AlarmOn;				//	Stato allarme
	bool		Locked;					//	Sistema Lock/Unlocl
	bool		IntrusionAlarmOn;		//	Allarme intrusione
	bool		GasAlarmOn;				//	Allarme Rilevazione gas
	int			n_intrusion;			//	contatore intrusioni
	int			n_gasalarm;				//	contatore rilevazioni gas
} System_Status;

//--------------------------------------------------------------------------
// legge il valore del Sensore PIR e lo salva nell'apposita variabile		
// nella struttura relativa ai sensori che si trova nella struttura Sistema	
//--------------------------------------------------------------------------
void ReadAndSetMotion();

//--------------------------------------------------------------------------
// legge il valore del Sensore Luminosità e lo salva nell'apposita variabile
// nella struttura relativa ai sensori che si trova nella struttura Sistema	
//--------------------------------------------------------------------------
void ReadAndSetLuminosity();

//--------------------------------------------------------------------------
// legge il valore del Sensore MQ-4(Gas) e lo salva nell'apposita variabile	
// nella struttura relativa ai sensori che si trova nella struttura Sistema 
//--------------------------------------------------------------------------
void ReadAndSetGas();

//--------------------------------------------------------------------------
// legge il valore del Sensore DHT22( e lo salva nell'apposita variabile	
// nella struttura relativa ai sensori che si trova nella struttura Sistema	
//--------------------------------------------------------------------------
void ReadAndSetTemperatureAndHumidity();

//--------------------------------------------------------------------------
// Se il sistema è in lock, controlla la presenza di movimento, se superiore
// a una soglia stabilita a inizio H, viene invocato l'Allarme				
//--------------------------------------------------------------------------
void CheckIntrusion();

//--------------------------------------------------------------------------
// controlla la presenza di Gas/Fumi, se superiore a una soglia stabilita, o
// se il Pin Digitale del sensore è a livello Alto, viene invocato l'allarme
//--------------------------------------------------------------------------
void CheckGas();

//--------------------------------------------------------------------------
//inizializza ogni pin e il suo stato iniziale								
//--------------------------------------------------------------------------
void Init_Rele_Shield();

//--------------------------------------------------------------------------
//inizializza l'SD se possibile, altrimenti torna False come errore			
//--------------------------------------------------------------------------
bool Init_SD();

//Inizializza tutti i sensori
void InitSensors();

//--------------------------------------------------------------------------
// Se presente il file di Stato, Ripristina lo stato del sistema			
// tramite l'ultimo stato salvato											
//--------------------------------------------------------------------------
void RestoreSystemStatus();

//--------------------------------------------------------------------------
// Salva lo stato del sistema, se il file non eseiste viene creato			
// se esiste viene sostituito												
//--------------------------------------------------------------------------
void StorageSystem();

//--------------------------------------------------------------------------
// porta lo stato del Riscaldamento ad on/off, modifica il pin				
// e la variabile di sistema relativa allo stato del Riscaldamento			
//--------------------------------------------------------------------------
void turnThermo(int on);

//--------------------------------------------------------------------------
// inverte lo stato del sistema di riscaldamento							
//--------------------------------------------------------------------------
void SwitchThermoAuto();

//--------------------------------------------------------------------------
// attiva la modalita Automatica del Riscaldamento							
//--------------------------------------------------------------------------
void SetThermoAuto(int on);

//--------------------------------------------------------------------------
// Incrementa/Decrementa la temperatura desiderata della relativa unità,	
// la nuova scelta viene mantenuta nella struttura sistema					
//--------------------------------------------------------------------------
void IncDesiredTemp();

void DecDesiredTemp();

//--------------------------------------------------------------------------
// Se attivo gestisce il Riscaldamento in modo autonomo, se la temperatura	
// è inferiore a quella desiderata, accende il riscaldamento				
// se viceversa è superiore(di una certa soglia), lo spegne					
//--------------------------------------------------------------------------
void ManageThermoAuto();

//--------------------------------------------------------------------------
// porta lo stato delle luci ad on/off, modifica il pin relativo			
// e la variabile di sistema relativa allo stato delle luci					
//--------------------------------------------------------------------------
void turnLight(int on);

//--------------------------------------------------------------------------
// inverte lo stato del sistema d'Illuminazione								
//--------------------------------------------------------------------------
void SwitchLightAuto();

//--------------------------------------------------------------------------
// attiva la modalita Automatica del sistema d'Illuminazione				
//--------------------------------------------------------------------------
void SetLightAuto(int on);

//--------------------------------------------------------------------------
// Incrementa/Decrementa la Luminosità desiderata di un valore predefinito	
// la nuova scelta viene mantenuta nella struttura sistema					
//--------------------------------------------------------------------------
void IncDesiredLuminosity();

void DecDesiredLuminosity();

//--------------------------------------------------------------------------
// Se attivo gestisce il sistema di Illuminazione in modo autonomo,			
// se la Luminosità è inferiore(di una certa soglia) a quella desiderata,	
// accende il sistema di illuminazione, se viceversa è superiore, lo spegne	
//--------------------------------------------------------------------------
void ManageLightAuto();

//--------------------------------------------------------------------------
// Mette in Lock il sistema, Salva lo stato del Sistema						
// e spegne il Sistema di Riscaldamento e d'Illuminazione					
//--------------------------------------------------------------------------
void System_Lock();

//--------------------------------------------------------------------------
// Mette in Lock il sistema, Ripristina lo stato salvato tramite la Lock	
// ripristina lo stato del Sistema di Riscaldamento e d'Illuminazione		
//--------------------------------------------------------------------------
void System_Unlock();

//ritorna lo stato del Sistema
bool SystemLocked();

//--------------------------------------------------------------------------
// ritorna una copia dello stato del sistema								
//--------------------------------------------------------------------------
System_Status getSystemCpy();

//--------------------------------------------------------------------------
// ritorna una copia del tasl i-esimo										
//--------------------------------------------------------------------------
RT_Task getTaskCpy(int i);

//--------------------------------------------------------------------------
//Inizializza le strutture relative ai tasks
//--------------------------------------------------------------------------
void Init_Tasks();

//--------------------------------------------------------------------------
//inizio di ogni esecuzione
//--------------------------------------------------------------------------
void Start_Execution(const int i);

//--------------------------------------------------------------------------
//fine di ogni esecuzione
//--------------------------------------------------------------------------
void End_Execution(const int i);

//--------------------------------------------------------------------------
//Print su seriale di Execution Tive ed Average Execution Time 				
//--------------------------------------------------------------------------
void PrintlnExecTimeAndAvgExecTime(const int i);

#endif