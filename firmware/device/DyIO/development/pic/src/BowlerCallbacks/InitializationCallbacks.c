/*
 * UserInit.c
 *
 *  Created on: Nov 29, 2009
 *      Author: hephaestus
 */

#include "UserApp.h"

//BYTE InputState[NUM_PINS];

BYTE SaveTheState=0;

#define mInitSwitch()      (_TRISB0)=1;

const BYTE MY_MAC_ADDRESS[]={0x74,0xf7,0x26,0x01,0x01,0x01};

extern MAC_ADDR MyMAC __attribute__ ((section (".scs_global_var")));

void hardwareInit(){
	StartCritical();
	FlashGetMac(MyMAC.v);
	char macStr[13];
	int j=0,i=0;
	for (i=0;i<6;i++){
		macStr[j++]=GetHighNib(MyMAC.v[i]);
		macStr[j++]=GetLowNib(MyMAC.v[i]);
	}
	macStr[12]=0;
	println_I("MAC address is =");
	print_I(macStr);
#if defined(ROBOSUB_DEMO)
	//char * dev = "AHD Wave";
#else
	char * dev = "DyIO v.3";
#endif

	usb_CDC_Serial_Init(dev,macStr,0x04D8,0x3742);
	mInitSwitch();

	for (i=0;i<6;i++){
		MyMAC.v[i]= MY_MAC_ADDRESS[i];
	}

	Init_FLAG_BUSY_ASYNC();
	//InitCTS_RTS_HO();

	//AVR Reset pin
	InitAVR_RST();
	HoldAVRReset();

	//ConfigUARTOpenCollector();
	ConfigUARTRXTristate();


	InitLEDS();
	SetColor(0,0,1);
	//Starts Timer 3
	InitCounterPins();
	InitADC();
	println_I("Adding DyIO namespaces:");


	addNamespaceToList((NAMESPACE_LIST * )get_bcsIoNamespace());
	addNamespaceToList((NAMESPACE_LIST * )get_bcsIoSetmodeNamespace());
	addNamespaceToList((NAMESPACE_LIST * )get_neuronRoboticsDyIONamespace());
	addNamespaceToList((NAMESPACE_LIST * )get_bcsPidDypidNamespace());
	addNamespaceToList((NAMESPACE_LIST * )get_bcsSafeNamespace());
	addNamespaceToList((NAMESPACE_LIST * )getBcsPidNamespace());



	BYTE rev [] = {MAJOR_REV,MINOR_REV,FIRMWARE_VERSION};
	FlashSetFwRev(rev);

	//Starts co-proc uart
	initCoProcCom();

	EndCritical();
	INTEnableSystemMultiVectoredInt();

	initBluetooth();
	if(!hasBluetooth()){
		Pic32UARTSetBaud( 115200 );
	}

}

void UserInit(void){
	//setPrintStream(&USBPutArray);
	setPrintLevelInfoPrint();
	println_I("Starting PIC initialization");
	hardwareInit();
	println_I("Hardware Init done");

	ReleaseAVRReset();

	InitPins();

	CheckRev();

	LoadEEstore();

	LoadDefaultValues();

	CartesianControllerInit();

	InitPID();

	UpdateAVRLED();

	//println_I("Syncing modes:");
	SyncModes();
	//println_I("Setting modes:");


	lockServos();
	setPrintLevelInfoPrint();
	println_I("###Starting PIC In Debug Mode###\n");// All printfDEBUG functions do not need to be removed from code if debug is disabled
	//setPrintLevelErrorPrint();
	println_E("Error level printing");
	println_W("Warning level printing");
	println_I("Info level printing");
	BOOL brown = getEEBrownOutDetect();
	setCoProcBrownOutMode(brown);
	setBrownOutDetect(brown);

}

