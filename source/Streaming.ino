void streamFileToServer(char *filename)
{
#if (MEGA_IS_USED > 0)
	char buf[128];	// buffer too big for Arduino Uno
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
#else
	int16_t c;	

	server.println("<html>");
	datafile = SD.open(filename, FILE_READ);
	delay(10);
	while((c=datafile.read()) > 0) {	// read file until EOF
		server.print((char)c);
		delay(1);
	}
	server.print("</html>");
	datafile.close();
	delay(10);
#endif
	free(buf);
}

bool streamDataToFile()
{
	int ch=0, line=0;
	char databuffer[80];
	char *ptr_databuffer = databuffer;
	char filename[24] = "data/";
	char tmpFilename[24];
	char *ptr_filename = tmpFilename;

#if WEBDUINO_SERIAL_DEBUGGING > 0
	Serial.println("----------Header------------");
#endif
	// remove header
	server.readHeader(databuffer,80);
	server.readHeader(databuffer,80);

	/** get Filename */
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
	Serial.println("----------Params------------");
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
					Serial.println("WARNING: file already used!");
					Serial.println("Remove existing file?");
					lcd.clrScr();
					lcd.print("Datei ersetzen?",CENTER,0);
					server.println(PMEM("<p>ACHTUNG: Datei schon vorhanden! Zum ersetzen OK drücken.</p>"));
					while(!buttonOK)
					{
						if(buttonDOWN)
						{
							buttonDOWN=false;
							Serial.println("File NOT removed.");
							return false;
						}
					}
					buttonOK=false;
					SD.remove(filename);
					Serial.println("Saving new file.");
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
		Serial.println("File closed.");
#endif
		datafile.close();
		delay(10);
		Serial.println("written to file.");
		lcd.print("Fertig",CENTER,12);
	}
	return true;
}

void streamDataToCNC(char *filename)
{
	char buf[100];	// buffer too big for Arduino Uno
	int16_t c;
	int16_t index = 0;
	char firstStreamChar;

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
			while((c=datafile.read()) != 10) {	// read file until line feed
				buf[index++] = (char)c;
			}
			buf[index] = '\0';
			Serial.println(buf);
			Serial.println(PMEM("------------"));
			firstStreamChar=*cnc.translate(buf);
			if(firstStreamChar=='@')
			{
				Serial.println(cnc.translate(buf));
				CNCPort.println(cnc.translate(buf));
				waitForCNC();
				CNCnextState = PROCESSING;
			}
			index = 0;
		}
		buf[index] = '\0';
		datafile.close();
		delay(10);
		orderComplete = true;
	}
}

void printDirectory(File dir, int numTabs) {
   while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
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
}

int getFileList()
{
	int numFiles = 0;

	File dir = SD.open("data/");

   while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
		// no more files
		//Serial.println("**nomorefiles**");
		entry.close();
		dir.close();
		return numFiles;
     }
     if (!entry.isDirectory()) {
		if(numFiles < 10)
		{
			Serial.println(entry.name());
			lcd.print(entry.name(),LEFT,numFiles*8);
			fileList[numFiles] = entry.name();
			numFiles++;
		}
		else
		{
			Serial.println("**too much files**");
			entry.close();
			dir.close();
			return 9;
		}
	 }
   }
}
