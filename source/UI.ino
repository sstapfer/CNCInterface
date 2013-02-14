int menulayer = 0;
enum indexlcdmenu_t {LISTFILES=1,SETUP};
int indexLCDMenu=1;

void okPressed()
{
	buttonOK=true;
}

void downPressed()
{
	buttonDOWN=true;
}

void lcdMenuTop(byte index)
{
	switch (index)
	{
	lcd.clrScr();
	case 1:
		lcd.print("=> CNC Daten",LEFT,0);
		lcd.print("   Setup",LEFT,8);
		break;
	case 2:
		lcd.print("   CNC Daten",LEFT,0);
		lcd.print("=> Setup",LEFT,8);
		break;
	default:
		lcd.print("   CNC Daten",LEFT,0);
		lcd.print("   Setup",LEFT,8);
	}
}

void processUserInterface()
{
	char *buf = p_buffer;

	if(menulayer==0)
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
				menulayer = 1;
				break;
			case SETUP:
				Serial.println(PMEM("Config mode."));
				lcd.print("Configuration:",LEFT,0);
				break;
			default:
				;
			}
			buttonOK=false;
			delay(100);
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
			delay(500);
		}
	}
	if(menulayer == 1)
	{
		if(buttonOK)
		{
				Serial.print("> ");
				Serial.println(fileList[actualFile]);
				CNCnextState = XY_ZERO;
				buttonOK=false;
				menulayer=0;
				delay(100);
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
		delay(1000);
		}
	}
}