/*
 * (C) Copyright 2018 allwinnertech  <wangwei@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _SUNXI_CPU_NCAT_H
#define _SUNXI_CPU_NCAT_H

#define SUNXI_DE3_BASE			0x01000000
#if defined(CONFIG_MACH_SUN50IW11) || defined(CONFIG_MACH_SUN50IW12)
#define SUNXI_SS_BASE			0x03040000
#else
#define SUNXI_SS_BASE			0x01904000
#endif
#define SUNXI_EMCE_BASE			0x01905000

#define SUNXI_SRAMC_BASE		0x03000000
#if defined(CONFIG_MACH_SUN50IW11) || defined(CONFIG_MACH_SUN50IW12)
#define SUNXI_CCM_BASE			0x02001000
#else
#define SUNXI_CCM_BASE			0x03001000
#endif
#define SUNXI_DMA_BASE			0x03002000
/* SID address space starts at 0x03006000, but e-fuse is at offset 0x200 */
#define SUNXI_SID_BASE			0x03006000
#define SUNXI_SID_SRAM_BASE		0x03006200
#if defined(CONFIG_MACH_SUN50IW11)
#define SUNXI_TIMER_BASE		0x02000000
#elif defined(CONFIG_MACH_SUN50IW12)
#define SUNXI_TIMER_BASE		0x03060000
#else
#define SUNXI_TIMER_BASE		0x03009000
#endif
#if defined(CONFIG_MACH_SUN50IW11)
#define SUNXI_PIO_BASE			0x02000400
#elif defined(CONFIG_MACH_SUN50IW12)
#define SUNXI_PIO_BASE			0x02000000
#else
#define SUNXI_PIO_BASE			0x0300B000
#endif
#define SUNXI_PSI_BASE			0x0300C000

#define SUNXI_GIC400_BASE		0x03020000
#define SUNXI_IOMMU_BASE		0x030F0000

#define SUNXI_DRAM_COM_BASE		0x04002000
#define SUNXI_DRAM_CTL0_BASE		0x04003000
#define SUNXI_DRAM_PHY0_BASE		0x04005000
#define SUNXI_NFC_BASE			0x04011000
#define SUNXI_MMC0_BASE			0x04020000
#define SUNXI_MMC1_BASE			0x04021000
#define SUNXI_MMC2_BASE			0x04022000

#if defined(CONFIG_MACH_SUN50IW11) || defined(CONFIG_MACH_SUN50IW12)
#define SUNXI_UART0_BASE		0x02500000
#define SUNXI_UART1_BASE		0x02500400
#define SUNXI_UART2_BASE		0x02500800
#define SUNXI_UART3_BASE		0x02500C00
#else
#define SUNXI_UART0_BASE		0x05000000
#define SUNXI_UART1_BASE		0x05000400
#define SUNXI_UART2_BASE		0x05000800
#define SUNXI_UART3_BASE		0x05000C00
#endif
#if defined(CONFIG_MACH_SUN50IW11) || defined(CONFIG_MACH_SUN50IW12)
#define SUNXI_TWI0_BASE			0x02502000
#define SUNXI_TWI1_BASE			0x02502400
#else
#define SUNXI_TWI0_BASE			0x05002000
#define SUNXI_TWI1_BASE			0x05002400
#define SUNXI_TWI2_BASE			0x05002800
#define SUNXI_TWI3_BASE			0x05002C00
#endif
#if defined(CONFIG_MACH_SUN50IW11) || defined(CONFIG_MACH_SUN50IW12)
#define SUNXI_SPI0_BASE			0x04025000
#define SUNXI_SPI1_BASE			0x04026000
#else
#define SUNXI_SPI0_BASE			0x05010000
#define SUNXI_SPI1_BASE			0x05011000
#endif
#if defined(CONFIG_MACH_SUN50IW11) || defined(CONFIG_MACH_SUN50IW12)
#define SUNXI_GMAC_BASE			0x04500000
#else
#define SUNXI_GMAC_BASE			0x05020000
#endif
#if defined(CONFIG_MACH_SUN50IW11) || defined(CONFIG_MACH_SUN50IW12)
#define SUNXI_USB0_BASE			0x04100000
#else
#define SUNXI_USB0_BASE			0x05100000
#endif
#if defined(CONFIG_MACH_SUN50IW11)
#define SUNXI_USB1_BASE			0x04200000
#endif
/*USB DRD AS ehci0*/
#if defined(CONFIG_MACH_SUN50IW6)
#define SUNXI_EHCI0_BASE		0x05100000
#define SUNXI_EHCI1_BASE		0x05300000
#define SUNXI_EHCI2_BASE		0x05310000
#elif defined(CONFIG_MACH_SUN50IW5) || defined(CONFIG_MACH_SUN50IW9)
#define SUNXI_EHCI0_BASE		0x05101000
#define SUNXI_EHCI1_BASE		0x05200000
#define SUNXI_EHCI2_BASE		0x05310000
#define SUNXI_EHCI3_BASE		0x05311000
#elif defined(CONFIG_MACH_SUN50IW11)
#define SUNXI_EHCI0_BASE		0x04101000
#define SUNXI_EHCI1_BASE		0x04201000
#elif defined(CONFIG_MACH_SUN50IW12)
#define SUNXI_EHCI0_BASE		0x04101000
#define SUNXI_EHCI1_BASE		0x04200000
#elif defined(CONFIG_MACH_SUN8IW11)
#define SUNXI_EHCI0_BASE		0x01c14000
#define SUNXI_EHCI1_BASE		0x01c19000
#else
#define SUNXI_EHCI0_BASE		0x05101000
#define SUNXI_EHCI1_BASE		0x05200000
#endif

#define SUNXI_PCIE_BASE			0x05400000

#define SUNXI_HDMI_BASE			0x06000000
#define SUNXI_TCON_TOP_BASE		0x06510000
#define SUNXI_TCON_LCD0_BASE		0x06511000
#define SUNXI_TCON_TV0_BASE		0x06515000

#if defined(CONFIG_MACH_SUN50IW11) || defined(CONFIG_MACH_SUN50IW12)
#define SUNXI_RTC_BASE			0x07090000
#else
#define SUNXI_RTC_BASE			0x07000000
#endif
#define SUNXI_R_CPUCFG_BASE		0x07000400
#define SUNXI_PRCM_BASE			0x07010000
#define SUNXI_R_IR_RX_BASE		0x07040000
#define SUNXI_R_PIO_BASE		0x07022000
#define SUNXI_R_UART_BASE		0x07080000
#define SUNXI_R_TWI_BASE		0x07081400
#define SUNXI_R_RSB_BASE		0x07083000
#define SUNXI_RSB_BASE			0x07083000
#define SUNXI_RTWI_BRG_REG		(SUNXI_PRCM_BASE+0x019c)

#if defined (CONFIG_MACH_SUN50IW12)
#define SUNXI_WDT_BASE 			0x02051000
#endif

#if defined(CONFIG_MACH_SUN50IW10)
#define SUNXI_CPUXCFG_BASE		0x08100000
#else
#define SUNXI_CPUXCFG_BASE              0x09010000
#endif

#define SUNXI_USBOTG_BASE		SUNXI_USB0_BASE
#define SUNXI_RTC_DATA_BASE     (SUNXI_RTC_BASE+0x100)
#define RTC_XO_CTRL_REG         (SUNXI_RTC_BASE+0x160)
/*physical key*/
#define SUNXI_GPADC_BASE                  (0x05070000L)
#define SUNXI_LRADC_BASE                  (0x05070800L)
#define SUNXI_KEYADC_BASE                 SUNXI_LRADC_BASE

#ifndef __ASSEMBLY__
void sunxi_board_init(void);
void sunxi_reset(void);
int sunxi_get_sid(unsigned int *sid);
#endif

#endif /* _SUNXI_CPU_SCAT_H */
