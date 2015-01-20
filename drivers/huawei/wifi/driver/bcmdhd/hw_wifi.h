#ifndef __HW_WIFI_H__
#define __HW_WIFI_H__

/*********************************************
版本:				日期:		解决问题:
HUAWEI-2014.001:  	0529		solve wifi panic
HUAWEI-2014.002:       0609		solve scan_done panic.
HUAWEI-2014.003:       0613             throughput optimize.
HUAWEI-2014.004:       0626             solve wdev_cleanup_work panic.
HUAWEI-2014.005:       0702             solve country code problem.
**********************************************/
#define        HUAWEI_VERSION_STR ", HUAWEI-2014.005"

extern int is_blacklist_country(char *country);


#endif
