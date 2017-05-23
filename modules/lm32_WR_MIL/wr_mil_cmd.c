#include <wr_mil_cmd.h>
#include "mini_sdb.h"
#include "mprintf.h"

extern volatile uint32_t _startshared[]; // provided in linker script "ram.ld"
#define SHARED __attribute__((section(".shared")))
uint64_t SHARED dummy = 0; // not sure if this is really needed... 

volatile MilCmdRegs *MilCmd_init()
{
  volatile MilCmdRegs *cmd = (volatile MilCmdRegs*)_startshared;
  cmd->cmd = 0;
  return cmd;
}

// check if the command (cmd) register is != 0, 
//  do stuff according to its value, and set it 
//  back to 0
void MilCmd_poll(volatile MilCmdRegs *cmd)
{
  if (cmd->cmd)
  {
    switch(cmd->cmd)
    {
      case 0x00000001:
        mprintf("stop MCU\n");
        while(1);
      default:
        mprintf("found command %08x\n", cmd->cmd);
        break;
    }
    cmd->cmd = 0;
  }   
}
