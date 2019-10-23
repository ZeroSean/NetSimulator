#ifndef DECADEVICEAPI_H
#define DECADEVICEAPI_H

#include <QObject>

typedef uint64_t        uint64 ;
typedef int64_t         int64 ;
typedef uint32_t        uint32 ;
typedef int32_t         int32 ;
typedef uint16_t        uint16 ;
typedef int16_t         int16 ;
typedef uint8_t        uint8 ;
typedef int8_t         int8 ;
typedef uint8_t        u8 ;
typedef uint16_t       u16 ;

#define DWT_SUCCESS (0)
#define DWT_ERROR   (-1)

#define DWT_TIME_UNITS          (1.0/499.2e6/128.0) //!< = 15.65e-12 s

#define DWT_DEVICE_ID   (0xDECA0130)        //!< DW1000 MP device ID

//! constants for selecting the bit rate for data TX (and RX)
//! These are defined for write (with just a shift) the TX_FCTRL register
#define DWT_BR_110K     0   //!< UWB bit rate 110 kbits/s
#define DWT_BR_850K     1   //!< UWB bit rate 850 kbits/s
#define DWT_BR_6M8      2   //!< UWB bit rate 6.8 Mbits/s

//! constants for specifying the (Nominal) mean Pulse Repetition Frequency
//! These are defined for direct write (with a shift if necessary) to CHAN_CTRL and TX_FCTRL regs
#define DWT_PRF_16M     1   //!< UWB PRF 16 MHz
#define DWT_PRF_64M     2   //!< UWB PRF 64 MHz

//! constants for specifying Preamble Acquisition Chunk (PAC) Size in symbols
#define DWT_PAC8        0   //!< PAC  8 (recommended for RX of preamble length  128 and below
#define DWT_PAC16       1   //!< PAC 16 (recommended for RX of preamble length  256
#define DWT_PAC32       2   //!< PAC 32 (recommended for RX of preamble length  512
#define DWT_PAC64       3   //!< PAC 64 (recommended for RX of preamble length 1024 and up

//! constants for specifying TX Preamble length in symbols
//! These are defined to allow them be directly written into byte 2 of the TX_FCTRL register
//! (i.e. a four bit value destined for bits 20..18 but shifted left by 2 for byte alignment)
#define DWT_PLEN_4096   0x0C    //! Standard preamble length 4096 symbols
#define DWT_PLEN_2048   0x28    //! Non-standard preamble length 2048 symbols
#define DWT_PLEN_1536   0x18    //! Non-standard preamble length 1536 symbols
#define DWT_PLEN_1024   0x08    //! Standard preamble length 1024 symbols
#define DWT_PLEN_512    0x34    //! Non-standard preamble length 512 symbols
#define DWT_PLEN_256    0x24    //! Non-standard preamble length 256 symbols
#define DWT_PLEN_128    0x14    //! Non-standard preamble length 128 symbols
#define DWT_PLEN_64     0x04    //! Standard preamble length 64 symbols

#define DWT_SFDTOC_DEF              0x1041  // default SFD timeout value

#define DWT_PHRMODE_STD             0x0     // standard PHR mode
#define DWT_PHRMODE_EXT             0x3     // DW proprietary extended frames PHR mode

// Defined constants for "mode" bitmask parameter passed into dwt_starttx() function.
#define DWT_START_TX_IMMEDIATE      0
#define DWT_START_TX_DELAYED        1
#define DWT_RESPONSE_EXPECTED       2

#define DWT_START_RX_IMMEDIATE  0
#define DWT_START_RX_DELAYED    1    // Set up delayed RX, if "late" error triggers, then the RX will be enabled immediately
#define DWT_IDLE_ON_DLY_ERR     2    // If delayed RX failed due to "late" error then if this
                                     // flag is set the RX will not be re-enabled immediately, and device will be in IDLE when function exits
#define DWT_NO_SYNC_PTRS        4    // Do not try to sync IC side and Host side buffer pointers when enabling RX. This is used to perform manual RX
                                     // re-enabling when receiving a frame in double buffer mode.

// Defined constants for "mode" bit field parameter passed to dwt_setleds() function.
#define DWT_LEDS_DISABLE     0x00
#define DWT_LEDS_ENABLE      0x01
#define DWT_LEDS_INIT_BLINK  0x02

//frame filtering configuration options
#define DWT_FF_NOTYPE_EN            0x000           // no frame types allowed (FF disabled)
#define DWT_FF_COORD_EN             0x002           // behave as coordinator (can receive frames with no dest address (PAN ID has to match))
#define DWT_FF_BEACON_EN            0x004           // beacon frames allowed
#define DWT_FF_DATA_EN              0x008           // data frames allowed
#define DWT_FF_ACK_EN               0x010           // ack frames allowed
#define DWT_FF_MAC_EN               0x020           // mac control frames allowed
#define DWT_FF_RSVD_EN              0x040           // reserved frame types allowed

//DW1000 interrupt events
#define DWT_INT_TFRS            0x00000080          // frame sent
#define DWT_INT_LDED            0x00000400          // micro-code has finished execution
#define DWT_INT_RFCG            0x00004000          // frame received with good CRC
#define DWT_INT_RPHE            0x00001000          // receiver PHY header error
#define DWT_INT_RFCE            0x00008000          // receiver CRC error
#define DWT_INT_RFSL            0x00010000          // receiver sync loss error
#define DWT_INT_RFTO            0x00020000          // frame wait timeout
#define DWT_INT_RXOVRR          0x00100000          // receiver overrun
#define DWT_INT_RXPTO           0x00200000          // preamble detect timeout
#define DWT_INT_SFDT            0x04000000          // SFD timeout
#define DWT_INT_ARFE            0x20000000          // frame rejected (due to frame filtering configuration)


//DW1000 SLEEP and WAKEUP configuration parameters
#define DWT_PRESRV_SLEEP 0x0100                      // PRES_SLEEP - on wakeup preserve sleep bit
#define DWT_LOADOPSET    0x0080                      // ONW_L64P - on wakeup load operating parameter set for 64 PSR
#define DWT_CONFIG       0x0040                      // ONW_LDC - on wakeup restore (load) the saved configurations (from AON array into HIF)
#define DWT_LOADEUI      0x0008                      // ONW_LEUI - on wakeup load EUI
#define DWT_RX_EN        0x0002                      // ONW_RX - on wakeup activate reception
#define DWT_TANDV        0x0001                      // ONW_RADC - on wakeup run ADC to sample temperature and voltage sensor values

#define DWT_XTAL_EN      0x10                       // keep XTAL running during sleep
#define DWT_WAKE_SLPCNT  0x8                        // wake up after sleep count
#define DWT_WAKE_CS      0x4                        // wake up on chip select
#define DWT_WAKE_WK      0x2                        // wake up on WAKEUP PIN
#define DWT_SLP_EN       0x1                        // enable sleep/deep sleep functionality

//DW1000 INIT configuration parameters
#define DWT_LOADUCODE     0x1
#define DWT_LOADNONE      0x0

//DW1000 OTP operating parameter set selection
#define DWT_OPSET_64LEN   0x0
#define DWT_OPSET_TIGHT   0x1
#define DWT_OPSET_DEFLT   0x2

// Call-back data RX frames flags
#define DWT_CB_DATA_RX_FLAG_RNG 0x1 // Ranging bit

// TX/RX call-back data
typedef struct
{
    uint32 status;      //initial value of register as ISR is entered
    uint16 datalength;  //length of frame
    uint8  fctrl[2];    //frame control bytes
    uint8  rx_flags;    //RX frame flags, see above
} dwt_cb_data_t;

typedef struct
{
    uint8 chan ;           //!< channel number {1, 2, 3, 4, 5, 7 }
    uint8 prf ;            //!< Pulse Repetition Frequency {DWT_PRF_16M or DWT_PRF_64M}
    uint8 txPreambLength ; //!< DWT_PLEN_64..DWT_PLEN_4096
    uint8 rxPAC ;          //!< Acquisition Chunk Size (Relates to RX preamble length)
    uint8 txCode ;         //!< TX preamble code
    uint8 rxCode ;         //!< RX preamble code
    uint8 nsSFD ;          //!< Boolean should we use non-standard SFD for better performance
    uint8 dataRate ;       //!< Data Rate {DWT_BR_110K, DWT_BR_850K or DWT_BR_6M8}
    uint8 phrMode ;        //!< PHR mode {0x0 - standard DWT_PHRMODE_STD, 0x3 - extended frames DWT_PHRMODE_EXT}
    uint16 sfdTO ;         //!< SFD timeout value (in symbols)
} dwt_config_t ;

typedef struct
{
    uint8   PGdly;
    //TX POWER
    //31:24     BOOST_0.125ms_PWR
    //23:16     BOOST_0.25ms_PWR-TX_SHR_PWR
    //15:8      BOOST_0.5ms_PWR-TX_PHR_PWR
    //7:0       DEFAULT_PWR-TX_DATA_PWR
    uint32  power;
} dwt_txconfig_t ;

#define DWT_PRF_64M_RFDLY   (514.462f)
#define DWT_PRF_16M_RFDLY   (513.9067f)


#define USR_SFD_ID              0x21            /* User-specified short/long TX/RX SFD sequences */
#define USR_SFD_LEN             (41)
#define DW_NS_SFD_LEN_110K      64              /* Decawave non-standard SFD length for 110 kbps */
#define DW_NS_SFD_LEN_850K      16              /* Decawave non-standard SFD length for 850 kbps */
#define DW_NS_SFD_LEN_6M8       8               /* Decawave non-standard SFD length for 6.8 Mbps */




#endif // DECADEVICEAPI_H
