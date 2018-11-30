/*!
 *  @file daq.h
 *  @brief Control module for Data Acquisition Unit (DAQ)
 *  @date 13.11.2018
 *  @copyright (C) 2018 GSI Helmholtz Centre for Heavy Ion Research GmbH
 *
 *  @author Ulrich Becker <u.becker@gsi.de>
 *
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */
#ifndef _DAQ_H
#define _DAQ_H

#include "scu_bus.h"
#include "daq_descriptor.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * For the reason LM32 RAM consume can be reduced here is the possibility
 * to overwrite DAQ_MAX by the Makefile.
 */
#ifndef DAQ_MAX
   #define DAQ_MAX MAX_SCU_SLAVES //!< @brief Maximum number of DAQ's
#else
  #if DAQ_MAX > MAX_SCU_SLAVES
    #error Macro DAQ_MAX can not be greater than MAX_SCU_SLAVES !
  #endif
#endif

#ifndef DAQ_MAX_CHANNELS
  #define DAQ_MAX_CHANNELS 16 //!< @brief Maximum number of DAQ-channels
#else
  #if DAQ_MAX_CHANNELS > 16
    #error Not more than 16 channels per DAQ
  #endif
#endif


/*!
 * @brief Access indexes for writing and reading the DAQ-registers
 * @see daqChannelGetReg
 * @see daqChannelSetReg
 */
typedef enum
{
   CtrlReg         = 0x00, //!< @see DAQ_CTRL_REG_T
   TRIG_LW         = 0x10, //!< @brief Least significant word for SCU-bus event
                           //!<        tag tregger condition.
   TRIG_HW         = 0x20, //!< @brief Most significant word for SCU-bus event
                           //!<        tag tregger condition.
   TRIG_DLY        = 0x30, //!< @brief Trigger delay in samples.
   PM_DAT          = 0x40, //!< @brief Data of PostMortem respectively HiRes-FiFos.
   DAQ_DAT         = 0x50, //!< @brief Data of DAQ-FiFo.
   DAQ_FIFO_WORDS  = 0x70, //!< @brief Remaining FiFo word of DaqDat FiFo.
   PM_FIFO_WORDS   = 0x80  //!< @brief Remaining FiFo word of PmDat FiFo.
} DAQ_REGISTER_INDEX;

/*!
 * @brief Values for single bit respectively flag in bit-field structure.
 * @see DAQ_CTRL_REG_T
 */
typedef enum
{
   OFF =  0, //!< @brief Value for switch on.
   ON  =  1  //!< @brief Value for switch off.
} DAQ_SWITCH_STATE_T;

/*!
 * @brief Data type of the control register for each channel.
 * @note Do not change the order of attributes! Its a Hardware image!
 * @see CtrlReg
 * @see DAQ_SWITCH_STATE_T
 */
typedef volatile struct
{
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || defined(__DOXYGEN__)
   uint16_t slot:                  4; //!< @brief Bit [15:12] slot number,
                                      //!<        shall be initialized by software,
                                      //!<        will used for the DAQ-Descriptor-Word.
   uint16_t __unused__:            4; //!< @brief Bit [11:8] till now unused.
   uint16_t ExtTrig_nEvTrig_HiRes: 1; //!< @brief Bit [7] trigger source in high resolution mode
                                      //!<        1= external trigger, event trigger.
   uint16_t Ena_HiRes:             1; //!< @brief Bit [6] high resolution sampling with 4 MHz.
                                      //!< @note Ena_PM shall not be active at the same time!
   uint16_t ExtTrig_nEvTrig:       1; //!< @brief Bit [5] trigger source in DAQ mode:
                                      //!<        1=ext trigger, 0= event trigger.
   uint16_t Ena_TrigMod:           1; //!< @brief Bit [4] prevents sampling till triggering.
   uint16_t Sample1ms:             1; //!< @brief Bit [3] use 1 ms sample.
   uint16_t Sample100us:           1; //!< @brief Bit [2] use 100 us sample.
   uint16_t Sample10us:            1; //!< @brief Bit [1] use 10 us sample.
   uint16_t Ena_PM:                1; //!< @brief Bit [0] starts PM sampling with 100 us.
#else
   #error Big endian is requested for this bit- field structure!
#endif
} DAQ_CTRL_REG_T;
STATIC_ASSERT( sizeof(DAQ_CTRL_REG_T) == sizeof(uint16_t) );

/*!
 * @brief Data type of DAQ register "Daq_Fifo_Words"
 * @see DAQ_FIFO_WORDS
 */
typedef volatile struct
{
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || defined(__DOXYGEN__)
   uint16_t version:     7; //!< @brief Version number of DAQ macro.
   uint16_t fifoWords:   9; //!< @brief Remaining data words in PmDat Fifo
#else
   #error Big endian is requested for this bit- field structure!
#endif
} DAQ_DAQ_FIFO_WORDS_T;
STATIC_ASSERT( sizeof(DAQ_DAQ_FIFO_WORDS_T) == sizeof(uint16_t) );

/*!
 * @brief Data type of DAQ register "PM_Fifo_Words"
 * @see PM_FIFO_WORDS
 */
typedef volatile struct
{
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) || defined(__DOXYGEN__)
   uint16_t maxChannels: 6; //!< @brief Maximum number of used channels
   uint16_t fifoWords:  10; //!< @brief Remaining data words in PmDat Fifo
#else
   #error Big endian is requested for this bit- field structure!
#endif
} DAQ_PM_FIFO_WORDS_T;
STATIC_ASSERT( sizeof(DAQ_PM_FIFO_WORDS_T) == sizeof(uint16_t) );


/*!
 * @brief Relative start address of the SCU registers
 *
 * Accessing to a SCU register will made as followed:
 * Absolute-register-address = SCU-bus-slave base_address + DAQ_REGISTER_OFFSET
 *                           + canal_number * sizeof(uint16_t)
 */
#define DAQ_REGISTER_OFFSET 0x4000

struct DAQ_DATA_T
{
   //TODO
};

/*!
 * @brief Memory mapped IO-space of a DAQ macro
 */
typedef volatile union
{
   volatile uint16_t i[(SCUBUS_SLAVE_ADDR_SPACE-DAQ_REGISTER_OFFSET)/sizeof(uint16_t)];
   volatile struct DAQ_DATA_T s;
} DAQ_REGISTER_T;
STATIC_ASSERT( sizeof( DAQ_REGISTER_T ) == (SCUBUS_SLAVE_ADDR_SPACE-DAQ_REGISTER_OFFSET) );

/*!
 * @brief Flag pool of channel properties
 */
typedef struct
{
   bool notUsed; //!< @brief When true than this channel will not used.
} DAQ_CHANNEL_BF_PROPERTY_T;
STATIC_ASSERT( sizeof( DAQ_CHANNEL_BF_PROPERTY_T ) == sizeof( uint8_t ) );

/*!
 * @brief Object represents a single channel of a DAQ.
 */
typedef struct
{
   uint8_t                   n; //!< @brief Channel number [0..DAQ_MAX_CHANNELS-1]
   DAQ_CHANNEL_BF_PROPERTY_T properties; //!<@see DAQ_CHANNEL_PROPERTY_T
   void* p; //TODO pointer ti FoFo
} DAQ_CANNEL_T;

/*!
 * @brief Object represents a single SCU-Bus slave including a DAQ
 */
typedef struct
{
   unsigned int maxChannels; //!< @brief Number of DAQ-channels
   DAQ_CANNEL_T aChannel[DAQ_MAX_CHANNELS]; //!< @brief Array of channel objects
   DAQ_REGISTER_T* volatile pReg; //!< @brief Pointer to DAQ-registers (start of address space)
} DAQ_DEVICE_T;

/*!
 * @brief Object represents all on the SCU-bus connected DAQs.
 */
typedef struct
{
   unsigned int foundDevices;  //!< @brief Number of found DAQs
   DAQ_DEVICE_T aDaq[DAQ_MAX]; //!< @brief Array of all possible existing DAQs
} DAQ_BUS_T;

/*======================== DAQ channel functions ============================*/
/*! ---------------------------------------------------------------------------
 * @brief Returns the pointer to the control register of a given DAQ-channel
 * @note <p>CAUTION!</p>\n
 *       This function works only if the channel object (pThis) is
 *       real content of the container DAQ_DEVICE_T. \n
 *       That means, the address range of the concerned object DAQ_CANNEL_T
 *       has to be within the address range of its container-object
 *       DAQ_DEVICE_T!
 * @see CONTAINER_OF defined in helper_macros.h
 * @param pThis Pointer to the channel object
 * @return Pointer to the control register.
 */
static inline DAQ_REGISTER_T* volatile daqChannelGetRegPtr( register DAQ_CANNEL_T* pThis )
{
   LM32_ASSERT( pThis->n < DAQ_MAX_CHANNELS );
   return CONTAINER_OF( pThis, DAQ_DEVICE_T, aChannel[pThis->n] )->pReg;
}

/*! --------------------------------------------------------------------------
 * @brief Macro makes the index for memory mapped IO of the DQQ register space
 * @note For internal use only!
 * @see DAQ_REGISTER_T
 */
#define __DAQ_MAKE_INDEX(index) (index | pThis->n)

/*! ---------------------------------------------------------------------------
 * @brief DAQ-register access helper macro for get- and set- functions
 *        of the DAQ- registers.
 * @param index Register index name @see DAQ_REGISTER_INDEX
 * @note For internal use only!
 */
#define __DAQ_GET_REG( index ) (daqChannelGetRegPtr(pThis)->i[__DAQ_MAKE_INDEX(index)])

/*!
 * @brief Macro verifies whether the access is within the allowed IO- range
 */
#ifdef CONFIG_DAQ_PEDANTIC_CHECK
  #define __DAQ_VERIFY_REG_ACCESS( index ) \
      LM32_ASSERT( __DAQ_MAKE_INDEX(index) < sizeof(DAQ_REGISTER_T) )
#else
  #define __DAQ_VERIFY_REG_ACCESS( index )
#endif

/*! ---------------------------------------------------------------------------
 * @brief Returns a pointer to the control-register of the given channel-
 *        object.
 * @see daqChannelGetRegPtr
 * @see DAQ_CTRL_REG_T
 * @param pThis Pointer to the channel object
 * @return Pointer to control register bit field structure.
 */
ALWAYS_INLINE
static inline DAQ_CTRL_REG_T* daqChannelGetCtrlRegPtr( register DAQ_CANNEL_T* pThis )
{
   LM32_ASSERT( pThis != NULL );
   __DAQ_VERIFY_REG_ACCESS( CtrlReg );
   return (DAQ_CTRL_REG_T*) &__DAQ_GET_REG( CtrlReg );
}

/*! ---------------------------------------------------------------------------
 * @brief Get the slot number of the given DAQ-channel.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @return Slot number of the DAQ device where belonging this channel
 */
ALWAYS_INLINE
static inline int daqChannelGetSlot( register DAQ_CANNEL_T* pThis )
{
   return daqChannelGetCtrlRegPtr( pThis )->slot;
}

/*! ---------------------------------------------------------------------------
 * @brief Get the channel number in a range of 0 to 15 of this channel object.
 * @param pThis Pointer to the channel object.
 * @return Channel number.
 */
ALWAYS_INLINE
static inline int daqChannelGetNumber( const register DAQ_CANNEL_T* pThis )
{
   return pThis->n;
}

/*! ---------------------------------------------------------------------------
 * @brief Turns the 10 us sample mode on for the concerned channel.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline void daqChannelSample10usOn( register DAQ_CANNEL_T* pThis )
{
   DAQ_CTRL_REG_T* pCtrl = daqChannelGetCtrlRegPtr( pThis );
   pCtrl->Sample1ms = OFF;
   pCtrl->Sample100us = OFF;
   pCtrl->Sample10us = ON;
}

/*! ---------------------------------------------------------------------------
 * @brief Turns the 10 us sample mode off for the channel-object.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @param channel Channel number.
 */
static inline void daqChannelSample10usOff( register DAQ_CANNEL_T* pThis )
{
   daqChannelGetCtrlRegPtr( pThis )->Sample10us = OFF;
}

/*! ---------------------------------------------------------------------------
 * @brief Queries whether 10 us sample mode is active
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @retval true is active
 * @retval false is not active
 */
static inline bool daqChannelIsSample10usActive( register DAQ_CANNEL_T* pThis )
{
   return (daqChannelGetCtrlRegPtr( pThis )->Sample10us == ON);
}


/*! ---------------------------------------------------------------------------
 * @brief Turns the 100 us sample mode on for the concerned channel.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline void daqChannelSample100usOn( register DAQ_CANNEL_T* pThis )
{
   DAQ_CTRL_REG_T* pCtrl = daqChannelGetCtrlRegPtr( pThis );
   pCtrl->Sample10us = OFF;
   pCtrl->Sample1ms = OFF;
   pCtrl->Sample100us = ON;
}

/*! ---------------------------------------------------------------------------
 * @brief Turns the 100 us sample mode off for the concerned channel.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline void daqChannelSample100usOff( register DAQ_CANNEL_T* pThis )
{
   daqChannelGetCtrlRegPtr( pThis )->Sample100us = OFF;
}

/*! ---------------------------------------------------------------------------
 * @brief Queries whether 100 us sample mode is active
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @retval true is active
 * @retval false is not active
 */
static inline bool daqChannelIsSample100usActive( register DAQ_CANNEL_T* pThis )
{
   return (daqChannelGetCtrlRegPtr( pThis )->Sample100us == ON);
}

/*! ---------------------------------------------------------------------------
 * @brief Turns the 1 ms us sample mode on for the concerned channel.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline void daqChannelSample1msOn( register DAQ_CANNEL_T* pThis )
{
   DAQ_CTRL_REG_T* pCtrl = daqChannelGetCtrlRegPtr( pThis );
   pCtrl->Sample10us = OFF;
   pCtrl->Sample100us = OFF;
   pCtrl->Sample1ms = ON;
}

/*! ---------------------------------------------------------------------------
 * @brief Turns the 1 ms sample mode off for the concerned channel.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline void daqChannelSample1msOff( register DAQ_CANNEL_T* pThis )
{
   daqChannelGetCtrlRegPtr( pThis )->Sample1ms = OFF;
}

/*! ---------------------------------------------------------------------------
 * @brief Queries whether 1 ms sample mode is active
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @retval true is active
 * @retval false is not active
 */
static inline bool daqChannelIsSample1msActive( register DAQ_CANNEL_T* pThis )
{
   return (daqChannelGetCtrlRegPtr( pThis )->Sample1ms == ON);
}

/*! ---------------------------------------------------------------------------
 * @brief Enables the trigger mode of the concerning channel.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline void daqChannelEnableTriggerMode( register DAQ_CANNEL_T* pThis )
{
   daqChannelGetCtrlRegPtr( pThis )->Ena_TrigMod = ON;
}

/*! ---------------------------------------------------------------------------
 * @brief Disables the trigger mode of the concerning channel.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline void daqChannelDisableTriggerMode( register DAQ_CANNEL_T* pThis )
{
   daqChannelGetCtrlRegPtr( pThis )->Ena_TrigMod = OFF;
}

/*! ---------------------------------------------------------------------------
 * @brief Queries whether the trigger mode of the concerning channel is active.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @retval true is active
 * @retval false is not active
 */
static inline bool daqChannelIsTriggerModeEnabled( register DAQ_CANNEL_T* pThis )
{
   return (daqChannelGetCtrlRegPtr( pThis )->Ena_TrigMod == ON);
}

/*! ---------------------------------------------------------------------------
 * @brief Enabling external trigger source
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline void daqChannelEnableExtrenTrigger( register DAQ_CANNEL_T* pThis )
{
   daqChannelGetCtrlRegPtr( pThis )->ExtTrig_nEvTrig = ON;
}

/*! ---------------------------------------------------------------------------
 * @brief Enable event trigger
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline void daqChannelEnableEventTrigger( register DAQ_CANNEL_T* pThis )
{
   daqChannelGetCtrlRegPtr( pThis )->ExtTrig_nEvTrig = OFF;
}

/*! ---------------------------------------------------------------------------
 * @brief Returns the trigger source of the given channel object
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @retval true Channel is in external trigger mode
 * @retval false Channel in in event trigger mode
 */
static inline bool daqChannelGetTriggerSource( register DAQ_CANNEL_T* pThis )
{
   return (daqChannelGetCtrlRegPtr( pThis )->ExtTrig_nEvTrig == ON);
}

/*! ---------------------------------------------------------------------------
 * @brief Enables the post mortem mode (PM) of the given channel.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline bool daqChannelEnablePostMortem( register DAQ_CANNEL_T* pThis )
{
   LM32_ASSERT( daqChannelGetCtrlRegPtr( pThis )->Ena_HiRes == OFF );
   daqChannelGetCtrlRegPtr( pThis )->Ena_PM = ON;
}

/*! ---------------------------------------------------------------------------
 * @brief Queries whether the post mortem mode (PM) is enabled.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @retval true PM is enabled
 * @retval false PM is disabled
 */
static inline bool daqChannelIsPostMortemActive( register DAQ_CANNEL_T* pThis )
{
   return (daqChannelGetCtrlRegPtr( pThis )->Ena_PM == ON);
}

/*! ---------------------------------------------------------------------------
 * @brief Disables the post mortem mode (PM) of the given channel.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline bool daqChannelDisablePostMortem( register DAQ_CANNEL_T* pThis )
{
   daqChannelGetCtrlRegPtr( pThis )->Ena_PM = OFF;
}

/*! ---------------------------------------------------------------------------
 * @brief Enables the high resolution sampling
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline void daqChannelEnableHighResolution( register DAQ_CANNEL_T* pThis )
{
   LM32_ASSERT( daqChannelGetCtrlRegPtr( pThis )->Ena_PM == OFF );
   daqChannelGetCtrlRegPtr( pThis )->Ena_HiRes = ON;
}

/*! ---------------------------------------------------------------------------
 * @brief Disables the high resolution sampling
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline void daqChannelDisableHighResolution( register DAQ_CANNEL_T* pThis )
{
   daqChannelGetCtrlRegPtr( pThis )->Ena_HiRes = OFF;
}

/*! ---------------------------------------------------------------------------
 * @brief Queries whether the high resolution mod is enabled
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @retval true high resolution mode is enabled
 * @retval false high resolution mode is disabled
 */
static inline bool daqChannelIsHighResolutionEnabled( register DAQ_CANNEL_T* pThis )
{
   return (daqChannelGetCtrlRegPtr( pThis )->Ena_HiRes == ON);
}

/*! ---------------------------------------------------------------------------
 * @brief Set the trigger source for high resolution mode to external
 *        trigger source.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline void daqChannelEnableExternTriggerHighRes( register DAQ_CANNEL_T* pThis )
{
   daqChannelGetCtrlRegPtr( pThis )->ExtTrig_nEvTrig_HiRes = ON;
}

/*! ---------------------------------------------------------------------------
 * @brief Enables the event trigger mode in high resolution mode.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline void daqChannelEnableEventTriggerHighRes( register DAQ_CANNEL_T* pThis )
{
   daqChannelGetCtrlRegPtr( pThis )->ExtTrig_nEvTrig_HiRes = OFF;
}

/*! ---------------------------------------------------------------------------
 * @brief Returns the trigger source of high resolution of the given
 *        channel object
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @retval true Channel is in external trigger mode
 * @retval false Channel in in event trigger mode
 */
static inline bool daqChannelGetTriggerSourceHighRes( register DAQ_CANNEL_T* pThis )
{
   return (daqChannelGetCtrlRegPtr( pThis )->ExtTrig_nEvTrig_HiRes == ON);
}

/*! --------------------------------------------------------------------------
 * @brief Gets the least significant word of the bus tag event
 *        trigger condition.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @return least significant wort of bus tag event condition
 */
static inline uint16_t daqChannelGetTriggerConditionLW( register DAQ_CANNEL_T* pThis )
{
   LM32_ASSERT( pThis != NULL );
   __DAQ_VERIFY_REG_ACCESS(TRIG_LW);
   return __DAQ_GET_REG( TRIG_LW );
}

/*! --------------------------------------------------------------------------
 * @brief sets the least significant word of the bus tag event
 *        trigger condition.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @param value least significant wort of bus tag event condition
 */
static inline void daqChannelSetTriggerConditionLW( register DAQ_CANNEL_T* pThis,
                                                    uint16_t value )
{
   LM32_ASSERT( pThis != NULL );
   __DAQ_VERIFY_REG_ACCESS(TRIG_LW);
   __DAQ_GET_REG( TRIG_LW ) = value;
}

/*! --------------------------------------------------------------------------
 * @brief Gets the most significant word of the bus tag event
 *        trigger condition.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @return most significant wort of bus tag event condition
 */
static inline uint16_t daqChannelGetTriggerConditionHW( register DAQ_CANNEL_T* pThis )
{
   LM32_ASSERT( pThis != NULL );
   __DAQ_VERIFY_REG_ACCESS( TRIG_HW );
   return __DAQ_GET_REG( TRIG_HW );
}

/*! --------------------------------------------------------------------------
 * @brief sets the most significant word of the bus tag event
 *        trigger condition.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @param value most significant wort of bus tag event condition
 */
static inline void daqChannelSetTriggerConditionHW( register DAQ_CANNEL_T* pThis,
                                                    uint16_t value )
{
   LM32_ASSERT( pThis != NULL );
   __DAQ_VERIFY_REG_ACCESS( TRIG_HW );
   __DAQ_GET_REG( TRIG_HW ) = value;
}

/*! ---------------------------------------------------------------------------
 * @brief Gets the trigger delay of this channel.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @return trigger delay
 */
static inline uint16_t daqChannelGetTriggerDelay( register DAQ_CANNEL_T* pThis )
{
   LM32_ASSERT( pThis != NULL );
   __DAQ_VERIFY_REG_ACCESS( TRIG_DLY );
   return __DAQ_GET_REG( TRIG_DLY );
}

/*! ---------------------------------------------------------------------------
 * @brief Set the trigger delay of this channel.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 * @param value trigger delay
 */
static inline void daqChannelSetTriggerDelay( register DAQ_CANNEL_T* pThis, uint16_t value )
{
   LM32_ASSERT( pThis != NULL );
   __DAQ_VERIFY_REG_ACCESS( TRIG_DLY );
   __DAQ_GET_REG( TRIG_DLY ) = value;
}

/*! --------------------------------------------------------------------------
 * @brief Get the pointer to the data of the post mortem respectively to
 *        the high resolution FiFo.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline uint16_t volatile * daqChannelGetPmDatPtr( register DAQ_CANNEL_T* pThis )
{ //TODO ?
   LM32_ASSERT( pThis != NULL );
   __DAQ_VERIFY_REG_ACCESS( PM_DAT );
   return &__DAQ_GET_REG( PM_DAT );
}

ALWAYS_INLINE
static inline uint16_t daqChannelPopPmFifo( register DAQ_CANNEL_T* pThis )
{
   return *daqChannelGetPmDatPtr( pThis );
}

/*! --------------------------------------------------------------------------
 * @brief Get the pointer to the data of the DAQ FiFo.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the channel object
 */
static inline uint16_t volatile * daqChannelGetDaqDatPtr( register DAQ_CANNEL_T* pThis )
{  //TODO ?
   LM32_ASSERT( pThis != NULL );
   __DAQ_VERIFY_REG_ACCESS( DAQ_DAT );
   return &__DAQ_GET_REG( DAQ_DAT );
}

ALWAYS_INLINE
static inline uint16_t daqChannelPopDaqFifo( register DAQ_CANNEL_T* pThis )
{
   return *daqChannelGetDaqDatPtr( pThis );
}

/*! ---------------------------------------------------------------------------
 * @brief Get version of the DAQ VHDL Macros [0..126]
 * @param pThis Pointer to the channel object
 * @return version number
 */
static inline int daqChannelGetMacroVersion( register DAQ_CANNEL_T* pThis )
{
   LM32_ASSERT( pThis != NULL );
   __DAQ_VERIFY_REG_ACCESS( DAQ_FIFO_WORDS );
   return ((DAQ_DAQ_FIFO_WORDS_T*) &__DAQ_GET_REG( DAQ_FIFO_WORDS ))->version;
}

/*! ---------------------------------------------------------------------------
 * @brief Get remaining number of words in DaquDat Fifo.
 * @param pThis Pointer to the channel object
 * @return Remaining number of words during read out.
 */
static inline unsigned int daqChannelGetDaqFifoWords( register DAQ_CANNEL_T* pThis )
{
#ifdef CONFIG_DAQ_PEDANTIC_CHECK
   LM32_ASSERT( pThis != NULL );
   __DAQ_VERIFY_REG_ACCESS( DAQ_FIFO_WORDS );
#endif
   return ((DAQ_DAQ_FIFO_WORDS_T*) &__DAQ_GET_REG( DAQ_FIFO_WORDS ))->fifoWords;
}

/*! ---------------------------------------------------------------------------
 * @brief Gets the maximum number of all used channels form the DAQ device
 *        where this channel belongs.
 * @param pThis Pointer to the channel object
 * @return Number of used channels
 */
static inline unsigned int daqChannelGetMaxCannels( register DAQ_CANNEL_T* pThis )
{
   LM32_ASSERT( pThis != NULL );
   __DAQ_VERIFY_REG_ACCESS( PM_FIFO_WORDS );
   return ((DAQ_PM_FIFO_WORDS_T*) &__DAQ_GET_REG( PM_FIFO_WORDS ))->maxChannels;
}

/*! ---------------------------------------------------------------------------
 * @brief Get remaining number of words in PmDat Fifo.
 * @param pThis Pointer to the channel object
 * @return Remaining number of words during read out.
 */
static inline unsigned int daqChannelGetPmFifoWords( register DAQ_CANNEL_T* pThis )
{
#ifdef CONFIG_DAQ_PEDANTIC_CHECK
   LM32_ASSERT( pThis != NULL );
   __DAQ_VERIFY_REG_ACCESS( PM_FIFO_WORDS );
#endif
   return ((DAQ_PM_FIFO_WORDS_T*) &__DAQ_GET_REG( PM_FIFO_WORDS ))->fifoWords;
}

/*! ---------------------------------------------------------------------------
 * @brief Prints the actual channel information to the console.
 * @note This function becomes implemented only if the compiler switch
 *       CONFIG_DAQ_DEBUG has been defined!
 * @param pThis Pointer to the channel object
 */
#if defined( CONFIG_DAQ_DEBUG ) || defined(__DOXYGEN__)
   void daqChannelPrintInfo( register DAQ_CANNEL_T* pThis );
#else
   #define daqChannelPrintInfo( pThis ) (void)0
#endif

/*======================== DAQ- Device Functions ============================*/
/*! ---------------------------------------------------------------------------
 * @brief Get the slot number of a DAQ device respectively the
 *        DAQ-SCU-bis slave.
 * @see daqChannelGetRegPtr
 * @param pThis Pointer to the DAQ-device object
 * @return Slot number.
 */
static inline int daqDeviceGetSlot( register DAQ_DEVICE_T* pThis )
{
   LM32_ASSERT( pThis != NULL );
   LM32_ASSERT( pThis->pReg != NULL );
   /*
    * All existing channels of a DAQ have the same slot number
    * therefore its enough to choose the channel 0.
    */
   return daqChannelGetSlot( &pThis->aChannel[0] );
}

/*! ---------------------------------------------------------------------------
 * @brief Get version of the DAQ VHDL Macros [0..126]
 * @param pThis Pointer to the DAQ-device object
 * @return version number
 */
static inline int daqDeviceGetMacroVersion( register DAQ_DEVICE_T* pThis )
{
   LM32_ASSERT( pThis != NULL );
   LM32_ASSERT( pThis->pReg != NULL );
   /*
    * All existing channels of a DAQ have the same version number
    * therefore its enough to choose the channel 0.
    */
   return daqChannelGetMacroVersion( &pThis->aChannel[0] );
}

/*! ---------------------------------------------------------------------------
 * @brief Gets the maximum number of existing channels of this device.
 * @param pThis Pointer to the DAQ-device objects
 * @return Number of existing channels of this device.
 */
static inline unsigned int daqDeviceGetMaxChannels( register DAQ_DEVICE_T* pThis )
{
   LM32_ASSERT( pThis != NULL );
   LM32_ASSERT( pThis->pReg != NULL );
   /*
    * All existing channels of a DAQ have the same information
    * of the maximum number of used channels,
    * therefore its enough to choose the channel 0.
    */
   return daqChannelGetMaxCannels( &pThis->aChannel[0] );
}

/*! ---------------------------------------------------------------------------
 * @brief Gets the number of used channels.
 *
 * They must be equal or smaller than the maximum of existing channels.
 *
 * @see daqDeviceGetMaxChannels
 * @see DAQ_CHANNEL_BF_PROPERTY_T ::notUsed
 * @param pThis Pointer to the DAQ-device objects
 * @return Number of used channels of this device.
 */
unsigned int daqDeviceGetUsedChannels( register DAQ_DEVICE_T* pThis );


/*! ---------------------------------------------------------------------------
 * @brief Gets the pointer to the device object remaining to the given number.
 * @param pThis Pointer to the DAQ-device objects
 * @param n Channel number in a range of 0 to max found channels minus one.
 * @see daqDeviceGetMaxChannels
 */
static inline DAQ_CANNEL_T* daqDeviceGetChannelObject( register DAQ_DEVICE_T* pThis,
                                                       const unsigned int n )
{
   LM32_ASSERT( n < ARRAY_SIZE(pThis->aChannel) );
   LM32_ASSERT( n < pThis->maxChannels );
   return &pThis->aChannel[n];
}

/*! ---------------------------------------------------------------------------
 * @brief Becomes invoked from the interrupt routine.
 * @param pThis Pointer to the DAQ-device objects
 */
void daqDeviceOnInterrupt( register DAQ_DEVICE_T* pThis );

/*! ---------------------------------------------------------------------------
 * @brief Prints the actual DAQ-device information to the console.
 * @note This function becomes implemented only if the compiler switch
 *       CONFIG_DAQ_DEBUG has been defined!
 * @param pThis Pointer to the DAQ-device object
 */
#if defined( CONFIG_DAQ_DEBUG ) || defined(__DOXYGEN__)
   void daqDevicePrintInfo( register DAQ_DEVICE_T* pThis );
#else
   #define daqDevicePrintInfo( pThis ) (void)0
#endif

/*============================ DAQ Bus Functions ============================*/
/*! ------------------------------------------------------------------------
 * @brief Preinitialized the DAQ_BUS_T by zero and try to find all
 *        existing DAQs connected to SCU-bus.
 *
 * For each found DAQ the a element of DAQ_BUS_T::DAQ_DEVICE_T becomes initialized.
 *
 * @param pAllDAQ Pointer object of DAQ_BUS_T including a list of all DAQ.
 * @param pScuBusBase Base address of SCU bus.
 *                    Obtained by find_device_adr(GSI, SCU_BUS_MASTER);
 * @retval -1 Error occurred.
 * @retval  0 No DAQ found.
 * @retval >0 Number of connected DAQ in SCU-bus.
 */
int daqBusFindAndInitializeAll( register DAQ_BUS_T* pAllDAQ, const void* pScuBusBase );

/*! ---------------------------------------------------------------------------
 * @brief Returns the total number of DAQ input channels from all DAQ-slaves
 *        of the whole SCU-bus.
 * @param pAllDAQ Pointer object of DAQ_BUS_T including a list of all DAQ.
 * @note Function daqBusFindAndInitializeAll has to be invoked before!
 *
 */
int daqBusGetNumberOfAllFoundChannels( register DAQ_BUS_T* pAllDAQ );

/*! ---------------------------------------------------------------------------
 * @brief Gets the number of found DAQ devices.
 * @param pThis Pointer to the DAQ bus object.
 * @return Number of found DAQ - devices
 */
ALWAYS_INLINE
static inline unsigned int daqBusGetFoundDevices( const register DAQ_BUS_T* pThis )
{
   return pThis->foundDevices;
}

/*! ---------------------------------------------------------------------------
 * @brief Get the total number of used DAQ channels of this SCU bus.
 *
 * This number has to be smaller or equal than the number of
 * all found channels.
 * @see daqBusGetNumberOfAllFoundChannels
 * @see daqDeviceGetUsedChannels
 * @see DAQ_CHANNEL_BF_PROPERTY_T ::notUsed
 * @param pThis Pointer to the DAQ bus object.
 * @return Total number of all used channels from this bus.
 */
unsigned int daqBusGetUsedChannels( register DAQ_BUS_T* pThis );

/*! ---------------------------------------------------------------------------
 * @brief Gets the pointer to a device object by its device number.
 * @note Do not confuse the device number with the slot number!
 * @param pThis Pointer to the DAQ bus object.
 * @param n Device number in the range of 0 to number of found devices minus one.
 * @see daqBusGetFoundDevices
 * @return Pointer of type DAQ_DEVICE_T
 */
static inline DAQ_DEVICE_T* daqBusGetDeviceObject( register DAQ_BUS_T* pThis,
                                                   const unsigned int n )
{
   LM32_ASSERT( n < ARRAY_SIZE(pThis->aDaq) );
   LM32_ASSERT( n < pThis->foundDevices );
   return &pThis->aDaq[n];
}

/*! ---------------------------------------------------------------------------
 * @brief Gets the pointer of a DAQ device object by the slot number
 * @param pThis Pointer to the DAQ bus object.
 * @param slot Slot number range: [1..12]
 * @retval ==NULL No DAQ device in the given slot.
 * @retval !=NULL Pointer to the DAQ device connected in the given slot.
 */
DAQ_DEVICE_T* daqBusGetDeviceBySlotNumber( register DAQ_BUS_T* pThis,
                                           unsigned int slot );

/*! ---------------------------------------------------------------------------
 * @brief Gets the pointer to the channel object related to the absolute
 *        channel number.
 *
 * That means the absolute channel number independent of the DAQ device.\n
 * E.g.: Supposing all DAQ devices connected to the SCU-bus have four channels.\n
 *       Thus has the second channel of the second DAQ device the
 *       absolute number 5 from a maximum of 8 channels.
 * @note The counting starts at the left side of the SCU bus with zero.\n
 *       The first channel of the first device has the number 0.
 * @see daqBusGetNumberOfAllFoundChannels
 * @param pThis Pointer to the DAQ bus object.
 * @param n Absolute channel number beginning at the left side with zero.
 * @retval ==NULL Channel not present. (Maybe the given number is to high.)
 * @retval !=NULL Pointer to the channel object.
 */
DAQ_CANNEL_T* daqBusGetChannelObjectByAbsoluteNumber( register DAQ_BUS_T* pThis,
                                                      const unsigned int n );

/*! ---------------------------------------------------------------------------
 * @todo All!
 */
unsigned int daqBusDistributeMemory( register DAQ_BUS_T* pThis );

/*! ---------------------------------------------------------------------------
 * @brief Prints the information of all found DAQ-devices to the console.
 * @note This function becomes implemented only if the compiler switch
 *       CONFIG_DAQ_DEBUG has been defined!
 * @param pThis Pointer to the DAQ bus object.
 */
#if defined( CONFIG_DAQ_DEBUG ) || defined(__DOXYGEN__)
   void daqBusPrintInfo( register DAQ_BUS_T* pThis );
#else
   #define daqBusPrintInfo( pThis ) (void)0
#endif

#ifdef __cplusplus
}
#endif
#endif /* ifndef _DAQ_H */
/*================================== EOF ====================================*/
