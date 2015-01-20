#Copyright Huawei Technologies Co., Ltd. 1998-2011. All rights reserved.
#This file is Auto Generated 

dtb-y += MOGOLIA_TL_V3_config.dtb
dtb-y += MOGOLIA_TL_V1_config.dtb
dtb-y += MOGOLIA_TL_V3_noNFC_config.dtb
dtb-y += MOGOLIA_ULG_V3_config.dtb
dtb-y += MOGOLIA_ULCA_V1_noNFC_config.dtb
dtb-y += MOGOLIA_TL_CMCC_CERTIFICATION_noNFC_config.dtb
dtb-y += MOGOLIA_ULCA_GENERAL_V3_noNFC_config.dtb
dtb-y += MOGOLIA_ULG_V3_noNFC_config.dtb
dtb-y += MOGOLIA_TL_V1_TRIAL_config.dtb
dtb-y += MOGOLIA_ULG_V1_config.dtb

targets += dtbs
targets += $(dtb-y)

# *.dtb used to be generated in the directory above. Clean out the
# old build results so people don't accidentally use them.
dtbs: $(addprefix $(obj)/, $(dtb-y))
	$(Q)rm -f $(obj)/../*.dtb

clean-files := *.dtb

#end of file
