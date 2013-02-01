/*int getUsers()
{
	int16_t c;
	int index=0, users=0;

	if (!SD.exists("config/users.ini")) {
		Serial.println(PMEM("users.ini not found. Insert SD card."));
	}
	else {
		Serial.println(PMEM("reading config..."));
		datafile = SD.open("config/users.ini", FILE_READ);
		delay(10);
		while((c=datafile.read()) > 0) {	// read file until EOF
			switch(c) {
			case 10:	// line feed
				userTable[users++][index] = '\0';
				index = 0;
				break;
			default:
				userTable[users][index++] = (char)c;
			}
			delay(1);
		}
		userTable[users++][index] = '\0';
	}
	return users;
}*/

int getUserData()
{
	char buf[64];
	char decoded[64];
	int16_t c;
	int index=0;
	char username[30];
	char *ptr_username = username;
	char password[30];
	char *ptr_password = password;

	if (!SD.exists("config/users.ini")) {
		Serial.println(PMEM("users.ini not found. Insert SD card."));
	}
	else {
		Serial.println(PMEM("reading config..."));
		datafile = SD.open("config/users.ini", FILE_READ);
		delay(10);
		while((c=datafile.read()) > 0) {	// read file until EOF
			switch(c) {
			case 10:	// line feed
				buf[index] = '\0';
				base64_decode(decoded,buf,index);
//				Serial.println(decoded);
				ptr_username = strtok(decoded,":");
				ptr_password = strtok(NULL,"\n\t");
				index = 0;
				break;
			default:
				buf[index++] = (char)c;
			}
			delay(1);
		}
		buf[index] = '\0';
		base64_decode(decoded,buf,index);
//		Serial.println(decoded);
		ptr_username = strtok(decoded,":");
		ptr_password = strtok(NULL,"\n\t");
//		Serial.println(ptr_username);
//		Serial.println(ptr_password);
		datafile.close();
	}
	Serial.println(PMEM("...done"));
	free(buf);
	free(decoded);
}