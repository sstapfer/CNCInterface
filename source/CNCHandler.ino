/** @file CNCHandler.ino
	@brief These functions are used in the milling process to move the CNC and send the new orders.*/

/** this function initializes the CNC machine by\n
	- defining the axes XYZ\n
	- the speed of the different axes\n
	- doing a reference movement*/
void initCNC()
{
	Serial.println(PMEM("--- init ---"));
	CNCPort.println("@07");// definition of axes to use: X,Y and Z
	waitForCNC();
	CNCPort.println("@0d2000,2000,2000,0");// config of axis speeds
	waitForCNC();
	CNCPort.println("@0R7");// reference movement
}

/** this function changes the X and Y origin of the coordinate system to the corner in front on the left side*/
void setXYAxesZero()
{
	Serial.println(PMEM("--- XY Zero ---"));
	CNCPort.println("@0M4500,5000,-37600,5000,0,1000,0,1000");		// <--- change X and Y here to set coordinate origin of PCB
}

/** this function moves the drilling head 10 mm above the PCB surface and 25mm in X and Y direction*/
void moveZAxis()
{
	CNCPort.println("@0n3");// set x- and y-Axis 0 on actual position
	waitForCNC();
	CNCPort.println("@0M5000,5000,5000,5000,-26000,5000,0,1000");// prepare for PCB touch
}

/** this function moves the drilling head down, until it touches the PCB. The new position is saved as Z = 0.\n
	After resetting the Z position the head is moved away from the PCB.*/
void setZAxisZero()
{
	int ZValue=0;

	Serial.println(PMEM("--- set Z zero ---"));
	Serial.println(analogRead(0));

	// Move down until tool touches PCB surface
	while((ZValue=analogRead(0))>100)
	{
		CNCPort.println("@0A0,1000,0,1000,-10,100,0,1000");// move z-Axis 10 steps down
		waitForCNC();
		Serial.print(PMEM("Z-Wert: "));
		Serial.println(ZValue);
	}
	delay(1000);
	CNCPort.println("@0n4");// set z-Axis 0 on actual position
	waitForCNC();
	Serial.println(PMEM("--- move Z 15mm up ---"));
	CNCPort.println("@0A0,5000,0,5000,3000,5000,0,1000");// move z-Axis 15mm above PCB
}

/** this function moves away the head at about 100mm above PCB*/
void moveToStartPos()
{
	Serial.println(PMEM("--- move to Startpos ---"));
	CNCPort.println("@0M0,5000,0,5000,20000,5000,0,1000");// move to X,Y=0 Z 100mm above PCB
}

/** this function is called to send the next order to the CNC machine. If there is no new line the order is\n
	completed and the state changes to Done.*/
void sendCNC()
{
	String tmpFilename;

	tmpFilename = "data/";
	tmpFilename += fileList[actualFile];
#if WEBDUINO_SERIAL_DEBUGGING > 0
	Serial.println(tmpFilename);
#endif
	tmpFilename.toCharArray(p_buffer,p_bufferLEN);
	streamDataToCNC(p_buffer);
	if(orderComplete)
	{
		CNCnextState = DONE;
		Serial.println(PMEM("Order complete."));
		serverBusy = false;
	}
	else
	{
		CNCnextState = PROCESSING;
	}
}

/** this function moves the table in front to the user and the drilling head up.*/
void moveToolAway()
{
	Serial.println(PMEM("--- move Tool away ---"));
	CNCPort.println("@0M0,5000,35000,5000,26000,5000,0,1000");// move to X=0 Y=175mm Z=130mm above PCB
}