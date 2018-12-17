#include "MySystem.h"

#include "arte.h"

#include <Wire.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2591.h>
#include <DHT.h>
#include <DHT_U.h>

//per poter accedere tramite id
const int	RT_Task_Periods[MAX_TASKS] = {
								PERIOD_TASK_T1,
								PERIOD_TASK_T2,
								PERIOD_TASK_T3,
								PERIOD_TASK_T4,
								PERIOD_TASK_T5
};

static RT_Task	myTasks[MAX_TASKS];

// per definire se lo stato del sistema deve essere salvato
static bool	to_be_saved = false;

// sensore DHT22, Temperatura e Umidità
static DHT_Unified	dht(SENSOR_DHT22_PIN, SENSOR_DHT_TYPE);

// Sensore TSL2591 Luminosità
static Adafruit_TSL2591	tsl = Adafruit_TSL2591(2591);

// inizializzazione dello stato del sistema
static System_Status	MySystem = {
									{0, 0, 0, 0, 0},
									DEFAULT_DESIRED_LUX,
									DEFAULT_DESIRED_TEMP,
									0, 0, 0, 0, 0, 0, 0, 0
};

//-----------------------------Hardware Operations---------------------------
void set_Rele(int pin, int value) {
	digitalWrite(pin, !value);
}

//Inizializza il pin per ogni rele, e li setta come spenti
void Init_Rele_Shield() {
	pinMode(RELE_PIN_1, OUTPUT);
	pinMode(RELE_PIN_2, OUTPUT);
	pinMode(RELE_PIN_3, OUTPUT);
	pinMode(RELE_PIN_4, OUTPUT);

	set_Rele(RELE_PIN_1, LOW);
	set_Rele(RELE_PIN_2, LOW);
	set_Rele(RELE_PIN_3, LOW);
	set_Rele(RELE_PIN_4, LOW);
}

bool Init_SD() {
	if (!SD.begin(SD_CS)) {
		#ifdef DEBUG_MYSYSTEM
		Serial.println("SD problem in Setup initialization");
		#endif
	}
}

void InitSensor_PIR() {
	pinMode(SENSOR_PIR_PIN, INPUT);
	pinMode(ALARM_LED_PIN, OUTPUT);
	digitalWrite(ALARM_LED_PIN, LOW);
}

bool Read_Pir() {
	return digitalRead(SENSOR_PIR_PIN);
}

void InitSensor_MQ4() {
	pinMode(SENSOR_MQ4_PIN_D, INPUT);
}

int Read_Gas() {
	return analogRead(SENSOR_MQ4_PIN_A);
}

void InitSensor_DHT22() {
	sensor_t	sensor;
	dht.begin();
	dht.temperature().getSensor(&sensor);
	dht.humidity().getSensor(&sensor);
}

//--------------------------------------------------------------------------
// le letture di temperatura ed umidità possono non andare a buon fine		
// perchè comunicano tramite protocollo Seriale Proprietario con il Sensore	
// se si presenta un interrupt il dato non sarà valido						
//--------------------------------------------------------------------------
float Read_Humidity() {
	sensors_event_t	event;

	dht.humidity().getEvent(&event);
	if (isnan(event.relative_humidity)) {
		#ifdef DEBUG_MYSYSTEM
		Serial.println("Error reading humidity!");
		#endif
		return -1;
	}
	else {
		return event.relative_humidity;
	}
}

float Read_Temperature() {
	sensors_event_t	event;  

	dht.temperature().getEvent(&event);
	if (isnan(event.temperature)) {
		#ifdef DEBUG_MYSYSTEM
		Serial.println("Error reading temperature!");
		#endif
		return -1;
	}
	else {
		return event.temperature;
	}
}

void InitSensor_TSL2591() {
	tsl2591Gain_t	gain;

	tsl.setGain(TSL2591_GAIN_MED);
	tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);
	gain = tsl.getGain();

	if (!tsl.begin()) {
		#ifdef DEBUG_MYSYSTEM
		Serial.println("Problem in TSL2591 sensor Initialization!");
		#endif
	}
}

int Read_Luminosity() {
	return tsl.getLuminosity(TSL2591_VISIBLE);
}

void InitSensors() {
	InitSensor_DHT22();  
	InitSensor_TSL2591();
	InitSensor_PIR();
	InitSensor_MQ4();
}

//---------------------End of Hardware Operations---------------------------

//---------------------System Operations------------------------------------

// verifica l'esistenza del Path, se non esiste viene creato
void TestDir(char path[]) {
	arteLock();

	if (!SD.exists(path)) {
		SD.mkdir(path);
	}

	arteUnlock();
}

//--------------------------------------------------------------------------
// in caso di problemi critici invocare halt, essendo in una sezione critica
// nessun altro task potrà eseguire va usata solo nella fase di debug,
// per il fatto che alcune librerie non sono adatte per poter essere
// utilizzate con ARTe, vedi le problematiche con SD
//--------------------------------------------------------------------------
void HALT() {
	arteLock();

	#ifdef DEBUG_MYSYSTEM

	Serial.print("Oh Shit a Critical error Occurred");
	Serial.println("I'll go in Cyclic Active Waiting to prevent Damage ");

	while (true)
		;

	#endif
	arteUnlock();
}

//--------------------------------------------------------------------------
// Apre il file e scrive una nuova riga in append							
// deve essere eseguita come sezione critica perchè la libreria SD, utilizza
// il pin SPI SS, lasciandolo attivo, se un task a più alta priorità esegue	
// ed utilizza SPI, allora due Dispositivi SPI saranno attivi insieme		
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Questo file Deve esistere, se non riesce ad aprirlo è stato corrotto		
//--------------------------------------------------------------------------
void PrintLnInFile(char fname[], char value[]) {
	File	f;
	arteLock();

	f = SD.open(fname, FILE_WRITE);
	if (f) {
		f.println(value);
		f.close();
	}
	else {
		HALT();
	}

	arteUnlock();
}

//--------------------------------------------------------------------------
// legge e ritorna un char da un file csv gia aperto,
// si ferma alla virgola, o al fine riga
//--------------------------------------------------------------------------
char CSVreadChar(File f) {
	char	ret, c;

	if (!f) return '!';

	c = 'y';
	while (f.available() && ((c != ',') && (c != '\r') && (c != '\n'))) {
		c = f.read();
		if ((c != ',') && (c != '\r') && (c != '\n')) {
			ret = c;
		}
	}
	return ret;
}

//--------------------------------------------------------------------------
// legge un booleano da file csv, il file deve essere già aperto
//--------------------------------------------------------------------------
bool CSVreadBool(File f) {
	char	c;

	c = CSVreadChar(f);
	return c - '0';
}

//--------------------------------------------------------------------------
// legge un Intero a 3 cifre da file csv, il file deve essere già aperto
//--------------------------------------------------------------------------
int CSVreadInt3D(File f) {
	int	i = 0;
	char c;
	char tmp[3];

	if (!f) return -1;

	c = 'y';
	while (f.available() && (c != ',' && c != '\r' && c != '\n')) {
		c = f.read();
		if ((c != ',' && c != '\r' && c != '\n') && (i<3)) {
			tmp[i] = c;
			i++;
		}
	}
	return atoi(tmp);
}

//--------------------------------------------------------------------------
// legge un Float a 4 cifre da file csv, il file deve essere già aperto	
//--------------------------------------------------------------------------
float CSVreadFloat4D(File f) {		// file già apero
	int		i = 0;
	char	c, tmp[5];		//5 digit 99.99
		
	if (!f) return -1;

	c = 'y';
	while (f.available() && ((c != ',') && (c != '\r') && (c != '\n'))) {
		c = f.read();
		if ((c != ',' && c != '\r' && c != '\n') && (i<5)) {
			tmp[i] = c;
			i++;
		}
	}
	return atof(tmp);
}

// scrive un boolean in formato csv in un file fià aperto
void WriteBoolCsv(File f, bool value) {
	f.print(value);
	f.print(',');
}

// scrive una stringa in formato csv in un file fià aperto
void WriteStrCsv(File f, char str[]) {
	f.print(str);
	f.print(',');
}

//--------------------------------------------------------------------------
// Salva lo stato del sistema, di questo salva soltando alcuni parametri
// quali le preferenze dell'utente, e lo stato delle funzionalità del 
// sistema, salva inoltre lo stato di lock per evitare violazioni
// è un'operazione critica per i problemi dovuti dalla libreria SD
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Questo file Deve esistere, se non riesce ad aprirlo è stato corrotto		
//--------------------------------------------------------------------------
void StoreSystemStatus() {
	char			tmp_lux[4], tmp_temp[6];
	File			dataFile;
	System_Status	Sysbck;

	if (to_be_saved) {
		to_be_saved = false;
		Sysbck = getSystemCpy();

		arteLock();

		if (SD.exists(SYSSTATUSFILENAME))
			SD.remove(SYSSTATUSFILENAME);

		dataFile = SD.open(SYSSTATUSFILENAME, FILE_WRITE);
		if (dataFile) {
			memset(tmp_lux, '\0', 4);
			memset(tmp_temp, '\0', 6);

			snprintf(tmp_lux, 4, "%03d", Sysbck.desired_lux);
			WriteStrCsv(dataFile, tmp_lux);
		
			snprintf(tmp_temp, 6, "%04.2f", Sysbck.desired_temp);
			WriteStrCsv(dataFile, tmp_temp);

			WriteBoolCsv(dataFile, Sysbck.LightOn);
			WriteBoolCsv(dataFile, Sysbck.LightAuto);
			WriteBoolCsv(dataFile, Sysbck.ThermoOn);
			WriteBoolCsv(dataFile, Sysbck.ThermoAuto);
			WriteBoolCsv(dataFile, Sysbck.AlarmOn);
			WriteBoolCsv(dataFile, Sysbck.Locked);
			WriteBoolCsv(dataFile, Sysbck.IntrusionAlarmOn);
			WriteBoolCsv(dataFile, Sysbck.GasAlarmOn);

		    dataFile.close();
		}
		else {
			HALT();
		}

		arteUnlock();
	}
}


void SetDesiredTemp(float t);
void SetDesiredLuminosity(int l);
void turnAlarm(int on);

//--------------------------------------------------------------------------
// Se esiste uno stato precendemente salvato, Ripristina lo stato del		
// sistema è un'operazione critica per i problemi dovuti dalla libreria SD	
//--------------------------------------------------------------------------
void RestoreSystemStatus() {
	System_Status	Sysbck;
	File			dataFile;

	arteLock();

	if (SD.exists(SYSSTATUSFILENAME)) {
		dataFile = SD.open(SYSSTATUSFILENAME, FILE_READ);
		if (!dataFile) {
			HALT();
			return;
		}

		SetDesiredLuminosity(CSVreadInt3D(dataFile));
		SetDesiredTemp(CSVreadFloat4D(dataFile));
		turnLight(CSVreadBool(dataFile));
		SetLightAuto(CSVreadBool(dataFile));
		turnThermo(CSVreadBool(dataFile));
		SetThermoAuto(CSVreadBool(dataFile));
		turnAlarm(CSVreadBool(dataFile));

		if (CSVreadBool(dataFile))
			System_Lock();
		else 
			System_Unlock();

		MySystem.IntrusionAlarmOn = CSVreadBool(dataFile);
		MySystem.GasAlarmOn = CSVreadBool(dataFile);

		dataFile.close();
	}
	arteUnlock();
}

//--------------------------------------------------------------------------
// metodo invocato dal task che si occupa del salvataggio del sistema		
//--------------------------------------------------------------------------
void StorageSystem() {
	StoreSystemStatus();
}

//--------------------------------------------------------------------------
// Attiva/Disattiva l'allarme di sistema, è realizzata come sezione critica	
// per non portare il sistema in uno stato non consistente
//--------------------------------------------------------------------------
void turnAlarm(int on) {
	arteLock();

//per disattivarlo, le condizioni di non allarme devono essere verificate
	if (!on && (MySystem.IntrusionAlarmOn || MySystem.GasAlarmOn)) {
		arteUnlock();
		return;
	}

	if (MySystem.AlarmOn != on) {
		MySystem.AlarmOn = on;
		set_Rele(ALARM_PIN, on);
		digitalWrite(ALARM_LED_PIN , on);

		to_be_saved = true;
	}

	arteUnlock();
}

//--------------------------------------------------------------------------
// Attiva/Disattiva il Riscaldamento, se in modalità automatica, interpreto	
// una richiesta di attiva come aumenta la temperatura desiderata
// e disattiva come riduci la temperatura desiderata
// viene realizzata come sezione critica per non portare il sistema in uno
// stato non consistente
//--------------------------------------------------------------------------
void turnThermo(int on) {
	arteLock();

	if (!MySystem.ThermoAuto) {
		MySystem.ThermoOn = on;
		set_Rele(THERMO_PIN, on);

		to_be_saved = true;
	}
	else {
		if (on) {
			IncDesiredTemp();
		}
		else {
			DecDesiredTemp();
		}
	}

	arteUnlock();
}

void SwitchThermoAuto() {
	MySystem.ThermoAuto = !MySystem.ThermoAuto;
	to_be_saved = true;
}

void SetThermoAuto(int on) {
	MySystem.ThermoAuto = on;
	to_be_saved = true;
}

void SetDesiredTemp(float t) {
	if ((t >= MIN_DESIRED_TEMP) && (t <= MAX_DESIRED_TEMP))
		MySystem.desired_temp = t;

	to_be_saved = true;
}

//--------------------------------------------------------------------------
// incrementa la temperatura desiderata
// viene realizzata come sezione critica per non portare il sistema in uno
// stato non consistente
//--------------------------------------------------------------------------
void IncDesiredTemp() {
	arteLock();

	if ((MySystem.desired_temp + INCDEC_TEMP_UNIT) <= MAX_DESIRED_TEMP)
		MySystem.desired_temp += INCDEC_TEMP_UNIT;
	to_be_saved = true;

	arteUnlock();
}

void DecDesiredTemp() {
	arteLock();

	if ((MySystem.desired_temp - INCDEC_TEMP_UNIT) >= MIN_DESIRED_TEMP)
		MySystem.desired_temp -= INCDEC_TEMP_UNIT;
	to_be_saved = true;

	arteUnlock();
}

//--------------------------------------------------------------------------
// Gestisce il riscaldamento in modo automatico se il sistema non è in lock	
// altrimenti attiverebbe il riscaldamento quondo sono in lock-Status,
// utilizza un parametro di errore per evitare che oscillazioni della 
// temperatura prelevata dai sensori portino il sistema in una fase 
// di oscillazione tra spegni e accendi, potendo danneggiare il sistema		
// viene realizzata come sezione critica per non portare il sistema in uno
// stato non consistente
//--------------------------------------------------------------------------
void ManageThermoAuto() {
	float	tempnow, tempdes, error;

	arteLock();

	if (!SystemLocked()) {
		if (MySystem.ThermoAuto) {
			tempnow = MySystem.SensorsValue.Temperature;
			tempdes = MySystem.desired_temp;
			error = TEMP_ERROR;

			if (tempnow <= (tempdes - error)) {
				if (!MySystem.ThermoOn) {
					MySystem.ThermoOn = true;
					set_Rele(THERMO_PIN, HIGH);
					to_be_saved = true;
				}
			}
			else {
				if (tempnow >= (tempdes + error)) {
					if (MySystem.ThermoOn) {
						MySystem.ThermoOn = false;
						set_Rele(THERMO_PIN, LOW);
						to_be_saved = true;
					}
				}
			}
		}
	}

	arteUnlock();
}

//--------------------------------------------------------------------------
// Attiva o disattiva il sistema d'Illuminazione,
// viene realizzata come sezione critica per non portare il sistema in uno
// stato non consistente
//--------------------------------------------------------------------------
void turnLight(int on) {
	arteLock();

	if (!MySystem.LightAuto) {
		MySystem.LightOn = on;
		set_Rele(LIGH_PIN, on);
		to_be_saved = true;
	}
	else {
		if (on) {
			IncDesiredLuminosity();
		}
		else {
			DecDesiredLuminosity();
		}
	}

	arteUnlock();
}

void SwitchLightAuto() {
	MySystem.LightAuto = !MySystem.LightAuto;
	to_be_saved = true;

}

void SetLightAuto(int on) {
	MySystem.LightAuto = on;
	to_be_saved = true;
}

void SetDesiredLuminosity(int l) {
	if ((l >= MIN_DESIRED_LUX) && (l <= MAX_DESIRED_LUX))
		MySystem.desired_lux = l;

	to_be_saved = true;
}

//--------------------------------------------------------------------------
// Incrementa la luminosità minima desiderata, è una sezione critica
// evitare stati inconsistenti del sistema
//--------------------------------------------------------------------------
void IncDesiredLuminosity() {
	arteLock();

	if ((MySystem.desired_lux + INCDEC_LUX_UNIT) <= MAX_DESIRED_LUX)
		MySystem.desired_lux += INCDEC_LUX_UNIT;

	to_be_saved = true;

	arteUnlock();
}

void DecDesiredLuminosity() {
	arteLock();

	if ((MySystem.desired_lux - INCDEC_LUX_UNIT) >= MIN_DESIRED_LUX)
		MySystem.desired_lux -= INCDEC_LUX_UNIT;

	to_be_saved = true;

	arteUnlock();
}

//--------------------------------------------------------------------------
// Gestisce l'Illuminazione in modo automatico se il sistema non è in lock,	
// altrimenti attiverebbe l'Illuminazione quondo sono in lock-Status,
// utilizza un parametro di errore come nel caso di ManageThermoAuto 
// viene realizzata come sezione critica per non portare il sistema in uno
// stato non consistente
//--------------------------------------------------------------------------
void ManageLightAuto() {
	int	luxnow, luxdes, error;

	arteLock();

	if (!SystemLocked()) {
		if (MySystem.LightAuto) {
			error = LUX_ERROR;
			luxnow = MySystem.SensorsValue.Luminosity;
			luxdes = MySystem.desired_lux;

			if (luxnow <= (luxdes - error)) {
				if (!MySystem.LightOn) {
					MySystem.LightOn = true;
					set_Rele(LIGH_PIN, HIGH);
					to_be_saved = true;
				}
			}
			else {
				if (luxnow >= (luxdes + error)) {
					if (MySystem.LightOn) {
						MySystem.LightOn = false;
						set_Rele(LIGH_PIN, LOW);
						to_be_saved = true;
					}
				}
			}
		}
	}
	arteUnlock();
}

//--------------------------------------------------------------------------
// porta il sistema in stato di Lock, si salva lo stato del sistema
// e disattiva riscaldamento e luci
//--------------------------------------------------------------------------
void System_Lock() {
	arteLock();

	MySystem.Locked = true;
	set_Rele(LOCK_PIN, HIGH);
	to_be_saved = true;

	// turn off light and thermo
	set_Rele(THERMO_PIN, LOW);
	set_Rele(LIGH_PIN, LOW);

	arteUnlock();
}

//--------------------------------------------------------------------------
// porta il sistema in stato di Unlock, ripristina lo stato precedentemente	
// salvatto dalla Lock, riportando lo stato del sistema a prima del lock
//--------------------------------------------------------------------------
void System_Unlock() {
	arteLock();

	MySystem.Locked = false;
	set_Rele(LOCK_PIN, LOW);

	MySystem.n_intrusion = 0;
	MySystem.IntrusionAlarmOn = false;

	//restore Thermo and Light Status
	if (MySystem.ThermoOn)
		set_Rele(THERMO_PIN, HIGH);

	if (MySystem.LightOn)
		set_Rele(LIGH_PIN, HIGH);

	to_be_saved = true;

	arteUnlock();
}

// ritorna lo stato del sistema
bool SystemLocked() {
	return MySystem.Locked;
}

//--------------------------------------------------------------------------
// metodo invocato dal Task relativo al sensore, salva il valore del Sensore
// nell'apposita struttura nella struttura sistema
//--------------------------------------------------------------------------
void ReadAndSetMotion() {
	bool	motion;

	motion = Read_Pir();
	MySystem.SensorsValue.Motion = motion;
}

//--------------------------------------------------------------------------
// metodo invocato dal Task relativo al sensore, salva il valore del sensore
// nell'apposita struttura nella struttura sistema
//--------------------------------------------------------------------------
void ReadAndSetLuminosity() {
	int	luminosity;

	luminosity = Read_Luminosity();
	MySystem.SensorsValue.Luminosity = luminosity;
}

//--------------------------------------------------------------------------
// metodo invocato dal Task relativo al sensore, salva il valore del sensore
// nell'apposita struttura nella struttura sistema
//--------------------------------------------------------------------------
void ReadAndSetGas() {
	int	gas;

	gas = Read_Gas();
	MySystem.SensorsValue.Gas = gas;
}

//--------------------------------------------------------------------------
// metodo invocato dal Task relativo al sensore, salva il valore del sensore
// nell'apposita struttura nella struttura sistema
// se il dato non è valido viene scartato
//--------------------------------------------------------------------------
void ReadAndSetTemperatureAndHumidity() {
	float	temperature, humidity;

	temperature = Read_Temperature();
	humidity = Read_Humidity();

	if (temperature != -1)
		MySystem.SensorsValue.Temperature = temperature;

	if (humidity != -1)
		MySystem.SensorsValue.Humidity = humidity;
}

//--------------------------------------------------------------------------
// I due metodi che verificano un evento di allarme, quando disattivano	
// l'allarme, tengono in considerazione dell'altro evento altrimenti si 
// potrebbe verificare la situazione in cui un metodo disattivi l'allarme 
// attivato per il manifesstarsi di un altro evento, vedi turnAlarm	
// Controlla il valore relativo al sensore motion, se indica un intrusione	
// incremento un contatore, se supero la soglia di intrusioni attivo 
// l'allarme, il quale si disattiverà quando il numero di intrusioni 
// è inferiore alla soglia minima, oppure quando si esegue una unlock
//--------------------------------------------------------------------------
void CheckIntrusion() {
	bool	alarmOn, motion;

	alarmOn = MySystem.AlarmOn;
	motion = MySystem.SensorsValue.Motion;

	if (SystemLocked() && motion) {
		MySystem.n_intrusion++;
		if (MySystem.n_intrusion >= MIN_INTRUSION) {
			MySystem.IntrusionAlarmOn = HIGH;
			if (!alarmOn)
				turnAlarm(true);
		}
	}
	else {
		if ((MySystem.n_intrusion - 1) >= 0) 
			MySystem.n_intrusion--;

		if (MySystem.n_intrusion < MIN_INTRUSION) {
			MySystem.IntrusionAlarmOn = LOW;

			if (alarmOn)
				turnAlarm(false);
		}			
	}
}

//--------------------------------------------------------------------------
// Controlla il valore relativo al sensore Gas, se indica una presenza di 
// gas incremento un contatore, se supero la soglia, attivo l'allarme
// il quale si disattiverà quando il numero di rilevazioni-gas è inferiore
// alla soglia minima
//--------------------------------------------------------------------------
void CheckGas() {
	int		gasLevel;
	bool	gasAlarmPin, alarmOn;	

	alarmOn = MySystem.AlarmOn;
	gasLevel = MySystem.SensorsValue.Gas;
	gasAlarmPin = !digitalRead(SENSOR_MQ4_PIN_D);

	if (gasAlarmPin || (gasLevel >= GAS_THREASHOLD)) {
		MySystem.n_gasalarm++;

		if (MySystem.n_gasalarm >= MIN_GASALARM) {
			MySystem.GasAlarmOn = HIGH;

			if (!alarmOn)
				turnAlarm(true);
		}
	}
	else {
		if ((MySystem.n_gasalarm - 1) >= 0)
			MySystem.n_gasalarm--;

		if (MySystem.n_gasalarm < MIN_GASALARM) {
			MySystem.GasAlarmOn = LOW;

			if (alarmOn)
				turnAlarm(false);
		}
	}
}

System_Status getSystemCpy() {
	System_Status	cpySS;

	arteLock();

	cpySS = MySystem;

	arteUnlock();
	return cpySS;
}

RT_Task getTaskCpy(const int i) {
	RT_Task cpy;

	arteLock();

	cpy = myTasks[i];

	arteUnlock();
	return cpy;
}

//--------------------------------------------------------------------------
// Inizializza le strutture dati relative ad ogni task						
//--------------------------------------------------------------------------
void Init_Tasks() {
	int	i, now;

	arteLock();

	now = millis();
	for (i=0; i<MAX_TASKS; i++) {
		myTasks[i].Period = RT_Task_Periods[i];
		myTasks[i].Start_execution = 0;
		myTasks[i].FirstActivation = now;
		myTasks[i].lastActivation =  now;
		myTasks[i].Deadline = (now + myTasks[i].Period);
		myTasks[i].Deadline_miss = 0;
		myTasks[i].max_execution_time = 0;
		myTasks[i].ExecutionTime = 0;
		myTasks[i].AverageExecutionTime = 0;
		myTasks[i].sumExecTime = 0;
		myTasks[i].n = 0;
	}

	arteUnlock();
}

//--------------------------------------------------------------------------
// inizio esecuzione task i-esimo, calcola deadline assoluta, ed activation
//--------------------------------------------------------------------------
void Start_Execution(const int i) {
	int	now, index, Time_since_FirstActivation;
	int offset;

	now = millis();
	myTasks[i].Start_execution = now;
	Time_since_FirstActivation = (now - myTasks[i].FirstActivation);

	index = floor(Time_since_FirstActivation / myTasks[i].Period);
	offset = myTasks[i].FirstActivation;
	myTasks[i].lastActivation = offset + (index * myTasks[i].Period);

	myTasks[i].Deadline = (myTasks[i].lastActivation + myTasks[i].Period);
}

//--------------------------------------------------------------------------
// fine esecuzione task i-esimo, calcola deadlineMiss, Execution Time,
// Average execution time
//--------------------------------------------------------------------------
void End_Execution(const int i) {
	int	this_exectime, now, index, tot_job, Time_since_FirstActivation;

	now = millis();
	this_exectime = now - myTasks[i].Start_execution;
	myTasks[i].ExecutionTime = this_exectime;

	myTasks[i].sumExecTime = myTasks[i].sumExecTime + this_exectime;
	myTasks[i].n++;
	myTasks[i].AverageExecutionTime = (myTasks[i].sumExecTime / myTasks[i].n);

	if (this_exectime > myTasks[i].max_execution_time) {
		myTasks[i].max_execution_time = this_exectime;
	}

	if ((now > myTasks[i].Deadline)) {
		myTasks[i].Deadline_miss++;
		#ifdef DEBUG_MYSYSTEM
			Serial.print("Task");
			Serial.print(i + 1);
			Serial.println(" Deadline Miss");
		#endif
	}
}

// debug function
void PrintlnExecTimeAndAvgExecTime(const int i) {
	#ifdef DEBUG_MYSYSTEM
	Serial.print("MaxExecTime: ");
	Serial.println(myTasks[i].max_execution_time);
	Serial.print("AvgExecTime: ");
	Serial.println(myTasks[i].AverageExecutionTime);
	#endif
}