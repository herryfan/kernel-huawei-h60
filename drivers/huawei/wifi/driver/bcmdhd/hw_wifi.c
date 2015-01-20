#include <linux/module.h>
#include <linux/init.h>
#include <wlioctl.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <hw_wifi.h>

/************修改记录************************
版本:      	日期:              		说明:
V01			2014/07/02			增加国家码黑名单


**************************************************/
#define BLACK_LIST_MAX 3
char *black_list[BLACK_LIST_MAX]={"IR", "CU", "KP"};


/*
判断是否是受限的国家: 
返回值:
0: 不是受限国家
1: 是受限国家
*/
int is_blacklist_country(char *country)
{
	int i = 0;

	if(country == NULL){
		return 0;
	}

	printk(KERN_ERR "input country code: %s.\n", country);
	for(i=0; i<BLACK_LIST_MAX; i++){
		 if(strncmp(black_list[i], country, strlen(black_list[i])) == 0){
			return 1;
		 }
	}

	return 0;
}



