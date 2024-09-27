/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#ifndef _HAL_GENERAL_DEF_H_
#define _HAL_GENERAL_DEF_H_

#define RTW_MAC_TBTT_AGG_DEF 1

enum rtw_chip_id {
	CHIP_WIFI6_8852A,
	CHIP_WIFI6_8834A,
	CHIP_WIFI6_8852B,
	CHIP_WIFI6_8852C,
	CHIP_WIFI6_MAX
};

enum rtw_efuse_info {
	/* MAC Part */
	EFUSE_INFO_MAC_ADDR,
	EFUSE_INFO_MAC_PID,
	EFUSE_INFO_MAC_DID,
	EFUSE_INFO_MAC_VID,
	EFUSE_INFO_MAC_SVID,
	EFUSE_INFO_MAC_SMID,
	EFUSE_INFO_MAC_MAX,
	/* BB Part */
	EFUSE_INFO_BB_ANTDIV,
	EFUSE_INFO_BB_MAX,
	/* RF Part */
	EFUSE_INFO_RF_PKG_TYPE,
	EFUSE_INFO_RF_PA,
	EFUSE_INFO_RF_VALID_PATH,
	EFUSE_INFO_RF_RFE,
	EFUSE_INFO_RF_TXPWR,
	EFUSE_INFO_RF_BOARD_OPTION,
	EFUSE_INFO_RF_CHAN_PLAN,
	EFUSE_INFO_RF_CHAN_PLAN_FORCE_HW,
	EFUSE_INFO_RF_COUNTRY,
	EFUSE_INFO_RF_THERMAL,
	EFUSE_INFO_RF_2G_CCK_A_TSSI_DE_1,
	EFUSE_INFO_RF_2G_CCK_A_TSSI_DE_2,
	EFUSE_INFO_RF_2G_CCK_A_TSSI_DE_3,
	EFUSE_INFO_RF_2G_CCK_A_TSSI_DE_4,
	EFUSE_INFO_RF_2G_CCK_A_TSSI_DE_5,
	EFUSE_INFO_RF_2G_CCK_A_TSSI_DE_6,
	EFUSE_INFO_RF_2G_BW40M_A_TSSI_DE_1,
	EFUSE_INFO_RF_2G_BW40M_A_TSSI_DE_2,
	EFUSE_INFO_RF_2G_BW40M_A_TSSI_DE_3,
	EFUSE_INFO_RF_2G_BW40M_A_TSSI_DE_4,
	EFUSE_INFO_RF_2G_BW40M_A_TSSI_DE_5,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_1,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_2,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_3,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_4,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_5,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_6,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_7,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_8,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_9,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_10,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_11,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_12,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_13,
	EFUSE_INFO_RF_5G_BW40M_A_TSSI_DE_14,
	EFUSE_INFO_RF_2G_CCK_B_TSSI_DE_1,
	EFUSE_INFO_RF_2G_CCK_B_TSSI_DE_2,
	EFUSE_INFO_RF_2G_CCK_B_TSSI_DE_3,
	EFUSE_INFO_RF_2G_CCK_B_TSSI_DE_4,
	EFUSE_INFO_RF_2G_CCK_B_TSSI_DE_5,
	EFUSE_INFO_RF_2G_CCK_B_TSSI_DE_6,
	EFUSE_INFO_RF_2G_BW40M_B_TSSI_DE_1,
	EFUSE_INFO_RF_2G_BW40M_B_TSSI_DE_2,
	EFUSE_INFO_RF_2G_BW40M_B_TSSI_DE_3,
	EFUSE_INFO_RF_2G_BW40M_B_TSSI_DE_4,
	EFUSE_INFO_RF_2G_BW40M_B_TSSI_DE_5,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_1,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_2,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_3,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_4,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_5,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_6,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_7,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_8,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_9,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_10,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_11,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_12,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_13,
	EFUSE_INFO_RF_5G_BW40M_B_TSSI_DE_14,
	EFUSE_INFO_RF_2G_CCK_C_TSSI_DE_1,
	EFUSE_INFO_RF_2G_CCK_C_TSSI_DE_2,
	EFUSE_INFO_RF_2G_CCK_C_TSSI_DE_3,
	EFUSE_INFO_RF_2G_CCK_C_TSSI_DE_4,
	EFUSE_INFO_RF_2G_CCK_C_TSSI_DE_5,
	EFUSE_INFO_RF_2G_CCK_C_TSSI_DE_6,
	EFUSE_INFO_RF_2G_BW40M_C_TSSI_DE_1,
	EFUSE_INFO_RF_2G_BW40M_C_TSSI_DE_2,
	EFUSE_INFO_RF_2G_BW40M_C_TSSI_DE_3,
	EFUSE_INFO_RF_2G_BW40M_C_TSSI_DE_4,
	EFUSE_INFO_RF_2G_BW40M_C_TSSI_DE_5,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_1,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_2,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_3,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_4,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_5,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_6,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_7,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_8,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_9,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_10,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_11,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_12,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_13,
	EFUSE_INFO_RF_5G_BW40M_C_TSSI_DE_14,
	EFUSE_INFO_RF_2G_CCK_D_TSSI_DE_1,
	EFUSE_INFO_RF_2G_CCK_D_TSSI_DE_2,
	EFUSE_INFO_RF_2G_CCK_D_TSSI_DE_3,
	EFUSE_INFO_RF_2G_CCK_D_TSSI_DE_4,
	EFUSE_INFO_RF_2G_CCK_D_TSSI_DE_5,
	EFUSE_INFO_RF_2G_CCK_D_TSSI_DE_6,
	EFUSE_INFO_RF_2G_BW40M_D_TSSI_DE_1,
	EFUSE_INFO_RF_2G_BW40M_D_TSSI_DE_2,
	EFUSE_INFO_RF_2G_BW40M_D_TSSI_DE_3,
	EFUSE_INFO_RF_2G_BW40M_D_TSSI_DE_4,
	EFUSE_INFO_RF_2G_BW40M_D_TSSI_DE_5,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_1,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_2,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_3,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_4,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_5,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_6,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_7,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_8,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_9,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_10,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_11,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_12,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_13,
	EFUSE_INFO_RF_5G_BW40M_D_TSSI_DE_14,
	EFUSE_INFO_RF_THERMAL_A,
	EFUSE_INFO_RF_THERMAL_B,
	EFUSE_INFO_RF_THERMAL_C,
	EFUSE_INFO_RF_THERMAL_D,
	EFUSE_INFO_RF_XTAL,
	/*RX Gain K*/
        EFUSE_INFO_RF_RX_GAIN_K_A_2G_CCK,
        EFUSE_INFO_RF_RX_GAIN_K_A_2G_OFMD,
        EFUSE_INFO_RF_RX_GAIN_K_A_5GL,
        EFUSE_INFO_RF_RX_GAIN_K_A_5GM,
        EFUSE_INFO_RF_RX_GAIN_K_A_5GH,
	EFUSE_INFO_RF_MAX,
	/* BTCOEX Part */
	EFUSE_INFO_BTCOEX_COEX,
	EFUSE_INFO_BTCOEX_ANT_NUM,
	EFUSE_INFO_BTCOEX_ANT_PATH,
	EFUSE_INFO_BTCOEX_MAX,
};

enum rtw_cv {
	CAV,
	CBV,
	CCV,
	CDV,
	CEV,
	CFV,
	CGV,
	CTV,
	CMAXV,
};

enum rtw_fv {
	FTV,
	FUV,
	FSV,
};

enum rtw_dv_sel {
	DAV,
	DDV,
};

enum hal_pwr_by_rate_setting {
	PW_BY_RATE_ON = 0,
	PW_BY_RATE_ALL_SAME = 1
};

enum hal_pwr_limit_type {
	PWLMT_BY_EFUSE = 0,
	PWLMT_DISABLE = 1,
	PWBYRATE_AND_PWLMT = 2
};

enum wl_func {
	EFUSE_WL_FUNC_NONE = 0,
	EFUSE_WL_FUNC_DRAGON = 0xe,
	EFUSE_WL_FUNC_GENERAL = 0xf
};

enum hw_stype{
	EFUSE_HW_STYPE_NONE = 0x0,
	EFUSE_HW_STYPE_GENERAL = 0xf
};
#endif /* _HAL_GENERAL_DEF_H_*/

