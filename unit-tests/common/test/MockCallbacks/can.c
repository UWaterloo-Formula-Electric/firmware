#include "can.h"
#include "unity.h"
#if BOARD_TYPE_F0
CAN_HandleTypeDef hcan;
#elif BOARD_TYPE_F7
CAN_HandleTypeDef hcan3;
#endif

void configCANFilters(CAN_HandleTypeDef* canHandle)
{
}

HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan, CAN_FilterTypeDef *sFilterConfig) {
#if 0
	uint32_t filternbrbitpos;
	CAN_TypeDef *can_ip = hcan->Instance;
	HAL_CAN_StateTypeDef state = hcan->State;

	if ((state == HAL_CAN_STATE_READY) ||
		(state == HAL_CAN_STATE_LISTENING))
	{
		/* Check the parameters */
		TEST_ASSERT_TRUE(IS_CAN_FILTER_ID_HALFWORD(sFilterConfig->FilterIdHigh));
		TEST_ASSERT_TRUE(IS_CAN_FILTER_ID_HALFWORD(sFilterConfig->FilterIdLow));
		TEST_ASSERT_TRUE(IS_CAN_FILTER_ID_HALFWORD(sFilterConfig->FilterMaskIdHigh));
		TEST_ASSERT_TRUE(IS_CAN_FILTER_ID_HALFWORD(sFilterConfig->FilterMaskIdLow));
		TEST_ASSERT_TRUE(IS_CAN_FILTER_MODE(sFilterConfig->FilterMode));
		TEST_ASSERT_TRUE(IS_CAN_FILTER_SCALE(sFilterConfig->FilterScale));
		TEST_ASSERT_TRUE(IS_CAN_FILTER_FIFO(sFilterConfig->FilterFIFOAssignment));
		TEST_ASSERT_TRUE(IS_CAN_FILTER_ACTIVATION(sFilterConfig->FilterActivation));

		/* CAN is single instance with 14 dedicated filters banks */

		/* Check the parameters */
		TEST_ASSERT_TRUE(IS_CAN_FILTER_BANK_SINGLE(sFilterConfig->FilterBank));

		/* Initialisation mode for the filter */
		SET_BIT(can_ip->FMR, CAN_FMR_FINIT);

		/* Convert filter number into bit position */
		filternbrbitpos = (uint32_t)1 << (sFilterConfig->FilterBank & 0x1FU);

		/* Filter Deactivation */
		CLEAR_BIT(can_ip->FA1R, filternbrbitpos);

		/* Filter Scale */
		if (sFilterConfig->FilterScale == CAN_FILTERSCALE_16BIT)
		{
		/* 16-bit scale for the filter */
		CLEAR_BIT(can_ip->FS1R, filternbrbitpos);

		/* First 16-bit identifier and First 16-bit mask */
		/* Or First 16-bit identifier and Second 16-bit identifier */
		can_ip->sFilterRegister[sFilterConfig->FilterBank].FR1 =
			((0x0000FFFFU & (uint32_t)sFilterConfig->FilterMaskIdLow) << 16U) |
			(0x0000FFFFU & (uint32_t)sFilterConfig->FilterIdLow);

		/* Second 16-bit identifier and Second 16-bit mask */
		/* Or Third 16-bit identifier and Fourth 16-bit identifier */
		can_ip->sFilterRegister[sFilterConfig->FilterBank].FR2 =
			((0x0000FFFFU & (uint32_t)sFilterConfig->FilterMaskIdHigh) << 16U) |
			(0x0000FFFFU & (uint32_t)sFilterConfig->FilterIdHigh);
		}

		if (sFilterConfig->FilterScale == CAN_FILTERSCALE_32BIT)
		{
		/* 32-bit scale for the filter */
		SET_BIT(can_ip->FS1R, filternbrbitpos);

		/* 32-bit identifier or First 32-bit identifier */
		can_ip->sFilterRegister[sFilterConfig->FilterBank].FR1 =
			((0x0000FFFFU & (uint32_t)sFilterConfig->FilterIdHigh) << 16U) |
			(0x0000FFFFU & (uint32_t)sFilterConfig->FilterIdLow);

		/* 32-bit mask or Second 32-bit identifier */
		can_ip->sFilterRegister[sFilterConfig->FilterBank].FR2 =
			((0x0000FFFFU & (uint32_t)sFilterConfig->FilterMaskIdHigh) << 16U) |
			(0x0000FFFFU & (uint32_t)sFilterConfig->FilterMaskIdLow);
		}

		/* Filter Mode */
		if (sFilterConfig->FilterMode == CAN_FILTERMODE_IDMASK)
		{
		/* Id/Mask mode for the filter*/
		CLEAR_BIT(can_ip->FM1R, filternbrbitpos);
		}
		else /* CAN_FilterInitStruct->CAN_FilterMode == CAN_FilterMode_IdList */
		{
		/* Identifier list mode for the filter*/
		SET_BIT(can_ip->FM1R, filternbrbitpos);
		}

		/* Filter FIFO assignment */
		if (sFilterConfig->FilterFIFOAssignment == CAN_FILTER_FIFO0)
		{
		/* FIFO 0 assignation for the filter */
		CLEAR_BIT(can_ip->FFA1R, filternbrbitpos);
		}
		else
		{
		/* FIFO 1 assignation for the filter */
		SET_BIT(can_ip->FFA1R, filternbrbitpos);
		}

		/* Filter activation */
		if (sFilterConfig->FilterActivation == CAN_FILTER_ENABLE)
		{
		SET_BIT(can_ip->FA1R, filternbrbitpos);
		}

		/* Leave the initialisation mode for the filter */
		CLEAR_BIT(can_ip->FMR, CAN_FMR_FINIT);

		/* Return function status */
		return HAL_OK;
	}
	else
	{
		/* Update error code */
		hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;

		return HAL_ERROR;
	}
#endif
	return HAL_OK;
}
