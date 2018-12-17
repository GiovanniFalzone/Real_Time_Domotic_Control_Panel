#include "arte.h"
#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>
#include <WebSocket.h>
#include <SD.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2591.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Nextion.h>

#include "WebServer.h"
#include "Display.h"
#include "MySystem.h"
 
#define DEBUG

#define TASK_ID1				0
#define TASK_ID2				1
#define TASK_ID3				2
#define TASK_ID4				3
#define TASK_ID5				4

#define SETUP_DELAY				2000
#define SERIAL_SPEED			57600

static bool	setup_completed = false;

void setup() {
	// il parser mette qui i SetRelAlarm(loop, offset, period);

	Init_Tasks();

	Serial.begin(SERIAL_SPEED);

	InitSensors();
	Init_Rele_Shield();
	Init_SD();
	Init_WebServer();
	InitDisplay();

	System_Lock();

	RestoreSystemStatus();	//se posso ripristino

	if (SystemLocked())
		DisplayShowLockPage();
	else
		DisplayShowHomePage();

	delay(SETUP_DELAY);
	setup_completed = true;
}

void loop() {
	while (1)
		;
}

// Task System 1
void loop1(350) {
	Start_Execution(TASK_ID1);

	if (!setup_completed) {
		End_Execution(TASK_ID1);
		return;
	}

	ReadAndSetLuminosity();
	ReadAndSetGas();
	CheckGas();
	ManageLightAuto();
	ManageThermoAuto();

	End_Execution(TASK_ID1);
}

// Task System 2
void loop2(4000) {
	Start_Execution(TASK_ID2);

	if (!setup_completed) {
		End_Execution(TASK_ID2);
		return;
	}

	ReadAndSetMotion();
	CheckIntrusion();
	ReadAndSetTemperatureAndHumidity();
	StorageSystem();

	End_Execution(TASK_ID2);
}

// Task Control Panel
void loop3(350) {
	Start_Execution(TASK_ID3);

	if (!setup_completed) {
		End_Execution(TASK_ID3);
		return;
	}

	DisplayIteration();

	End_Execution(TASK_ID3);
}

// Task Web Server
void loop4(4000) {
	Start_Execution(TASK_ID4);

	if (!setup_completed) {
		End_Execution(TASK_ID4);
		return;
	}

	if (!SystemLocked()) {
		listenForEthernetClients();
	}

	End_Execution(TASK_ID4);
}

// Task Web Socket
void loop5(500) {
	Start_Execution(TASK_ID5);

	if (!setup_completed) {
		End_Execution(TASK_ID5);
		return;
	}

	if (!SystemLocked()) {
		WebSocketIteration();
	}

	ListenIncomingDataFromSocket();

	End_Execution(TASK_ID5);
}