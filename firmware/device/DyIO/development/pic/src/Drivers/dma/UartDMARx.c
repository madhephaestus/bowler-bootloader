/*
 * UartDMARx.c
 *
 *  Created on: Nov 21, 2011
 *      Author: hephaestus
 */

#include "UserApp.h"

#if defined(USE_DMA)


static DmaChannel	chn = DMA_CHANNEL1;	// DMA channel to use for our example

static int dmaReadPointer = 0;

//static int lastIndex=0;
//static int sameCheck =0;

static BYTE private[DMA_SIZE+2];

static BOOL running=FALSE;
static BOOL abortDump=FALSE;

void closeDma(){
	//println("Closing DMA");
	//lastIndex=0;
	//sameCheck=0;
	//dmaReadPointer = 0;
	stopUartCoProc();
	DmaChnAbortTxfer(chn);
	//INTEnable(INT_SOURCE_DMA(chn), INT_DISABLED);
	DmaChnClrEvFlags(chn,DMA_EV_ALL_EVNTS);
	//DmaChnDisable(chn);

	running=FALSE;
}

void startUartDma(){
	if(running)
		return;
	//println("Starting DMA");
	running=TRUE;
//	lastIndex=0;
//	sameCheck=0;
	dmaReadPointer = 0;
	startUartCoProc();
	DmaChnOpen(chn, DMA_CHN_PRI2, DMA_OPEN_DEFAULT);
	// set the events: we want the UART2 rx interrupt to start our transfer
	// also we want to enable the pattern match: transfer stops upon detection of CR
	DmaChnSetEventControl(chn, 	DMA_EV_START_IRQ_EN|
								//DMA_EV_MATCH_EN|
								DMA_EV_START_IRQ(_UART2_RX_IRQ));

	// set the transfer source and dest addresses, source and dest sizes and the cell size
	DmaChnSetTxfer(chn, (void*)&U2RXREG, private, 1, DMA_SIZE, 1);

	DmaChnSetEvEnableFlags(chn, DMA_EV_BLOCK_DONE);		// enable the transfer done interrupt

	// enable system wide multi vectored interrupts
	INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
	INTEnableInterrupts();

	INTSetVectorPriority(INT_VECTOR_DMA(chn), INT_PRIORITY_LEVEL_5);		// set INT controller priority
	INTSetVectorSubPriority(INT_VECTOR_DMA(chn), INT_SUB_PRIORITY_LEVEL_3);		// set INT controller sub-priority

	INTEnable(INT_SOURCE_DMA(chn), INT_ENABLED);		// enable the chn interrupt in the INT controller

	// enable the chn
	DmaChnEnable(chn);
	EndCritical();
	//FLAG_ASYNC=FLAG_OK;
}

int dump(int from , int to){
	if(from >= to)
		return 0;
	//println("Adding bytes from: " );p_ul(from);print(" to: ");p_ul(to);
	int i;
	int num=0;
	for(i=from;i<to;i++){
		if(abortDump==TRUE){
			//println("Aborting dump" );
			return num;
		}
		//INTEnable(INT_SOURCE_DMA(chn), INT_DISABLED);
		addCoProcByte(private[dmaReadPointer++]);
		//INTEnable(INT_SOURCE_DMA(chn), INT_ENABLED);		// enable the chn interrupt in the INT controller
		num++;
	}
	return num;
}
BOOL skip = FALSE;
int pushContents(){
	abortDump=FALSE;
	int from = dmaReadPointer;
	int to = DmaChnGetDstPnt(chn);
	if(to>DMA_SIZE-10 && skip == FALSE){
		skip = TRUE;
		return 0;
	}
	if(to>from ){
		skip = FALSE;
		//closeDma();
		int back = dump(from,to);
		//startUartDma();

		return back;
	}else{
		skip = FALSE;
		return 0;
	}
}

int updateUartDmaRx(){
	startUartDma();
	uartErrorCheck();
	int numAdded=0;
//	int got = 	DmaChnGetDstPnt(chn);
//	if((got == lastIndex) || got==DMA_SIZE){
//		if(got>0){
//			sameCheck++;
//			if(sameCheck>2|| got==DMA_SIZE){
//				//println("DMA got:");p_ul(got);//print(" Data [");
//				//StartCritical();
//				numAdded = pushContents();
//				sameCheck=0;
//			}else{
//				//DelayMs(1);
//			}
//		}
//	}else{
//		//DelayMs(1);
//		lastIndex = got;
//	}
//	//EndCritical();
	numAdded = pushContents();
	return numAdded;
}

// handler for the DMA channel 1 interrupt
void __ISR(_DMA1_VECTOR, IPL5SOFT) DmaHandler1(void)
{
	int	evFlags;				// event flags when getting the interrupt

	INTClearFlag(INT_SOURCE_DMA(chn));	// release the interrupt in the INT controller, we're servicing int

	evFlags=DmaChnGetEvFlags(chn);	// get the event flags

    if(evFlags&DMA_EV_BLOCK_DONE)
    { // just a sanity check. we enabled just the DMA_EV_BLOCK_DONE transfer done interrupt
    	//println("Maxed out DMA buffer, resetting");
		FLAG_ASYNC=FLAG_BLOCK;
    	//closeDma();
    	//startUartDma();

    	DmaChnAbortTxfer(chn);

    	DmaChnSetTxfer(chn, (void*)&U2RXREG, private, 1, DMA_SIZE, 1);
    	DmaChnSetEvEnableFlags(chn, DMA_EV_BLOCK_DONE);		// enable the transfer done interrupt
    	DmaChnEnable(chn);
    	int size = dmaReadPointer;
    	dump(dmaReadPointer,DMA_SIZE);
		while(DataRdyUART2()){
			addCoProcByte(UARTGetDataByte(UART2));
			buttonCheck(33);
		}
    	//println("Maxed out DMA buffer, resetting from: " );p_ul(size);print("\n");
    	dmaReadPointer=0;


    	abortDump=TRUE;
    	//print("`");
    	FLAG_ASYNC=FLAG_OK;


    }
}
#endif
