#ifndef CHARGER_REGS_H
#define CHARGER_REGS_H

#define BQ_I2C_ADDRESS              0x6B
#define BQ_I2C_HAL_ADDRESS          (BQ_I2C_ADDRESS << 1)
#define BQ_REG_PART_INFO            0x3D // Part Info Register
#define BQ_PART_NUM_SHIFT           3    
#define BQ_PART_NUM_MASK            0x78
#define BQ_DEV_REV_SHIFT            0   
#define BQ_DEV_REV_MASK             0x07
#define BQ_PART_INFO_EXPECTED       0x12 
#define BQ_PART_NUMBER_EXPECTED     ((BQ_PART_INFO_EXPECTED & BQ_PART_NUM_MASK) << BQ_PART_NUM_SHIFT)

#endif // CHARGER_REGS_H