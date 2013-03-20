/** @file Streaming.ino
	@brief These functions are used to stream data from the SD card to the webserver and the CNC and from the webserver
	to the SD Card.*/

/** @brief this function streams the specified file from the SD card to the webserver. Used to display HTML pages
	saved on SD card.
	@param filename Name of the file to stream*/
void streamFileToServer(char *filename)
{
	char buf[128];
	int16_t c;	
	int16_t index = 0;

	datafile = SD.open(filename, FILE_READ);
	delay(10);
	while((c=datafile.read()) > 0) {	// read file until EOF
		switch(c) {
		case 10:	// line feed
			buf[index] = '\0';
			server.println(buf);
			index = 0;
			break;
		default:
			buf[index++] = (char)c;
		}
		delay(1);
	}
	buf[index] = '\0';
	server.println(buf);
	datafile.close();
	delay(10);
	free(buf);
}

/** @brief this function streams data from a user selected file to the SD card using the upload dialog.*/
bool streamDataToFile()
{
	int ch=0, line=0;
	char databuffer[80];
	char *ptr_databuffer = databuffer;
	char filename[24] = "data/";
	char tmpFilename[24];
	char *ptr_filename = tmpFilename;

#if WEBDUINO_SERIAL_DEBUGGING > 0
	Serial.println(PMEM("----------Header------------"));
#endif
	// remove header
	server.readHeader(databuffer,80);
	server.readHeader(databuffer,80);

	/* get Filename */
	ptr_databuffer = strstr(databuffer,"filename=");
	ptr_databuffer += 10;
	while(*ptr_databuffer != '.')
	{
		*ptr_filename++ = *ptr_databuffer++;
	}
	*ptr_filename = '\0';
	strcat(filename, tmpFilename);
	free(tmpFilename);
	strcat(filename,".cnc");
#if WEBDUINO_SERIAL_DEBUGGING > 0
	Serial.println(filename);
#endif
	ptr_databuffer = databuffer;		// reset index of ptr_databuffer to databuffer[0]
	/* End of get Filename */

	server.readHeader(databuffer,80);

	for(int index=0;index<4;index++)	// remove CR's and LF at beginning
	{
		server.read();
	}
#if WEBDUINO_SERIAL_DEBUGGING > 0
	Serial.println(PMEM("----------Params------------"));
#endif

	while((ch = server.read()) != -1)
	{
		while(ch!=10) {
			if( (ch>64 && ch<91) || (ch>96 && ch<123) || (ch>47 && ch<58) || (ch==45) || (ch==46) || (ch==32)) {
				*ptr_databuffer++ = (char)ch;
			}
			ch = server.read();
		}
		*ptr_databuffer = '\0';
		ptr_databuffer = databuffer;		// reset index of ptr_databuffer to databuffer[0]
		if(databuffer[0]=='N') {
			line++;
#if WEBDUINO_SERIAL_DEBUGGING > 0
			Serial.print("Line ");
			Serial.print(line);
			Serial.print(": ");
			Serial.println(ptr_databuffer);
#endif
			if(line==1) {
				if(SD.exists(filename))
				{
					Serial.println(PMEM("WARNING: file already used!"));
					Serial.println(PMEM("Remove existing file?"));
					Serial.println(PMEM("Ok = Remove file, Down = Leave file"));
					lcd.clrScr();
					lcd.print("Datei ersetzen?",CENTER,0);
					lcd.print("Ok = Ja",CENTER,8);
					while(!buttonOK)
					{
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
						if(buttonDOWN)
						{
							buttonDOWN=false;
							Serial.println("File NOT removed.");
							server.println("Datei nicht ersetzt.<br />");
							return false;
						}
					}
					buttonOK=false;
					SD.remove(filename);
					Serial.println("Saving new file...");
					lcd.print("Wird gespeichert...",CENTER,8);
					datafile = SD.open(filename, FILE_WRITE);
					delay(10);
				}
				else
				{
					datafile = SD.open(filename, FILE_WRITE);
					delay(10);
				}
			}
			server.println(ptr_databuffer);
			server.println("<br />");
			while(*ptr_databuffer!='\0') {
#if WEBDUINO_SERIAL_DEBUGGING > 0
				Serial.print(*ptr_databuffer);
#endif
				datafile.write(*ptr_databuffer++);
			}
			datafile.write('\n');
			ptr_databuffer = databuffer;
		}
	}
	if(line>0) {
#if WEBDUINO_SERIAL_DEBUGGING > 0
		Serial.println(PMEM("File closed."));
#endif
		datafile.close();
		delay(10);
		Serial.println("written to file.");
		lcd.print("Fertig",CENTER,12);
	}
	return true;
}

/** @brief this function streams data on the SD card to the USART1 port
	@param filename Name of file to stream from SD card to the USART1 port*/
void streamDataToCNC(char *filename)
{
	char buf[80];	// buffer of 80 bytes too big for Arduino Uno
	char lcdTmpBuffer[12];
	int16_t c;
	int16_t index = 0;
	long lineNumber = 0;
	char firstStreamChar;
	char *translatedString = NULL;

	if(CNCcurrentState == READY)
	{
		datafile = SD.open(filename, FILE_READ);
		delay(10);
		Serial.println(PMEM("File open."));
		orderComplete = false;
		CNCnextState = PROCESSING;
	}
	else
	{
		while(c=datafile.read() > 0)
		{
			buf[index++] = (char)(c+77);	// problem in uploaded file?
			while(((c=datafile.read())!=10) && (c!=13)) {	// read file until LF or CRLF (Windows)
				buf[index++] = (char)c;
			}
			buf[index] = '\0';
			Serial.println(buf);
			Serial.println(PMEM("------------"));
			translatedString = cnc.translate(buf);
			if(translatedString[0]=='@')
			{
				Serial.println(translatedString);
				CNCPort.println(translatedString);
#if LOCAL_DEBUG == 1
#else
				lcd.print("Line Nr.:",0,CENTER);
				lcd.print(ltoa(lineNumber++,lcdTmpBuffer,10),8,CENTER);
				waitForCNC();
#endif
				CNCnextState = PROCESSING;
			}
			index = 0;
		}
		datafile.close();
		delay(10);
		orderComplete = true;
	}
}


/*void printDirectory(File dir, int numTabs) {
   while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
   }
   datafile.close();
}*/

/** @brief this function saves the filenames in the data directory in the fileList and returns the number of files.*/
int getFileList()
{
	int numFiles = 0;

	File dir = SD.open("data/");

   while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
		Serial.println("**nomorefiles**");
		entry.close();
		dir.close();
		return numFiles;
     }
     if (!entry.isDirectory()) {
		if(numFiles < 10)
		{
			Serial.println(entry.name());
			fileList[numFiles] = entry.name();
			numFiles++;
		}
		else
		{
			Serial.println(PMEM("**zu viele Dateien**"));
			entry.close();
			dir.close();
			return 9;
		}
	 }
   }
}

/** @brief this function removes all data files on the sd-card.*/
void removeDataFiles() {
	char filename[24] = "data/";

	Serial.println("Removing Datafiles...");
	File dir = SD.open("data/");

	while(true) {
		File entry = dir.openNextFile();
		if(!entry) {
			Serial.println("**nomorefiles**");
			entry.close();
			dir.close();
			return;
		}
		if(!entry.isDirectory()) {
			strcat(filename,entry.name());
			Serial.print("Remove file: ");
			Serial.println(filename);
			SD.remove(filename);
			strcpy(filename,"data/");
		}
	}
}