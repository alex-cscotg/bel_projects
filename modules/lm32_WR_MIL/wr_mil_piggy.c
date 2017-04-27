#include "wr_mil_piggy.h"

#define   MIL_RD_WR_DATA      0x0000    // read or write mil bus; only 32bit access alowed; data[31..16] don't care
#define   MIL_WR_CMD          0x0001    // write command to mil bus; only 32bit access alowed; data[31..16] don't care
#define   MIL_WR_RD_STATUS    0x0002    // read mil status register; only 32bit access alowed; data[31..16] don't care
                                        // write mil control register; only 32bit access alowed; data[31..16] don't care
#define   RD_CLR_NO_VW_CNT    0x0003    // use only when FPGA Manchester Endecoder is enabled;
                                        // read => valid word error counters; write => clears valid word error counters
#define   RD_WR_NOT_EQ_CNT    0x0004    // use only when FPGA Manchester Endecoder is enabled;
                                        // read => rcv pattern not equal errors; write => clears rcv pattern not equal errors
#define   RD_CLR_EV_FIFO      0x0005    // read => event fifo; write clears event fifo.
#define   RD_CLR_TIMER        0x0006    // read => event timer; write clear event timer.
#define   RD_WR_DLY_TIMER     0x0007    // read => delay timer; write set delay timer.
#define   RD_CLR_WAIT_TIMER   0x0008    // read => wait timer; write => clear wait timer.

#define   EV_FILT_FIRST       0x1000    // first event filter (ram) address.
#define   EV_FILT_LAST        0x1FFF    // last event filter (ram) addres.



/***********************************************************
 * 
 * mil registers available via Wishbone
 * 
 ***********************************************************/
#define   MIL_REG_RD_WR_DATA         MIL_RD_WR_DATA   << 2  // read or write mil bus; only 32bit access alowed; data[31..16] don't care
#define   MIL_REG_WR_CMD             MIL_WR_CMD       << 2  // write command to mil bus; only 32bit access alowed; data[31..16] don't care
#define   MIL_REG_WR_RD_STATUS       MIL_WR_RD_STATUS << 2  // read mil status register; only 32bit access alowed; data[31..16] don't care
                                                            // write mil control register; only 32bit access alowed; data[31..16] don't care
#define   MIL_REG_RD_CLR_NO_VW_CNT   RD_CLR_NO_VW_CNT << 2  // use only when FPGA Manchester Endecoder is enabled;
                                                            // read => valid word error counters; write => clears valid word error counters
#define   MIL_REG_RD_WR_NOT_EQ_CNT   RD_WR_NOT_EQ_CNT << 2  // use only when FPGA Manchester Endecoder is enabled;
                                                            // read => rcv pattern not equal errors; write => clears rcv pattern not equal errors
#define   MIL_REG_RD_CLR_EV_FIFO     RD_CLR_EV_FIFO << 2    // read => event fifo; write clears event fifo.
#define   MIL_REG_RD_CLR_TIMER       RD_CLR_TIMER << 2      // read => event timer; write clear event timer.
#define   MIL_REG_RD_WR_DLY_TIMER    RD_WR_DLY_TIMER << 2   // read => delay timer; write set delay timer.
#define   MIL_REG_RD_CLR_WAIT_TIMER  RD_CLR_WAIT_TIMER << 2 // read => wait timer; write => clear wait timer.
#define   MIL_REG_WR_RF_LEMO_CONF    0x0024                 // read => mil lemo config; write ==> mil lemo config (definition of bits: see below)
#define   MIL_REG_WR_RD_LEMO_DAT     0x0028                 // read => mil lemo data; write ==> mil lemo data
#define   MIL_REG_RD_LEMO_INP_A      0x002C                 // read mil lemo input
#define   MIL_REG_EV_FILT_FIRST      EV_FILT_FIRST << 2     // first event filter (ram) address.
#define   MIL_REG_EV_FILT_LAST       EV_FILT_LAST << 2      // last event filter (ram) addres.
                                                            // the filter RAM has a size of 4096 elements
                                                            // each element has a size of 4 bytes
                                                            // (definition of filter bits: see below)
                                                            // the filter RAM is addressed in the following way
                                                            //    uint32_t *pFilter; 
                                                            //    pFilter[virtAcc * 256 + evtCode]





void MilPiggy_init(MilPiggy_t *piggy, uint32_t *device_addr)
{
	piggy->pMilPiggy      = device_addr;
	piggy->pReadWriteData = piggy->pMilPiggy + (MIL_REG_RD_WR_DATA >> 2);
	piggy->pWriteCmd      = piggy->pMilPiggy + (MIL_REG_WR_CMD     >> 2);
}

/* write the lower 16 bit of cmd to Mil device bus */
void MilPiggy_writeCmd(MilPiggy_t *piggy, uint32_t cmd)
{
	*piggy->pWriteCmd = cmd;
}
