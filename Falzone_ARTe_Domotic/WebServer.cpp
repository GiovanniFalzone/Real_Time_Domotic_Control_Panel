#include "WebServer.h"

#include <SPI.h>
#include <Wire.h>
#include <Ethernet.h>
#include <WebSocket.h>

#include "WebPages.h"	//contiene le pagine come const char *pg[];

const char	*HTTP_header[HTTP_HEADER_DIM] = {
	HTTP_HEADER_LINE0,
	HTTP_HEADER_LINE1,
	HTTP_HEADER_LINE2
};

const char	*HTTP_header404[HTTP_HEADER404_DIM] = {
	HTTP_HEADER404_LINE0,
	HTTP_HEADER404_LINE1,
	HTTP_HEADER404_LINE2
};

//--------------------------------------------------------------------------
// Lo stato	dell'interfaccia dipende dalla pagina in cui si trova, anche nel
// nel caso di pagine dinamiche, in questo modo riduco il flusso di dati
//--------------------------------------------------------------------------
static enum	WPage {	
	HOMEPAGE, 			// 0
	SENSORPAGE, 		// 1
	TASKPAGE 			// 2
}	WebInterfaceState;

//--------------------------------------------------------------------------
// Nomi delle pagine, vengono recuperati con l'indice della pagina richiesta
//--------------------------------------------------------------------------
static char	PagesFName[NPAGES][MAXFILENAMESIZE] = {
	HOMEPAGE_FILE, 		// 0
	SENSORPAGE_FILE, 	// 1
	TASKPAGE_FILE		// 2
};

//--------------------------------------------------------------------------
static byte	mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static int	IpAddr[] = { 192, 168, 1, 2 };
static int	Gateway[] = { 192, 168, 1, 1 };
static int	SubtMask[] = { 255, 255, 255, 0 };

//--------------------------------------------------------------------------
// assegno gli indirizzi ip per il controller Ethernet
//--------------------------------------------------------------------------
static IPAddress	ip(IpAddr[0], IpAddr[1], IpAddr[2], IpAddr[3]);
static IPAddress	gateway(Gateway[0], Gateway[1], Gateway[2], Gateway[3]);
static IPAddress	subnet(SubtMask[0], SubtMask[1], SubtMask[2], SubtMask[3]);

static EthernetServer	server(SERVER_PORT);
static EthernetClient	client;

static WebSocketServer	webSocketServer(ROOT, WEBSOCKET_PORT, MAX_CONNECTION);
static WebSocket		*webSocket = NULL;


//--------------------------------------------------------------------------
// Operazioni richieste riguardo la gestione delle luci, ogni comando viene 
// rappresentao da un char definito nell' header
//--------------------------------------------------------------------------
void LightOps(char cmd) {
	switch (cmd) {
		case OFF_CHAR:
			turnLight(LOW);
			break;

		case ON_CHAR:
			turnLight(HIGH);
			break;

		case PLUS_CHAR:
			IncDesiredLuminosity();
			break;

		case MINUS_CHAR:
			DecDesiredLuminosity();
			break;

		case AUTO_CHAR:
			SwitchLightAuto();
			break;

		default:
			break;
	}
}

//--------------------------------------------------------------------------
// Operazioni richieste riguardo la gestione del riscaldamento, ogni comando
// viene rappresentao da un char definito nell' header
//--------------------------------------------------------------------------
void TermoOps(char cmd) {
	switch (cmd) {
		case OFF_CHAR:
			turnThermo(LOW);
			break;

		case ON_CHAR:
			turnThermo(HIGH);
			break;

		case PLUS_CHAR:
			IncDesiredTemp();
			break;

		case MINUS_CHAR:
			DecDesiredTemp();
			break;

		case AUTO_CHAR:
			SwitchThermoAuto();
			break;

		default:
			break;
	}
}

//--------------------------------------------------------------------------
// Operazioni permesse quando ci troviamo nella HomePage, i comandi vengono 
// rappresentati tramite un char definito nell' header
//--------------------------------------------------------------------------
void OnDataReceive_HomePageOps(char *cmd) {
	switch (cmd[0]) {
		case LIGHT_CHAR:
			LightOps(cmd[1]);
			break;

		case THERMO_CHAR:
			TermoOps(cmd[1]);
			break;

		case STATE_CHAR:
			WebInterfaceState = static_cast<WPage>(cmd[1] - '0');
			break;

		default:
			break;
	}	
}

//--------------------------------------------------------------------------
// Operazioni permesse quando ci troviamo nella SensorPage, i comandi 
// vengono rappresentati tramite un char definito nell' header
//--------------------------------------------------------------------------
void OnDataReceive_SensorPageOps(char *cmd) {
	switch (cmd[0]) {
		case STATE_CHAR:
			WebInterfaceState = static_cast<WPage>(cmd[1] - '0');
		break;

		default:
			break;
	}
}

//--------------------------------------------------------------------------
// Operazioni permesse quando ci troviamo nella TasksPage, i comandi
// vengono rappresentati tramite un char definito nell' header
//--------------------------------------------------------------------------
void OnDataReceive_TaskPageOps(char *cmd) {
	switch (cmd[0]) {
		case STATE_CHAR:
			WebInterfaceState = static_cast<WPage>(cmd[1] - '0');
		break;

		default:
			break;
	}
}

//--------------------------------------------------------------------------
// Funzione invocata tramite CallBack quando abbiamo ricevuto dati tramite 	
// WebSocket, i comandi vengono ricevuti come array di char, ogni pagine 	
// ha le relative operazioni permesse, un comando non lecito viene scartato 
//--------------------------------------------------------------------------
void OnDataReceiveOps(char *cmd) {
	switch (WebInterfaceState) {
		case HOMEPAGE:
			OnDataReceive_HomePageOps(cmd);
			break;

		case SENSORPAGE:
			OnDataReceive_SensorPageOps(cmd);
			break;

		case TASKPAGE:
			OnDataReceive_TaskPageOps(cmd);
			break;

		default:
			break;
	}
}

//--------------------------------------------------------------------------
// CallBack invocata dal WebSocketServer quando abbiamo una richiesta 
// di connessione, viene assegnata dunque la websocket relativa	al client
//--------------------------------------------------------------------------
void onConnect(WebSocket &socket) {
	webSocket = &socket;
}

//--------------------------------------------------------------------------
// CallBack invocata dal WebSocketServer quando riceviamo dati tramite la 	
// WebSocket i dati sono un array di char 
//--------------------------------------------------------------------------
void onData(WebSocket &socket, char* dataString, byte frameLength) {
	if (!SystemLocked())
		OnDataReceiveOps(dataString);
}

//--------------------------------------------------------------------------
// CallBack invocata dal WebSocketServer quando riceviamo una disconnect,
// il webSocketServer risponde alla disconnect e io posso liberare il socket
//--------------------------------------------------------------------------
void onDisconnect(WebSocket &socket) {
	if (&socket == webSocket)
		webSocket = NULL;
}

//--------------------------------------------------------------------------
// Invio http header relativo al 404 page not found, definita in header 
//--------------------------------------------------------------------------
void Send404ErrorPage() {
	int	i;

	for (i=0; i<HTTP_HEADER404_DIM; i++)
		client.println(HTTP_header404[i]);
}


//--------------------------------------------------------------------------
// Invio la pagina che si trova in WebPages.h
//--------------------------------------------------------------------------
void SendHomePage() {
	int	i;

	for (i=0; i<LEN_HOME_PAGE; i++)
		client.println(home_page[i]);

}

void SendSensorPage() {
	int	i;

	for (i=0; i<LEN_SENSOR_PAGE; i++)
		client.println(sensor_page[i]);

}

void SendTaskPage() {
	int	i;

	for (i=0; i<LEN_TASK_PAGE; i++)
		client.println(task_page[i]);

}

//--------------------------------------------------------------------------
// dato l'id della pagina richiesta, invio prima l'http header e in seguito 
// apro la pagina su SD, e per ogni byte letto lo invio tramite il webSocket
// relativo al client, se il file non esiste su SD esco con errore 	
//--------------------------------------------------------------------------
void Send_WebPage_to_Client(int PageId) {
	int		i;
	for (i=0; i<HTTP_HEADER_DIM; i++)
		client.println(HTTP_header[i]);

	switch (PageId) {
		case 0:
			SendHomePage();
			break;

		case 1:
			SendSensorPage();
			break;

		case 2:
			SendTaskPage();
			break;
		default:
			break;
	}

}

//--------------------------------------------------------------------------
// Controllo l'id passato tramite http request, se è un id corretto invio la
// pagina relativa a quell'indice, se richiesta vuota invio la HomePage		
// quando invio una pagina, imposto lo stato dell'interfaccia alla pagina	
// relativa. Se l'id non è pertinente invio http header 404 Page not found	
//--------------------------------------------------------------------------
void CheckIndex(char c1) {
	int	PageId = (c1 - '0');
	
	if ((PageId >= 0) && (PageId < NPAGES)) {
		Send_WebPage_to_Client(PageId);
		WebInterfaceState = static_cast< WPage >(PageId);
	}
	else {
		if (c1 == ' ') {
			PageId = HOMEPAGE;
			Send_WebPage_to_Client(PageId);
			WebInterfaceState = static_cast< WPage >(PageId);
		}
		else {
			Send404ErrorPage();
		}
	}
}

//--------------------------------------------------------------------------
// Controllo se ho richieste http provenienti dalla Ethernet, analizzo la 	
// richiesta, e invio l'indice alla checkIndex, è permessa una sola 
// connessione (una http request finisce con una linea vuota)
//--------------------------------------------------------------------------
void listenForEthernetClients() {
	bool	close = false;
	int		count;	//cursore di lettura, per selezionare l'indice pagina
	char	c, c1;

	client = server.available();
	if (client && !webSocket) {					// solo una connessione
		boolean currentLineIsBlank = true;
		close = false;
		count=0;
		while (client.connected() && client.available() && !close) {
			c = client.read();
			count++;

			if (count == 6) {
				c1 = c;
			}

			if (c == '\n' && currentLineIsBlank) {	//end of HTTP Request
				CheckIndex(c1);
				close = true;
			}

			if (c == '\n') {
				currentLineIsBlank = true;
			}
			else
				if (c != '\r') {
					currentLineIsBlank = false;
			}
		}
	}
	delay(1);	// give the web browser time to receive the data
	client.stop();
}

//--------------------------------------------------------------------------
// Invocata dal loop, controlla se ci sono richieste o dati inviati alla 	
// la webSocketServer, se vi sono richieste chiama le funzioni di CallBack 	
// relative alla richiesta, va invocata anche quando siamo in Lock per poter
// servire una richiesta di Disconnect
//--------------------------------------------------------------------------
void ListenIncomingDataFromSocket() {
	webSocketServer.listen();
}

byte HowManyConnetions() {
	return webSocketServer.connectionCount();
}

void SendStringToClient(char *str, int len) {
	webSocketServer.send(str, len);
}

int CurrentWebPage() {
	return WebInterfaceState;
}

//--------------------------------------------------------------------------
// Inizializza il controller Ethernet, il WebServer, il WebSocket, registra 
// le funzioni di CallBack relative alle richieste provenienti dal WebSocket
//--------------------------------------------------------------------------
void Init_WebServer() {
	int	i;

	Ethernet.begin(mac, ip);
	server.begin();

	webSocketServer.registerConnectCallback(&onConnect);
	webSocketServer.registerDataCallback(&onData);
	webSocketServer.registerDisconnectCallback(&onDisconnect);

	webSocketServer.begin();

}

//--------------------------------------------------------------------------
// Invia in formato JSON  un id e il relativo valore
//--------------------------------------------------------------------------
void sendJSONToClient(const char * id, const char * value) {
	const char	fst[] =  JSON_ID;
	const char	scnd[] = JSON_VALUE;
	const char	thrd[] = JSON_END;
	const int	sum = (strlen(fst) + strlen(scnd) + strlen(thrd));
	const int	len = (sum + strlen(id) + strlen(value) + 1);
	char		tmp[len];

	memset(tmp, '\0', len);

	strcat(tmp, fst);
	strcat(tmp, id);
	strcat(tmp, scnd);
	strcat(tmp, value);
	strcat(tmp, thrd);

	SendStringToClient(tmp, strlen(tmp));
}

//--------------------------------------------------------------------------
// ha come parametro il periodo di un Task e provvede a comporre ed inviare 
// il messaggio in formato JSON al client
//--------------------------------------------------------------------------
void SendPeriod(int i, int period) {
	const int	namelen = sizeof(ID_PERIOD);
	const int	valuelen= 7;	// "123456 123,456\0"
	char		name[namelen];
	char		value[valuelen];

	memset(name, '\0', namelen);
	memset(value, '\0', valuelen);
	snprintf(name, namelen, ID_PERIOD, (i + 1));
	snprintf(value, valuelen, "%4i\0", period);

	sendJSONToClient(name, value);
}

//--------------------------------------------------------------------------
// ha come parametro iltempo di esecuzione di un Task e provvede a comporre	
// ed inviare  il messaggio in formato JSON al client
//--------------------------------------------------------------------------
void SendExecTime(int i, int exectime) {
	const int	namelen = sizeof(ID_EXECTIM);
	const int	valuelen= 7;	// "123456 123,456\0"
	char		name[namelen];
	char		value[valuelen];

	memset(name, '\0', namelen);
	memset(value, '\0', valuelen);
	snprintf(name, namelen, ID_EXECTIM, (i + 1));
	snprintf(value, valuelen, "%4i\0", exectime);

	sendJSONToClient(name, value);
}

//--------------------------------------------------------------------------
// simile alla precedente, invia il numero di deadline miss del task i-esemo
// il messaggio è in formato JSON al client
//--------------------------------------------------------------------------
void SendDeadlineMiss(int i, int deadls) {
	const int	namelen = sizeof(ID_DEADLS);
	const int 	valuelen= 5;
	char		name[namelen];
	char		value[valuelen];

	memset(name, '\0', namelen);
	memset(value, '\0', valuelen);

	snprintf(name, namelen, ID_DEADLS, (i + 1));
	snprintf(value, valuelen, "%4i\0", deadls);

	sendJSONToClient(name, value);

}

//--------------------------------------------------------------------------
// simile alla precedente, invia il Max execution time del task i-esemo
// il messaggio è in formato JSON al client
//--------------------------------------------------------------------------
void SendMaxExec(int i, int maxexec) {
	const int	namelen = sizeof(ID_MAXEXEC);
	const int 	valuelen= 5;
	char		name[namelen];
	char		value[valuelen];	

	memset(name, '\0', namelen);
	memset(value, '\0', valuelen);

	snprintf(name, namelen, ID_MAXEXEC, (i + 1));
	snprintf(value, valuelen, "%4i\0", maxexec);

	sendJSONToClient(name, value);
}

void SendAvgExec(int i, float avgexectime) {
	const int	namelen = sizeof(ID_AVG_EXECTIM);
	const int 	valuelen= 9;
	char		name[namelen];
	char		value[valuelen];

	memset(name, '\0', namelen);
	memset(value, '\0', valuelen);

	snprintf(name, namelen, ID_AVG_EXECTIM, (i + 1));
	snprintf(value, valuelen, "%8.3f\0", avgexectime);

	sendJSONToClient(name, value);
}

//--------------------------------------------------------------------------
// invia tutti i dati relativi alla pagina Tasks in formato JSON al client	
//--------------------------------------------------------------------------
void UpdateTaskPage() {
	int i;
	RT_Task 	TaskCpy;

	for (i=0; i<MAX_TASKS; i++) {
		TaskCpy = getTaskCpy(i);

		SendPeriod(i, TaskCpy.Period);
		SendExecTime(i, TaskCpy.ExecutionTime);
		SendDeadlineMiss(i, TaskCpy.Deadline_miss);
		SendMaxExec(i, TaskCpy.max_execution_time);
		SendAvgExec(i, TaskCpy.AverageExecutionTime);
	}
}

//--------------------------------------------------------------------------
// invia tutti i dati relativi alla pagina Sensors in formato JSON al client
//--------------------------------------------------------------------------
void UpdateSensors(int l, int g, float t, float h, bool p) {
	const int	len =8;
	char		value[len];

	memset(value, '\0', len);
	snprintf(value, len, "%4i\0", l);
	sendJSONToClient(ID_LUX, value);

	memset(value, '\0', len);
	snprintf(value, len, "%4i\0", g);
	sendJSONToClient(ID_GAS, value);

	memset(value, '\0', len);
	snprintf(value, len, "%4.2f\0", t);
	sendJSONToClient(ID_TEMP, value);

	memset(value, '\0', len);
	snprintf(value, len, "%4.2f\0", h);
	sendJSONToClient(ID_HUM, value);

	memset(value, '\0', len);
	snprintf(value, len, "%4i\0", p);
	sendJSONToClient(ID_PIR, value);
}

//--------------------------------------------------------------------------
// invia le soglie di Temperatura ed umidità al client in formato JSON		
//--------------------------------------------------------------------------
void UpdateThreasholds(int deslux, float destemp) {
	const int	len =8;
	char		value[len];

	memset(value, '\0', len);
	snprintf(value, len, "%4i\0", deslux);
	sendJSONToClient(ID_DESLUX, value);

	memset(value, '\0', len);
	snprintf(value, len, "%4.2f\0", destemp);
	sendJSONToClient(ID_DESTEMP, value);
}

//--------------------------------------------------------------------------
// invia lo stato del Riscaldamento e dell'Illuminazione al client in JSON 	
//--------------------------------------------------------------------------
void UpdateButtons(bool LOn, bool LAuto, bool TOn, bool TAuto) {
	const int	len =2;
	char		value[len];

	snprintf(value, len, "%1d\0", LOn);
	sendJSONToClient(ID_LIGHT_ON, value);

	snprintf(value, len, "%1d\0", LAuto);
	sendJSONToClient(ID_LIGHT_AUTO, value);

	snprintf(value, len, "%1d\0", TOn);
	sendJSONToClient(ID_THERMO_ON, value);

	snprintf(value, len, "%1d\0", TAuto);
	sendJSONToClient(ID_THERMO_AUTO, value);
}

//--------------------------------------------------------------------------
// invocata dal loop, se abbiamo un client connesso alla websocket, provvede
// ad aggiornare la paggina in cui si trova inviandogli le informazioni 	
// relative a quella pagina	
//--------------------------------------------------------------------------
void WebSocketIteration() {
	float	temp, hum, destemp;
	int		i, gas, lux, deslux;
	bool	pir, thermoon, lighton, thermoauto, lightauto;
	System_Status	cpySS;

	cpySS = getSystemCpy();

	pir = cpySS.SensorsValue.Motion;
	lux = cpySS.SensorsValue.Luminosity;
	gas = cpySS.SensorsValue.Gas;
	temp = cpySS.SensorsValue.Temperature;
	hum = cpySS.SensorsValue.Humidity;
	deslux = cpySS.desired_lux;
	destemp = cpySS.desired_temp;
	lighton = cpySS.LightOn;
	lightauto = cpySS.LightAuto;
	thermoon = cpySS.ThermoOn;
	thermoauto = cpySS.ThermoAuto;

	if (HowManyConnetions()) {
		switch (WebInterfaceState) {
			case HOMEPAGE:
				UpdateSensors(lux, gas, temp, hum, pir);
				UpdateThreasholds(deslux, destemp);
				UpdateButtons(lighton, lightauto, thermoon, thermoauto);
				break;

			case SENSORPAGE:
				UpdateSensors(lux, gas, temp, hum, pir);
				break;

			case TASKPAGE:
				UpdateTaskPage();
				break;

			default:
				break;
		}
	}
}