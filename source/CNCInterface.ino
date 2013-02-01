/**
	Project:	CNCInterface

	\brief {This is an ethernet interface for control of a ISEL CNC mill.
			It uses the Ethernet and SD libraries.}

	\author {S. Stapfer}

	\version 0.8

*/

#define WEBDUINO_AUTH_REALM "CNC Interface"
#define WEBDUINO_SERIAL_DEBUGGING 0
#define MEGA_IS_USED 1

#include <SD.h>
#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>
#include <Base64.h>
#include <CNCInterpreter.h>
#include <LCD5110_Basic.h>

/* -------------------- Server config ------------------- */

byte mac[] = {0x90,0xA2,0xDA,0x00,0x9A,0xEB};
//byte ip[] = {10, 218, 5, 177};	//	ADM Domäne
//byte ip[] = {192, 168, 0, 1};
byte ip[] = {10, 202, 2, 24};		//	IP Labor

#define PREFIX ""
WebServer server(PREFIX, 80);

bool ServerBusy = false, orderComplete = false;

#define CNCPort Serial1	// Serial only for debug, use Serial1 instead!

int actualfile = 0, numFiles = 0;

CNCInterpreter cnc;

LCD5110 lcd(7,6,5,3,8);
volatile bool buttonOK=false,buttonUP=false,buttonDOWN=false;
/* --------------------- Static strings ----------------- */

#define p_bufferLEN 80
char p_buffer[p_bufferLEN];

#define PMEM(str) (strcpy_P(p_buffer, PSTR(str)), p_buffer)

extern uint8_t SmallFont[];

File datafile;

String fileList[10];

/*char userTable[10][64];
int numUsers = 0;*/

enum state_t {INIT,WAITING,IDLE,XY_ZERO,Z_PRE_ZERO,Z_ZERO,START_POS,READY,PROCESSING,DONE,CONFIG};
int CNCprevState, CNCcurrentState, CNCnextState;

void displayIndexHTML(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  server.httpSuccess();	// send'em headers
  if (type != WebServer::HEAD)
  {
	if (!SD.exists("html/index.htm")) {
		P(helloMsg) = "index.htm not found. Insert SD card.";
		server.printP(helloMsg);
	}
	else {
		streamFileToServer("html/index.htm");
	}
	Serial.println(PMEM("index...done."));
  }
}

void displayUploadForm(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
	bool userValid = false;

	if(!ServerBusy)
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
	ServerBusy = true;
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

void processDataUpload(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  URLPARAM_RESULT rc;

  /* if we're handling a GET or POST, we can output our data here.
     For a HEAD request, we just stop after outputting headers. */
  if (type == WebServer::HEAD)
    return;

//  server.printP("<html><body><h1>CNC Interpreter</h1>");
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
		/* this line sends the standard "we're all OK" headers back to the
			browser */
		server.httpSuccess();

        server.print("Unknown request\n");
		return;
    }
  	Serial.println(PMEM("file parsing...done."));
	ServerBusy = false;
}

void displayLogoutHTML(WebServer &server, WebServer::ConnectionType type, char *, bool)
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

void clearCNCPort()
{
	while(CNCPort.available()>0)
	{
		CNCPort.read();
	}
}

void waitForCNC()
{
	while(CNCPort.read()!='0');
}

void setup()
{


	Serial.begin(115200);
	
	lcd.InitLCD();
	lcd.setFont(SmallFont);
	lcd.clrScr();

	attachInterrupt(3,okPressed,FALLING);
	attachInterrupt(2,downPressed,FALLING);
	/** Begin SD Card initialization */
	if (!SD.begin(4)) {
		Serial.println(PMEM("initialization failed!"));
		return;
	}
	lcd.print("SD Card found.",LEFT,0);
	Serial.println(PMEM("SD Card found."));
	/* End SD Card initialization */

	/** Begin Webserver initialization */
	Ethernet.begin(mac,ip);
	server.setDefaultCommand(&displayIndexHTML);
	server.addCommand("index.htm", &displayIndexHTML);
	server.addCommand("upload.htm",&displayUploadForm);
	server.addCommand("done.htm",&processDataUpload);
	server.addCommand("logout.htm",&displayLogoutHTML);
	server.begin();
	lcd.print("Serveraddress:",LEFT,8);
	lcd.print(PMEM("10.202.2.24"),LEFT,16);
	Serial.print(PMEM("Serveradress: "));
	Serial.println(Ethernet.localIP());
	/* End Webserver initialization */

	/** Begin CNC (Serial) initialization */
	CNCPort.begin(19200);
	CNCcurrentState = INIT;
	/* End CNC (Serial) initialization */

}

void loop()
{
	char buff[128];	//64
	int len = 128;

	/* process incoming connections one at a time forever */
	server.processConnection(buff, &len);

	switch(CNCcurrentState)
	{
	case INIT:
//		Serial.println("**Init state");
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
//				Serial.println("XY_Zero done.");
				CNCnextState = Z_PRE_ZERO;
				break;
			case Z_PRE_ZERO:
//				Serial.println("Z_pre_zero done.");
				CNCnextState = Z_ZERO;
				break;
			case Z_ZERO:
//				Serial.println("Z_zero done.");
				CNCnextState = START_POS;
				break;
			case START_POS:
//				Serial.println("Start_pos done.");
				CNCnextState = READY;
				break;
			case DONE:
				Serial.println("Bitte fertige Platine entfernen.");
				lcd.clrScr();
				lcd.print("Bitte fertige",CENTER,8);
				lcd.print("Platine entfernen",CENTER,16);
				while(!buttonOK);
				buttonOK=false;
				lcd.clrScr();
				CNCnextState = IDLE;
				break;
			default:
				Serial.print("Unknown state: ");
				Serial.println(CNCprevState);
				CNCnextState = IDLE;
			}
		}
		break;
	case IDLE:

		char btnRead;

/*		btnRead = CNCPort.read();
//		Serial.println("**Idle state");
//		--- OK button simulation ---
		if(btnRead=='o')
		{
			buttonOK=true;
		}
		if(btnRead=='l')
		{
			buttonDOWN=true;
		}
		------------------------*/
		processUserInterface();
		break;
	case XY_ZERO:
//		Serial.println("**XY_Zero state");
		lcd.clrScr();
		lcd.print("In Bearbeitung",CENTER,16);
		ServerBusy = true;
		setXYAxesZero();
		CNCprevState = CNCcurrentState;		// save previous state
		CNCnextState = WAITING;
		break;
	case Z_PRE_ZERO:
//		Serial.println("**Z_pre_zero state");
		moveZAxis();
		CNCprevState = CNCcurrentState;		// save previous state
		CNCnextState = WAITING;
		break;
	case Z_ZERO:
//		Serial.println("**Z_zero state");
		setZAxisZero();
		CNCprevState = CNCcurrentState;		// save previous state
		CNCnextState = WAITING;
		break;
	case START_POS:
//		Serial.println("**Start_pos state");
		moveToStartPos();
		CNCprevState = CNCcurrentState;		// save previous state
		CNCnextState = WAITING;
		break;
	case READY:
		Serial.println("Bitte Fraesmotor starten.");
		lcd.clrScr();
		lcd.print("Bitte Fraes-",CENTER,8);
		lcd.print("motor starten",CENTER,16);
		while(!buttonOK)
		{
			/* simulation of OK Button 
			if(CNCPort.read()=='o')
			{
				buttonOK=true;
			}*/
			;
		}
		detachInterrupt(3);		// Interrupt des Ok-Knopfes deaktivieren
		buttonOK=false;
		sendCNC();
		CNCprevState = CNCcurrentState;		// save previous state
		break;
	case PROCESSING:
//		Serial.println("**Processing state");
		sendCNC();
		CNCprevState = CNCcurrentState;		// save previous state
		break;
	case DONE:
//		Serial.println("**Done state");
		moveToolAway();
		attachInterrupt(3,okPressed,FALLING);	// Interrupt des Ok-Knopfes wieder aktivieren
		CNCprevState = CNCcurrentState;		// save previous state
		CNCnextState = WAITING;
		break;
	default:
		;
	}

	CNCcurrentState = CNCnextState;		// set new state

}
