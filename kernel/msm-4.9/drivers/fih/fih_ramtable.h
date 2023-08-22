#ifndef __FIH_RAMTABLE_H
#define __FIH_RAMTABLE_H

//#define FIH_RAM_BASE					0xC3200000
#define FIH_RAM_BASE					0x84A00000
#define FIH_RAM_SIZE					0x00400000
#define FIH_RAM_SIZE_MB					4

//jason add for run in memory test
#define FIH_RAM_TEST					0xC5A00000
#define FIH_RAM_TEST_SIZE_MB		934

#define FIH_RAM_SIZE_CH0_MB			1536
#define FIH_RAM_TEST_CH0_START		(0x80000000-(FIH_RAM_SIZE_CH0_MB*1024*1024))
 /* -------------------------------------------------------- */
/* modem rf_nv */
#define FIH_MODEM_RF_NV_BASE			FIH_RAM_BASE
#define FIH_MODEM_RF_NV_SIZE			0x00200000
#define NV_RF_ADDR						FIH_MODEM_RF_NV_BASE
#define NV_RF_SIZE						FIH_MODEM_RF_NV_SIZE

/* modem cust_nv */
#define FIH_MODEM_CUST_NV_BASE			(FIH_MODEM_RF_NV_BASE + FIH_MODEM_RF_NV_SIZE)
#define FIH_MODEM_CUST_NV_SIZE			0x00000000
#define NV_CUST_ADDR					FIH_MODEM_CUST_NV_BASE
#define NV_CUST_SIZE					FIH_MODEM_CUST_NV_SIZE

/* modem default_nv */
#define FIH_MODEM_DEFAULT_NV_BASE			(FIH_MODEM_CUST_NV_BASE + FIH_MODEM_CUST_NV_SIZE)
#define FIH_MODEM_DEFAULT_NV_SIZE			0x00200000
#define NV_DEFAULT_ADDR					FIH_MODEM_DEFAULT_NV_BASE
#define NV_DEFAULT_SIZE					FIH_MODEM_DEFAULT_NV_SIZE

/* modem log */
#define FIH_MODEM_LOG_BASE				(FIH_MODEM_DEFAULT_NV_BASE + FIH_MODEM_DEFAULT_NV_SIZE)
#define FIH_MODEM_LOG_SIZE				0x00100000

/* -------------------------------------------------------- 5MB */
/* last_alog_main */
#define FIH_LAST_ALOG_MAIN_BASE			(FIH_MODEM_LOG_BASE + FIH_MODEM_LOG_SIZE)
#define FIH_LAST_ALOG_MAIN_SIZE			0x00040000

/* last_alog_events */
#define FIH_LAST_ALOG_EVENTS_BASE		(FIH_LAST_ALOG_MAIN_BASE + FIH_LAST_ALOG_MAIN_SIZE)
#define FIH_LAST_ALOG_EVENTS_SIZE		0x00040000

/* last_alog_radio */
#define	FIH_LAST_ALOG_RADIO_BASE		(FIH_LAST_ALOG_EVENTS_BASE + FIH_LAST_ALOG_EVENTS_SIZE)
#define FIH_LAST_ALOG_RADIO_SIZE		0x00040000

/* last_alog_system */
#define FIH_LAST_ALOG_SYSTEM_BASE		(FIH_LAST_ALOG_RADIO_BASE + FIH_LAST_ALOG_RADIO_SIZE)
#define FIH_LAST_ALOG_SYSTEM_SIZE		0x00040000

/* last_kmsg */
#define FIH_LAST_KMSG_BASE				(FIH_LAST_ALOG_SYSTEM_BASE + FIH_LAST_ALOG_SYSTEM_SIZE)
#define FIH_LAST_KMSG_SIZE				0x00040000

/* last_blog */
#define FIH_LAST_BLOG_BASE				(FIH_LAST_KMSG_BASE + FIH_LAST_KMSG_SIZE)
#define FIH_LAST_BLOG_SIZE				0x00020000
#define FIH_DEBUG_LAST_BLOG_ADDR		FIH_LAST_BLOG_BASE

/* blog */
#define FIH_BLOG_BASE					(FIH_LAST_BLOG_BASE + FIH_LAST_BLOG_SIZE)
#define FIH_BLOG_SIZE					0x00020000
#define FIH_DEBUG_BLOG_ADDR				FIH_BLOG_BASE
#define FIH_DEBUG_BLOG_SIZE				FIH_BLOG_SIZE
#define FIH_DEBUG_BLOG_LIMT				(FIH_BLOG_BASE + FIH_BLOG_SIZE)
 /* -------------------------------------------------------- 6.5MB */
/* hwid */
#define FIH_HWID_HWCFG_BASE				(FIH_BLOG_BASE + FIH_BLOG_SIZE)
#define FIH_HWID_HWCFG_SIZE				0x00000040
#define FIH_HWID_ADDR					FIH_HWID_HWCFG_BASE
#define FIH_HWID_SIZE					FIH_HWID_HWCFG_SIZE

/* secboot:devinfo */
#define FIH_SECBOOT_DEVINFO_BASE		(FIH_HWID_HWCFG_BASE + FIH_HWID_HWCFG_SIZE)
#define FIH_SECBOOT_DEVINFO_SIZE		0x00000040

/* secboot:unlock */
#define FIH_SECBOOT_UNLOCK_BASE			(FIH_SECBOOT_DEVINFO_BASE + FIH_SECBOOT_DEVINFO_SIZE)
#define FIH_SECBOOT_UNLOCK_SIZE			0x00000100

/* sutinfo */
#define FIH_SUTINFO_BASE				(FIH_SECBOOT_UNLOCK_BASE + FIH_SECBOOT_UNLOCK_SIZE)
#define FIH_SUTINFO_SIZE				0x00000080
#define FIH_SUT_ADDR					FIH_SUTINFO_BASE
#define FIH_SUT_SIZE					FIH_SUTINFO_SIZE

/* no use 1 */
#define FIH_NO_USE_1_BASE				(FIH_SUTINFO_BASE + FIH_SUTINFO_SIZE)
#define FIH_NO_USE_1_SIZE				0x00000010

/* bset */
#define FIH_BSET_BASE					(FIH_NO_USE_1_BASE + FIH_NO_USE_1_SIZE)
#define FIH_BSET_SIZE					0x00000010
#define FIH_BSET_MEM_ADDR				FIH_BSET_BASE
#define FIH_BSET_MEM_SIZE				FIH_BSET_SIZE

/* bat-id adc */
#define FIH_BAT_ID_ADC_BASE				(FIH_BSET_BASE + FIH_BSET_SIZE)
#define FIH_BAT_ID_ADC_SIZE				0x00000010

/* no use 2 */
#define FIH_NO_USE_2_BASE				(FIH_BAT_ID_ADC_BASE + FIH_BAT_ID_ADC_SIZE)
#define FIH_NO_USE_2_SIZE				0x00000010

/* apr */
#define FIH_APR_BASE					(FIH_NO_USE_2_BASE + FIH_NO_USE_2_SIZE)
#define FIH_APR_SIZE					0x00000020
#define FIH_APR_MEM_ADDR				FIH_APR_BASE
#define FIH_APR_MEM_SIZE				FIH_APR_SIZE

/* no use 3 */
#define FIH_NO_USE_3_BASE				(FIH_APR_BASE + FIH_APR_SIZE)
#define FIH_NO_USE_3_SIZE				0x00000180

/* mem */
#define FIH_MEM_BASE					(FIH_NO_USE_3_BASE + FIH_NO_USE_3_SIZE)
#define FIH_MEM_SIZE					0x00000020
#define FIH_MEM_MEM_ADDR				FIH_MEM_BASE
#define FIH_MEM_MEM_SIZE				FIH_MEM_SIZE

/* no use 4 */
#define FIH_NO_USE_4_BASE				(FIH_MEM_BASE + FIH_MEM_SIZE)
#define FIH_NO_USE_4_SIZE				0x00000C00

/* e2p */
#define FIH_E2P_BASE					(FIH_NO_USE_4_BASE + FIH_NO_USE_4_SIZE)
#define FIH_E2P_SIZE					0x00001000
#define FIH_E2P_ST_ADDR					FIH_E2P_BASE
#define FIH_E2P_ST_SIZE					FIH_E2P_SIZE

/* cda */
#define FIH_CDA_BASE					(FIH_E2P_BASE + FIH_E2P_SIZE)
#define FIH_CDA_SIZE					0x00001000
#define FIH_CDA_ST_ADDR					FIH_CDA_BASE
#define FIH_CDA_ST_SIZE					FIH_CDA_SIZE

/* note */
#define FIH_NOTE_BASE					(FIH_CDA_BASE + FIH_CDA_SIZE)
#define FIH_NOTE_SIZE					0x00001000
#define FIH_NOTE_MEM_ADDR					FIH_NOTE_BASE
#define FIH_NOTE_MEM_SIZE					FIH_NOTE_SIZE

/* hwcfg */
#define FIH_HWCFG_BASE					(FIH_NOTE_BASE + FIH_NOTE_SIZE)
#define FIH_HWCFG_SIZE					0x00001000
#define FIH_HWCFG_MEM_ADDR					FIH_HWCFG_BASE
#define FIH_HWCFG_MEM_SIZE					FIH_HWCFG_SIZE

/* fac */
#define FIH_FAC_BASE					(FIH_HWCFG_BASE + FIH_HWCFG_SIZE)
#define FIH_FAC_SIZE					0x00001000
#define FIH_FAC_MEM_ADDR					FIH_FAC_BASE
#define FIH_FAC_MEM_SIZE					FIH_FAC_SIZE

/* no use 5 */
#define FIH_NO_USE_5_BASE				(FIH_FAC_BASE + FIH_FAC_SIZE)
#define FIH_NO_USE_5_SIZE				0x00002000

/* fver *//*fihtdc,derek,enlarge fver to 256KB*/
#define FIH_FVER_BASE					(FIH_NO_USE_5_BASE + FIH_NO_USE_5_SIZE)
#define FIH_FVER_SIZE					0x00040000
#define FIH_FVER_ADDR					FIH_FVER_BASE

/* sensordata */
#define FIH_SENSORDATA_BASE				(FIH_FVER_BASE + FIH_FVER_SIZE)
#define FIH_SENSORDATA_SIZE				0x0000C000
#define FIH_SENSOR_MEM_ADDR				FIH_SENSORDATA_BASE
#define FIH_SENSOR_MEM_SIZE				FIH_SENSORDATA_SIZE

/* SW4-HL-Display-ReadOtpColorTemperatureNormalInLkAndWriteBackInKernel*{_20160307 */
/* LCM data */
#define FIH_LCMDATA_BASE				(FIH_SENSORDATA_BASE + FIH_SENSORDATA_SIZE)
#define FIH_LCMDATA_SIZE				0x00004000
#define FIH_LCM_MEM_ADDR				FIH_LCMDATA_BASE
#define FIH_LCM_MEM_SIZE				FIH_LCMDATA_SIZE

/* no use 6 */
#define FIH_NO_USE_6_BASE				(FIH_LCMDATA_BASE + FIH_LCMDATA_SIZE)
#define FIH_NO_USE_6_SIZE				0x00028000
 /* SW4-HL-Display-ReadOtpColorTemperatureNormalInLkAndWriteBackInKernel*}_20160307 */

 /* -------------------------------------------------------- 9MB */

/**************************************************************
 * START       | SIZE        | TARGET
 * -------------------------------------------------------- 0MB
 * 0xC320_0000 | 0x0020_0000 | modem rf_nv (2MB)
 * 0xC340_0000 | 0x0020_0000 | modem cust_nv (2MB)
 * 0xC360_0000 | 0x0020_0000 | modem default_nv (2MB)
 * 0xC380_0000 | 0x0010_0000 | modem log (1MB)
  * -------------------------------------------------------- 7MB
 * 0xC390_0000 | 0x0004_0000 | last_alog_main (256KB)
 * 0xC394_0000 | 0x0004_0000 | last_alog_events (256KB)
 * 0xC398_0000 | 0x0004_0000 | last_alog_radio (256KB)
 * 0xC39C_0000 | 0x0004_0000 | last_alog_system (256KB)
 * 0xC3A0_0000 | 0x0004_0000 | last_kmsg (256KB)
 * 0xC3A4_0000 | 0x0002_0000 | last_blog (128KB)
 * 0xC3A6_0000 | 0x0002_0000 | blog (128KB)
 * -------------------------------------------------------- 8.5MB
 * 0xC3A8_0000 | 0x0000_0040 | hwid:hwcfg (64B)
 * 0xC3A8_0040 | 0x0000_0040 | secboot:devinfo (64B)
 * 0xC3A8_0080 | 0x0000_0100 | secboot:unlock (256B)
 * 0xC3A8_0180 | 0x0000_0080 | sutinfo (128B)
 * 0xC3A8_0200 | 0x0000_0010 | no use (16B)
 * 0xC3A8_0210 | 0x0000_0010 | bset (16B)
 * 0xC3A8_0220 | 0x0000_0010 | bat-id adc (16B)
 * 0xC3A8_0230 | 0x0000_0010 | no use (16B)
 * 0xC3A8_0240 | 0x0000_0020 | apr (32B)
 * 0xC3A8_0260 | 0x0000_0180 | no use (384B)
 * 0xC3A8_03E0 | 0x0000_0020 | mem (32B)
 * 0xC3A8_0400 | 0x0000_0C00 | no use (3KB)
 * 0xC3A8_1000 | 0x0000_1000 | e2p (4KB)
 * 0xC3A8_2000 | 0x0000_1000 | cda (4KB)
 * 0xC3A8_3000 | 0x0000_1000 | note (4KB)
 * 0xC3A8_4000 | 0x0000_1000 | hwcfg (4KB)
 * 0xC3A8_5000 | 0x0000_1000 | fac (4KB)
 * 0xC3A8_6000 | 0x0000_2000 | no use 5 (8KB)
 * 0xC3A8_8000 | 0x0004_0000 | fver (256KB)
 * 0xC3AC_8000 | 0x0000_C000 | sensordata (48KB)
 * 0xC3AD_4000 | 0x0000_4000 | lcmdata (16KB)
 * 0xC3AD_8000 | 0x0002_8000 | no use 6 (160KB)
 * -------------------------------------------------------- 9MB
 * 0xC3B0_0000 | 0x0090_0000 | HLOS (9MB)

 */
#endif /* __FIH_RAMTABLE_H */




























