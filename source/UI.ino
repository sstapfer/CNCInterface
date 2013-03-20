/** @file UI.ino
	@brief These functions are used to build a user interface logic and to process user input.*/

int menulayer = 0;

enum indexlcdmenu_t {LISTFILES=1,SETUP};
int indexLCDMenu=1;

enum indexlcdconfigmenu_t {CLEARALL=1,BACK};
int indexLCDConfigMenu=1;

/** @brief this is the small interrupt routine for the ok button.*/
void okPressed()
{
	static unsigned long last_ok_interrupt_time = 0;
	unsigned long interrupt_time = millis();

	if(interrupt_time-last_ok_interrupt_time > 200) {
		buttonOK=true;
	}
	last_ok_interrupt_time = interrupt_time;
}

/** @brief this is the small interrupt routine for the down button.*/
void downPressed()
{
	static unsigned long last_dkey_interrupt_time = 0;
	unsigned long interrupt_time = millis();

	if(interrupt_time-last_dkey_interrupt_time > 300) {
		buttonDOWN=true;
	}
	last_dkey_interrupt_time = interrupt_time;
}

/** @brief this function displays the user menu on top layer.
	@param index This is the current cursor position*/
void lcdMenuTop(byte index)
{
	switch (index)
	{
	lcd.clrScr();
	case 1:
		lcd.print("=> CNC Daten",LEFT,8);
		lcd.print("   Setup",LEFT,16);
		break;
	case 2:
		lcd.print("   CNC Daten",LEFT,8);
		lcd.print("=> Setup",LEFT,16);
		break;
	default:
		lcd.print("   CNC Daten",LEFT,8);
		lcd.print("   Setup",LEFT,16);
	}
}

/** @brief this function displays the user menu on config layer.
	@param index This is the current cursor position*/
void lcdConfigMenu(byte index)
{
	switch (index)
	{
	lcd.clrScr();
	case 1:
		lcd.print("=> Daten clr",LEFT,16);
		lcd.print("   Zurueck",LEFT,24);
		break;
	case 2:
		lcd.print("   Daten clr",LEFT,16);
		lcd.print("=> Zurueck",LEFT,24);
		break;
	default:
		lcd.print("   Loeschen",LEFT,16);
		lcd.print("   Zurueck",LEFT,24);
	}
}

/** @brief this function processes the user input logic*/
void processUserInterface()
{
	char *buf = p_buffer;

	if(menulayer==0)	// top menu
	{
		lcdMenuTop(indexLCDMenu);

		if(buttonOK)
		{
			lcd.clrScr();
			switch(indexLCDMenu)
			{
			case LISTFILES:
				Serial.println(PMEM("Available files:"));
				numFiles = getFileList();
				if(numFiles>0) {
					int fileIndex = 0;
					while(fileIndex<numFiles) {
						fileList[fileIndex++].toCharArray(buf,80);
						lcd.print(buf,LEFT,fileIndex*8);
					}
					menulayer = 1;
				}
				else {
					lcd.print("Keine Daten",CENTER,8);
					lcd.print("vorhanden",CENTER,16);
					menulayer = 0;
					delay(2000);
					lcd.clrScr();
				}
				break;
			case SETUP:
				Serial.println(PMEM("Config mode."));
				lcd.print("Configuration:",LEFT,0);
				menulayer = 2;
				break;
			default:
				;
			}
			buttonOK=false;
		}
		if(buttonDOWN)
		{
			if((indexLCDMenu<0)||(indexLCDMenu>1))
			{
				indexLCDMenu=1;
			}
			else
			{
				indexLCDMenu++;
			}
			buttonDOWN=false;
		}
	}
	if(menulayer == 1)	// "CNC Daten" menu
	{
		if(buttonOK)
		{
				Serial.print("> ");
				Serial.println(fileList[actualFile]);
				CNCnextState = XY_ZERO;
				buttonOK=false;
				menulayer=0;
		}
		if(buttonDOWN)
		{
			if(actualFile>(numFiles-2))
			{
				actualFile = 0;
			}
			else
			{
				actualFile++;
			}
			Serial.println(fileList[actualFile]);
			lcd.clrScr();
			fileList[actualFile].toCharArray(buf,80);
			lcd.print(buf,LEFT,0);
			buttonDOWN=false;
		}
	}
	if(menulayer == 2)	// "Setup" menu
	{
		lcdConfigMenu(indexLCDConfigMenu);

		if(buttonOK)
		{
			lcd.clrScr();
			switch(indexLCDConfigMenu)
			{
			case CLEARALL:
				lcd.print("Alle Daten",CENTER,8);
				lcd.print("geloescht",CENTER,16);
				removeDataFiles();
				menulayer=0;
				delay(2000);
				break;
			case BACK:
				menulayer=0;
				break;
			default:
				;
			}
			buttonOK=false;
			lcd.clrScr();
		}
		if(buttonDOWN)
		{
			if((indexLCDConfigMenu<0)||(indexLCDConfigMenu>1))
			{
				indexLCDConfigMenu=1;
			}
			else
			{
				indexLCDConfigMenu++;
			}
			buttonDOWN=false;
		}
	}
}