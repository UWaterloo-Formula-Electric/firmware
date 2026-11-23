#ifndef CHARGER_REGS_H
#define CHARGER_REGS_H

#define BQ_I2C_ADDRESS              0x6B
#define BQ_I2C_HAL_ADDRESS          (BQ_I2C_ADDRESS << 1)
#define BQ_CHARGE_VOLTAGE_LIMIT_REG 0x00   // 16 bit
#define BQ_VFB_REG_MASK             0x001F // VFB_REG [4:0] 2mV/step from 1504mV
#define BQ_CHARGE_CURRENT_LIMIT_REG 0x02   // 16 bit
#define BQ_ICHG_REG_MASK            0x07FC // ICHG_REG [10:2] 50mA/step
#define BQ_TERM_CURRENT_LIMIT_REG   0x12   // 16 bit
#define BQ_ITERM_MASK               0x03FC // ITERM [9:2], 50 mA/step
#define BQ_PART_INFO_REG            0x3D // Part Info Register
#define BQ_PART_NUM_SHIFT           3    
#define BQ_PART_NUM_MASK            0x78
#define BQ_DEV_REV_SHIFT            0   
#define BQ_DEV_REV_MASK             0x07
#define BQ_PART_INFO_EXPECTED       0x12 
#define BQ_PART_NUMBER_EXPECTED     ((BQ_PART_INFO_EXPECTED & BQ_PART_NUM_MASK) >> BQ_PART_NUM_SHIFT)
#define BQ_CONTROL_REG              0x17
#define BQ_WATCHDOG_SHIFT           5
#define BQ_WATCHDOG_RESET_MASK      0x20
#define BQ_DIS_CE_SHIFT             4
#define BQ_DIS_CE_MASK              0x10
#define BQ_VRECHG_MASK              0xC0   // 0x17 [7:6] recharge threshold
#define BQ_CHARGE_STAT1_REG         0x21
#define BQ_CHARGE_STAT2_REG         0x22
#define BQ_FAULT_STAT_REG           0x24 
#define BQ_ADC_CONTROL_REG          0x2B
#define BQ_ADC_EN_MASK              0x80   // 0x2B [7]
#define BQ_ADC_RATE_MASK            0x40   // 0x2B [6]: 0 = continuous, 1 = oneshot (default)
#define BQ_VBAT_ADC_REG             0x33
#define BQ_TS_ADC_REG               0x37
#define BQ_IBAT_ADC_REG             0x2F
#endif // CHARGER_REGS_H