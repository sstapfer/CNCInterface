void initCNC()
{
	Serial.println(PMEM("--- init ---"));
	CNCPort.println("@07");
	waitForCNC();
	CNCPort.println("@0d2000,2000,2000,0");		// config of axis speeds
	waitForCNC();
	CNCPort.println("@0R7");		// reference movement
}

/*
void moveToTestPoint()
{
	Serial.println(PMEM("--- Moving to Testpoint (X,Y,Z) 1000,-1000,-1000 ---"));
	CNCPort.println("@0M1000,1000,-1000,1000,-1000,1000,0,1000");
	waitForCNC();
	CNCPort.println("@0n3");	// set x- and y-Axis 0 on actual position
	waitForCNC();
	CNCPort.println("@0n4");	// set x- and y-Axis 0 on actual position
	waitForCNC();
	CNCnextState = IDLE;
}
*/
void setXYAxesZero()
{
	Serial.println(PMEM("--- XY Zero ---"));
	CNCPort.println("@0M1800,5000,-39500,5000,0,1000,0,1000");
//	waitForCNC();
}

void moveZAxis()
{
	CNCPort.println("@0n3");	// set x- and y-Axis 0 on actual position
	waitForCNC();
	CNCPort.println("@0M5000,5000,5000,5000,-26000,5000,0,1000");	// prepare for PCB touch
//	waitForCNC();
}

void setZAxisZero()
{
	int ZValue=0;

	Serial.println(PMEM("--- set Z zero ---"));
	Serial.println(analogRead(0));
	// Move down until tool touches PCB surface
	while((ZValue=analogRead(0))>100)
	{
		CNCPort.println("@0A0,1000,0,1000,-10,100,0,1000");		// move z-Axis 10 steps down
		waitForCNC();
		Serial.print(PMEM("Z-Wert: "));
		Serial.println(ZValue);
	}
	delay(1000);
	CNCPort.println("@0n4");		// set z-Axis 0 on actual position
	waitForCNC();
	Serial.println(PMEM("--- move Z 15mm up ---"));
	CNCPort.println("@0A0,5000,0,5000,3000,5000,0,1000");		// move z-Axis 15mm above PCB
//	waitForCNC();
}

void moveToStartPos()
{
	Serial.println(PMEM("--- move to Startpos ---"));
	CNCPort.println("@0M0,5000,0,5000,20000,5000,0,1000");		// move to X,Y=0 Z 100mm above PCB
//	waitForCNC();
}

void sendCNC()
{
	String tmpFilename;

	tmpFilename = "data/";
	tmpFilename += fileList[actualfile];
#if WEBDUINO_SERIAL_DEBUGGING > 0
	Serial.println(tmpFilename);
#endif
	tmpFilename.toCharArray(p_buffer,p_bufferLEN);
	streamDataToCNC(p_buffer);
	if(orderComplete)
	{
		CNCnextState = DONE;
		Serial.println(PMEM("Order complete."));
		ServerBusy = false;
	}
	else
	{
		CNCnextState = PROCESSING;
	}
}

void moveToolAway()
{
	Serial.println(PMEM("--- move Tool away ---"));
	CNCPort.println("@0M0,5000,35000,5000,26000,5000,0,1000");		// move to X=0 Y=175mm Z=130mm above PCB
//	waitForCNC();
}