/**
	@file CNCInterface.ino

	@brief This is an ethernet interface (Webserver) to control an ISEL CNC mill.
			Users can upload their G-Code files and select which file to process to create PCB Prototypes.

	@author S. Stapfer

	@version 1.1

*/

#define WEBDUINO_AUTH_REALM "CNC Interface"
#define WEBDUINO_READ_TIMEOUT_IN_MS 10000
#define WEBDUINO_SERIAL_DEBUGGING 0
#define PREFIX ""
#define p_bufferLEN 80
#define PMEM(str) (strcpy_P(p_buffer, PSTR(str)), p_buffer)	// used to save RAM. Strings are stored in Flash instead.

#define LOCAL_DEBUG 0

/* ---------- Debug redirection -------------- */
#if LOCAL_DEBUG == 1
	#define CNCPort Serial
#else
	#define CNCPort Serial1
#endif
/* -------------------------------------- */

#include <SD.h>
#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>
#include <CNCInterpreter.h>
#include <LCD5110_Basic.h>

/* -------------------- Server config ------------------- */
byte mac[] = {0x90,0xA2,0xDA,0x00,0x9A,0xEB};

#if LOCAL_DEBUG == 1
	byte ip[] = {192, 168, 0, 1};
#else
	byte ip[] = {10, 202, 2, 24};		//	IP Labor
#endif

/** @brief This object is the webserver. Function calls can be made by "server.function()"
	@param PREFIX
	@param Port Port of the webserver. Usually 80*/
WebServer server(PREFIX, 80);
/* ---------------- End of Server config ---------------- */

// global variables
bool serverBusy = false;	// if CNC milling is in progress, file upload is not permitted -> serverBusy 
bool orderComplete = false;	// used to determine if order is complete
volatile bool buttonOK=false;
volatile bool buttonDOWN=false;
int actualFile = 0;			// index number of actual file (from 0 to numFiles)
int numFiles = 0;			// number of datafiles (G-Code) on SD-Card
char p_buffer[p_bufferLEN];
extern uint8_t SmallFont[];	// font definition for LCD

// Definition of the available states
enum state_t {INIT,WAITING,IDLE,XY_ZERO,Z_PRE_ZERO,Z_ZERO,START_POS,READY,PROCESSING,DONE,CONFIG};
int CNCprevState;
int CNCcurrentState;
int CNCnextState;

/** @brief This object is used to translate G-Code to ISEL specific "@...." syntax using the CNCInterpreter class*/
CNCInterpreter cnc;

/** @brief This object is used to display strings on an attached LC Display (Nokia 5110). Communication is made
	over the SPI bus.
	@param SCK Clock pin
	@param MOSI Master out Slave in pin
	@param DC Data or Command mode
	@param RST Reset pin
	@param CS Client select pin*/
LCD5110 lcd(7,6,5,3,8);

/** @brief This object is the file object on the SD-Card.*/
File datafile;

/** @brief This string object is the list of files on the SD-Card.
	@param Arraywidth defines the number of files displayed.*/
String fileList[10];


/**	@brief This displayIndexHTML function is called if a user request the /index.htm file. It reads the
	content of a index.htm file on the SD-Card and send it to the client.
	@param server Pointer to the server object
	@param type Type of connection (INVALID, GET, HEAD, POST, PUT, DELETE or PATCH)
	@param url_tail Rest of the URL, that doesn't fit in the given pattern
	@param tail_complete True, if the complete URL fit in url_tail buffer, false if not */
void displayIndexHTML(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  server.httpSuccess();	/* send header 400 to client */
  if (type != WebServer::HEAD)
  {
	if (!SD.exists("html/index.htm")) {
		P(helloMsg) = "index.htm not found. Insert SD card.";
		server.printP(helloMsg);
	}
	else {
		/* Load index.htm file from SD-Card and send it to the client. */
		streamFileToServer("html/index.htm");
	}
	Serial.println(PMEM("index...done."));
  }
}


/**	@brief This displayUploadForm function is called from the index.htm site or if a user requests the 
	/upload.htm file. It generates an upload dialog for storing files on the embedded SD-Card.
	@param server Pointer to the server object
	@param type Type of connection (INVALID, GET, HEAD, POST, PUT, DELETE or PATCH)
	@param url_tail Rest of the URL, that doesn't fit in the given pattern
	@param tail_complete True, if the complete URL fit in url_tail buffer, false if not */
void displayUploadForm(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
	bool userValid = false;

	if(!serverBusy)
	{
	/* if the user has requested this page using the following credentials
   * username = user
   * password = user
   * display a page saying "Hello User"
   *
   * the credentials have to be concatenated with a colon like
   * username:password
   * and encoded using Base64 - this should be done outside of your Arduino
   * to be easy on your resources
   *
   * in other words: "dXNlcjp1c2Vy" is the Base64 representation of "user:user"
   *
   * if you need to change the username/password dynamically please search
   * the web for a Base64 library */

/*	numUsers = getUsers();
	Serial.println(numUsers);
	for(int user=0;user<numUsers;user++)
	{
		Serial.println(userTable[user]);
		userValid = server.checkCredentials(userTable[user]);
		if(userValid)
		{
			Serial.println("User found");
//			break;
		}
	}*/

  if (server.checkCredentials("dXNlcjp1c2Vy"))
//	if(userValid)
  {
	serverBusy = true;
    server.httpSuccess();
    if (type != WebServer::HEAD)
    {
		if (!SD.exists("html/upload.htm")) {
			#if WEBDUINO_SERIAL_DEBUGGING
			Serial.println("File not found!");
			#endif
			P(helloMsg) = "upload.htm not found. Insert SD card.";
			server.printP(helloMsg);
		}
		else {
			streamFileToServer("html/upload.htm");
		}
	Serial.println(PMEM("upload...done."));
    }
  }
  /* if the user has requested this page using the following credentials
   * username = admin
   * password = admin
   * display a page saying "Hello Admin"
   *
   * in other words: "YWRtaW46YWRtaW4=" is the Base64 representation of "admin:admin" */
/*  else if (server.checkCredentials("YWRtaW46YWRtaW4="))
  {
    server.httpSuccess();
    if (type != WebServer::HEAD)
    {
		if (!SD.exists("html/admin.htm")) {
			#if WEBDUINO_SERIAL_DEBUGGING
			Serial.println("File not found!");
			#endif
			P(helloMsg) = "admin.htm not found. Insert SD card.";
			server.printP(helloMsg);
		}
		else {
			streamFileToServer("html/admin.htm");
		}
    }
  }*/
  else
  {
    /* send a 401 error back causing the web browser to prompt the user for credentials */
    server.httpUnauthorized();
  }
	}
	else
	{
		server.httpSuccess();
		if (!SD.exists("html/locked.htm")) {
			#if WEBDUINO_SERIAL_DEBUGGING
			Serial.println("File not found!");
			#endif
			P(helloMsg) = "locked.htm not found. Insert SD card.";
			server.printP(helloMsg);
		}
		else {
			streamFileToServer("html/locked.htm");
		}
	}
}


/**	@brief This processDataUpload function is called from the upload.htm form. It starts the upload of data to
	the SD-Card.
	@param server Pointer to the server object
	@param type Type of connection (INVALID, GET, HEAD, POST, PUT, DELETE or PATCH)
	@param url_tail Rest of the URL, that doesn't fit in the given pattern
	@param tail_complete True, if the complete URL fit in url_tail buffer, false if not */
void processDataUpload(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  URLPARAM_RESULT rc;

  /* if we're handling a GET or POST, we can output our data here.
     For a HEAD request, we just stop after outputting headers. */
  if (type == WebServer::HEAD)
    return;

  switch (type)
    {
    case WebServer::GET:
		Serial.println(PMEM("GET Request"));
		server.httpSeeOther("index.htm");
       break;
    case WebServer::POST:
		/* this line sends the standard "we're all OK" headers back to the
			browser */
		server.httpSuccess();

		Serial.println(PMEM("POST Request"));
		if (!SD.exists("html/done.htm")) {
			#if WEBDUINO_SERIAL_DEBUGGING
			Serial.println("File not found!");
			#endif
			P(helloMsg) = "done.htm not found. Insert SD card.";
			server.printP(helloMsg);
		}
		else {
			streamFileToServer("html/done.htm");
		}
		delay(10);
		streamDataToFile();
		server.println(PMEM("</div></div></body></html>"));
		break;
    default:
		server.httpSuccess();
        server.print("Unknown request\n");
		return;
    }
  	Serial.println(PMEM("file parsing...done."));
	serverBusy = false;
}


/**	@brief This displayLogoutHTML function is called if the users clicks on the logout button after an upload.
	@param server Pointer to the server object
	@param type Type of connection (INVALID, GET, HEAD, POST, PUT, DELETE or PATCH)
	@param url_tail Rest of the URL, that doesn't fit in the given pattern
	@param tail_complete True, if the complete URL fit in url_tail buffer, false if not */
void displayLogoutHTML(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
	server.httpSuccess();
	if (!SD.exists("html/logout.htm")) {
		P(helloMsg) = "logout.htm not found. Insert SD card.";
		server.printP(helloMsg);
	}
	else {
		streamFileToServer("html/logout.htm");
	}
}

/** @brief This function is waiting for the acknowledge byte '0' from the CNC machine */
void waitForCNC()
{
	while(CNCPort.read()!='0');
}

/** @brief Setup function is called once at the beginning. It is used to initialize all components such as Serial
	communication, LCD, SD-Card, Webserver and so on. */
void setup()
{
	Serial.begin(115200);	// initialize serial console
	
	char myIP[16] = ""; //maximale Größe des Strings sind 15 Zeichen plus Terminierung 
	int len = 0;
	len = sprintf(myIP, "%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);

	lcd.InitLCD();
	lcd.setFont(SmallFont);
	lcd.clrScr();

	// attach 2 interrupts for the buttons (OK and Down)
	attachInterrupt(3,okPressed,FALLING);
	attachInterrupt(2,downPressed,FALLING);

	/* Begin SD Card initialization */
	if (!SD.begin(4)) {
		Serial.println(PMEM("initialization failed!"));
		return;
	}
	lcd.print("SD Card found.",LEFT,0);
	Serial.println(PMEM("SD Card found."));
	/* End of SD Card initialization */

	/* Begin Webserver initialization */
	Ethernet.begin(mac,ip);
	server.setDefaultCommand(&displayIndexHTML);
	// register functions (see above) for specific pages i.e. "index.htm"
	server.addCommand("index.htm", &displayIndexHTML);
	server.addCommand("upload.htm",&displayUploadForm);
	server.addCommand("done.htm",&processDataUpload);
	server.addCommand("logout.htm",&displayLogoutHTML);
	server.begin();
	lcd.print("Serveraddress:",LEFT,8);
	lcd.print(myIP,LEFT,16);
	Serial.print(PMEM("Serveradress: "));
	Serial.println(myIP);
	/* End of Webserver initialization */

	/* Begin CNC (Serial) initialization */
#if LOCAL_DEBUG == 0
	CNCPort.begin(19200);
#endif
	CNCcurrentState = INIT;
	/* End CNC (Serial) initialization */

}

/** @brief loop function is called multiple times.

	Basically it does 3 things:
	1. Processing of the webserver
	2. Processing of the current state
	3. Switching to the new state*/
void loop()
{
	char buff[512];
	int len = 512;

	/* process incoming connections one at a time forever */
	server.processConnection(buff, &len);

	/* main state machine */
	switch(CNCcurrentState)
	{
	/** States:
		- Init: Configuration of CNC\n
		- Waiting: Wait for CNC to complete the last order. Depending on previous state there are different displays on
		the LCD\n
		- Idle: Processing of the user interface\n
		- XY_Zero: Sets the X and Y origin 10 mm on the inner side of the PCB\n
		- Z_Pre_Zero: Approach the PCB as preparation for the next step\n
		- Z_Zero: Move drill down on the surface of the PCB\n
		- Startpos: Move drill to a start position 100mm above the PCB\n
		- Ready: Wait for user to switch on the drilling motor\n
		- Processing: Send the next G-Code order to the CNC machine\n
		- Done: Move the milling tool away and display done to the user\n*/
	case INIT:
		initCNC();
		CNCprevState = CNCcurrentState;		// save previous state
		CNCnextState = WAITING;
		break;
	case WAITING:
		if(CNCPort.read()=='0')
		{
			switch(CNCprevState)
			{
			case INIT:
				Serial.println("Init done.");
				lcd.clrScr();
				CNCnextState = IDLE;
				break;
			case XY_ZERO:
				CNCnextState = Z_PRE_ZERO;
				break;
			case Z_PRE_ZERO:
				CNCnextState = Z_ZERO;
				break;
			case Z_ZERO:
				CNCnextState = START_POS;
				break;
			case START_POS:
				CNCnextState = READY;
				break;
			case DONE:
				Serial.println(PMEM("Bitte fertige Platine entfernen."));
				lcd.clrScr();
				lcd.print("Bitte fertige",CENTER,8);
				lcd.print("Platine",CENTER,16);
				lcd.print("entfernen",CENTER,24);
				while(!buttonOK);
				buttonOK=false;
				lcd.clrScr();
				CNCnextState = INIT;
				break;
			default:
				Serial.print("Unknown state: ");
				Serial.println(CNCprevState);
				CNCnextState = IDLE;
			}
		}
		break;
	case IDLE:
#if LOCAL_DEBUG == 1
//		--- OK button simulation ---
		char btnRead;
		btnRead = CNCPort.read();
		if(btnRead=='o')
		{
			buttonOK=true;
		}
		if(btnRead=='l')
		{
			buttonDOWN=true;
		}
#endif
		processUserInterface();
		break;
	case XY_ZERO:
		lcd.clrScr();
		lcd.print("In Bearbeitung",CENTER,16);
		serverBusy = true;
		setXYAxesZero();
		CNCprevState = CNCcurrentState;		// save previous state
		CNCnextState = WAITING;
		break;
	case Z_PRE_ZERO:
		moveZAxis();
		CNCprevState = CNCcurrentState;		// save previous state
		CNCnextState = WAITING;
		break;
	case Z_ZERO:
		setZAxisZero();
		CNCprevState = CNCcurrentState;		// save previous state
		CNCnextState = WAITING;
		break;
	case START_POS:
		moveToStartPos();
		CNCprevState = CNCcurrentState;		// save previous state
		CNCnextState = WAITING;
		break;
	case READY:
		Serial.println(PMEM("Bitte Fraesmotor starten."));
		lcd.clrScr();
		lcd.print("Bitte Fraes-",CENTER,8);
		lcd.print("motor starten",CENTER,16);
		lcd.print("OK = Weiter",CENTER,24);
		while(!buttonOK)
		{
#if LOCAL_DEBUG == 1
			// simulation of OK Button 
			if(CNCPort.read()=='o')
			{
				buttonOK=true;
			}
#endif
			;
		}
		detachInterrupt(3);		// Interrupt des Ok-Knopfes deaktivieren
		buttonOK=false;
		lcd.clrScr();
		sendCNC();
		CNCprevState = CNCcurrentState;		// save previous state
		break;
	case PROCESSING:
		lcd.clrScr();
		sendCNC();
		CNCprevState = CNCcurrentState;		// save previous state
		break;
	case DONE:
		moveToolAway();
		attachInterrupt(3,okPressed,FALLING);	// Interrupt des Ok-Knopfes wieder aktivieren
		lcd.clrScr();
		CNCprevState = CNCcurrentState;		// save previous state
		CNCnextState = WAITING;
		break;
	default:
		;
	}

	CNCcurrentState = CNCnextState;		// set new state
}
