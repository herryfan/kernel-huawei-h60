

#include <linux/module.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/hw_log.h>
#include "dsm_core.h"
#include "dsm_lib.h"

#define HWLOG_TAG	DSM
HWLOG_REGIST();
#define DSM_LOG_INFO(x...)		_hwlog_info(HWLOG_TAG, ##x)
#define DSM_LOG_ERR(x...)		_hwlog_err(HWLOG_TAG, ##x)
#define DSM_LOG_DEBUG(x...)	_hwlog_debug(HWLOG_TAG, ##x)

static struct dsm_server g_dsm_server;
static struct work_struct dsm_work;

struct dsm_client *dsm_register_client (struct dsm_dev *dev)
{
	int i;
	int size;
	int conflict = -1;
	struct dsm_client *ptr = NULL;

	if(g_dsm_server.server_state != DSM_SERVER_INITED){
		DSM_LOG_ERR("dsm server uninited\n");
		goto out;
	}

	if(dev == NULL){
		DSM_LOG_ERR("dsm_dev is NULL\n");
		goto out;
	}

	smp_rmb();
	if(g_dsm_server.client_count < CLIENT_SIZE){
		ptr = (struct dsm_client *)kzalloc((sizeof(struct dsm_client)+dev->buff_size), GFP_KERNEL);
		if(!ptr){
			DSM_LOG_ERR("clients malloc failed\n");
			goto out;
		}

		mutex_lock(&g_dsm_server.mtx_lock);
		for(i=0; i<CLIENT_SIZE; i++){
			if(!test_bit(DSM_CLIENT_VAILD_BIT, &g_dsm_server.client_flag[i]))
				break;
			conflict = strncmp(g_dsm_server.client_list[i]->client_name, dev->name, CLIENT_NAME_LEN);
			if(!conflict){
				DSM_LOG_ERR("new client %s conflict with No.%d client %s\n",
					dev->name, i, g_dsm_server.client_list[i]->client_name);
				break;
			}
		}

		if(i < CLIENT_SIZE && conflict){
			size = strlen(dev->name);
			size = (size < CLIENT_NAME_LEN) ? size : (CLIENT_NAME_LEN - 1);
			memcpy(ptr->client_name, dev->name, size);
			ptr->client_id = i;
			ptr->cops = dev->fops;
			ptr->buff_size = dev->buff_size;
			init_waitqueue_head(&ptr->waitq);
			g_dsm_server.client_list[i] = ptr;
			set_bit(DSM_CLIENT_VAILD_BIT, &g_dsm_server.client_flag[i]);
			g_dsm_server.client_count++;
			DSM_LOG_INFO("client %s register success!\n", ptr->client_name);
			smp_wmb();
		}else{
			DSM_LOG_ERR("clients register failed, index %d, conflict %d\n", i, conflict);
			kfree(ptr);
			ptr = NULL;
		}
		mutex_unlock(&g_dsm_server.mtx_lock);
	}
	else
		DSM_LOG_INFO("clients has full\n");

out:
	return ptr;
}

inline int dsm_client_ocuppy(struct dsm_client *client)
{
	int ret = -1;
	if(client){
		smp_rmb();
		ret = test_and_set_bit(CBUFF_OCCUPY_BIT, &client->buff_flag);
	}
	return ret;
}

inline int dsm_client_unocuppy(struct dsm_client *client)
{
	int ret = -1;
	if(client){
		clear_bit(CBUFF_OCCUPY_BIT, &client->buff_flag);
		smp_wmb();
		ret =0;
	}
	return ret;
}

void dsm_client_notify(struct dsm_client *client, int error_no)
{
	if(client){
		client->error_no = error_no;
		set_bit(CBUFF_READY_BIT, &client->buff_flag);
		set_bit(DSM_CLIENT_NOTIFY_BIT, &g_dsm_server.client_flag[client->client_id]);
		smp_wmb();
		queue_work(g_dsm_server.dsm_wq, &dsm_work);
	}
	return;
}

int dsm_client_record(struct dsm_client *client, const char *fmt, ...)
{
	va_list ap;
	int size = 0;
	char *str;
	struct snprintf_ctxt ctxt;

	if(!client){
		DSM_LOG_ERR("%s no client to record\n", __func__);
		goto out;
	}

	if(client->buff_size <= client->used_size){
		DSM_LOG_ERR("%s no buffer to record\n", __func__);
		goto out;
	}

	ctxt.avail = client->buff_size - client->used_size -1;
	str = (char *)&client->dump_buff[client->used_size];
	ctxt.next = str;

	va_start(ap, fmt);
	__xprintf(fmt, ap, printf_putc, &ctxt);
	va_end(ap);

	*ctxt.next = 0;
	size = ctxt.next - str;
	client->used_size += size;

out:
	return size;
}

int dsm_client_copy(struct dsm_client *client, void *src, int sz)
{
	int size = 0;

	if(!client){
		DSM_LOG_ERR("%s no client to record\n", __func__);
		goto out;
	}

	if((client->used_size + sz) > client->buff_size){
		DSM_LOG_ERR("%s no enough buffer to record\n", __func__);
		goto out;
	}

	size = sz;
	memcpy(&client->dump_buff[client->used_size], src, size);
	client->used_size += size;

out:
	return size;
}

static inline int dsm_client_readable(struct dsm_client *client)
{
	int ret = 0;
	if(client){
		smp_rmb();
		ret = test_bit(CBUFF_READY_BIT, &client->buff_flag);
	}
	return ret;
}

static inline int dsm_atoi(const char* p){
	int val = 0;

	if(!p)
		return -1;

	while(isdigit(*p))
		val = val*10 + (*p++ - '0');

	return val;
}

static inline char* dsm_strtok(char* string_org,const char* demial)
{
	static unsigned char* last;
	unsigned char* str;
	const unsigned char* ctrl = (const unsigned char*)demial;
	unsigned char map[32];
	int count;

	for (count =0; count <32; count++)
		map[count] = 0;

	do{
		map[*ctrl >> 3] |= (1 << (*ctrl & 7));
	} while (*ctrl++);

	if (string_org)
		str = (unsigned char*)string_org;
	else
		str = last;

	while ((map[*str >> 3] & (1 << (*str & 7)))  && *str)
		str++;

	string_org = (char*)str;
	for (;*str; str++){
		if (map[*str >> 3] & (1 << (*str & 7))){
			*str++ = '\0';
			break;
		}       
	}
	last =str;
	if (string_org == (char*)str)
		return NULL;
	else
		return string_org;
}

static inline int copy_int_to_user(void __user *argp, int val)
{
	int ret;
	int size;
	char buff[UINT_BUF_MAX]={0};

	size = snprintf(buff, UINT_BUF_MAX, "%d\n", val);
	ret = copy_to_user(argp, buff, size);
	DSM_LOG_DEBUG("%s result %d\n",__func__, ret);
	return ret;
}

static struct dsm_client *dsm_find_client(char *cname)
{
	int i;
	struct dsm_client * client = NULL;

	mutex_lock(&g_dsm_server.mtx_lock);
	smp_rmb();
	for(i=0; i<CLIENT_SIZE; i++){
		if((test_bit(DSM_CLIENT_VAILD_BIT, &g_dsm_server.client_flag[i]))
			&& (!strncasecmp(g_dsm_server.client_list[i]->client_name, cname, CLIENT_NAME_LEN))){
			client = g_dsm_server.client_list[i];
			break;
		}
	}
	mutex_unlock(&g_dsm_server.mtx_lock);
	DSM_LOG_DEBUG("cname: %s find %s\n", cname, client?"success":"failed");

	return client;
}

static inline void dsm_client_set_idle(struct dsm_client *client)
{
	client->used_size = 0;
	client->read_size = 0;
	client->error_no = 0;
	memset(client->dump_buff, 0, client->buff_size);
	clear_bit(CBUFF_READY_BIT, &client->buff_flag);
	clear_bit(CBUFF_OCCUPY_BIT, &client->buff_flag);
	clear_bit(DSM_CLIENT_NOTIFY_BIT, &g_dsm_server.client_flag[client->client_id]);
	smp_wmb();
	return;
}

static inline void dsm_bind_client(struct dsm_client *client)
{
	return;
}


static inline void dsm_unbind_client(struct dsm_client *client)
{
	return;
}

static void dsm_work_func(struct work_struct *work)
{
	int i;
	struct dsm_client *client;

	DSM_LOG_DEBUG("%s enter\n", __func__);
	mutex_lock(&g_dsm_server.mtx_lock);
	smp_rmb();
	for(i=0; i<CLIENT_SIZE; i++){
		if(test_bit(DSM_CLIENT_VAILD_BIT, &g_dsm_server.client_flag[i])){
			DSM_LOG_DEBUG("No.%d client name %s flag 0x%lx\n", i,
				g_dsm_server.client_list[i]->client_name, g_dsm_server.client_flag[i]);
			if(!test_and_clear_bit(DSM_CLIENT_NOTIFY_BIT, &g_dsm_server.client_flag[i]))
				continue;
			client = g_dsm_server.client_list[i];
			wake_up_interruptible_all(&client->waitq);
			DSM_LOG_INFO("%s finish notify\n", client->client_name);
		}
	}
	mutex_unlock(&g_dsm_server.mtx_lock);
	DSM_LOG_DEBUG("%s exit\n", __func__);

	return;
}

static ssize_t dsm_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct dsm_client *client = file->private_data;
	size_t copy_size = 0;

	DSM_LOG_DEBUG("%s enter\n",__func__);

	if(!client){
		DSM_LOG_ERR("client not bind\n");
		goto out;
	}

	if(dsm_client_readable(client)){
		copy_size = min(count, (client->used_size -client->read_size));
		if(copy_to_user(buf, &client->dump_buff[client->read_size], copy_size))
			DSM_LOG_ERR("copy to user failed\n");
		client->read_size += copy_size;
		if(client->read_size >= client->used_size)
			dsm_client_set_idle(client);
		DSM_LOG_DEBUG("%d bytes read to user\n", copy_size);
	}

out:
	DSM_LOG_DEBUG("%s exit\n",__func__);
	return copy_size;
}

static ssize_t dsm_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	char client_name[CLIENT_NAME_LEN]={0};
	int size;
	struct dsm_client *client = NULL;
	char *buff = NULL;
	char *ptr;

	DSM_LOG_DEBUG("%s enter\n",__func__);
	buff = (char *)kzalloc(count, GFP_KERNEL);
	if(!buff){
		DSM_LOG_ERR("dsm write malloc failed\n");
		goto out;
	}

	if(copy_from_user(buff, buf, count)){
		DSM_LOG_ERR("dsm write copy failed\n");
		goto out;
	}

	ptr = buff;
	while(*ptr){
		if(*ptr == '\n')
			break;
		ptr++;
	}

	if(*ptr == '\n'){
		size = ptr - buff;
		size = (size < CLIENT_NAME_LEN) ? size : (CLIENT_NAME_LEN - 1);
		memcpy(client_name, buff, size);
		client = dsm_find_client(client_name);
		if(client && (!dsm_client_ocuppy(client))){
			DSM_LOG_DEBUG("dsm write find client - %s\n", client_name);
			ptr++;//ignore '/n'
			size = count - (ptr - buff);
			dsm_client_copy(client, ptr, size);
			dsm_client_notify(client, 20203);
		}else
			DSM_LOG_INFO("dsm write can't find client - %s\n", client_name);
	}else
		DSM_LOG_ERR("dsm write can't find client name\n");

out:
	if(buff)
		kfree(buff);
	DSM_LOG_DEBUG("%s exit\n",__func__);
	return count;
}

static unsigned int dsm_poll(struct file *file, poll_table *wait)
{
	struct dsm_client *client = file->private_data;
	unsigned int mask = 0;

	DSM_LOG_DEBUG("%s enter\n",__func__);
	if(!client){
		DSM_LOG_ERR("dsm can't poll without client\n");
		goto out;
	}
	DSM_LOG_DEBUG("client name :%s\n", client->client_name);
	poll_wait(file, &client->waitq, wait);
	if(test_bit(CBUFF_READY_BIT, &client->buff_flag))
		mask = POLLIN | POLLRDNORM;

out:
	DSM_LOG_DEBUG("%s exit, mask:%d\n",__func__, mask);
	return mask;
}

static int dsm_open(struct inode *inode, struct file *file)
{
	DSM_LOG_DEBUG("%s enter\n",__func__);
	file->private_data = NULL;
	DSM_LOG_DEBUG("%s exit\n",__func__);
	return 0;
}

static int dsm_close(struct inode *inode, struct file *file)
{
	struct dsm_client *client = file->private_data;

	DSM_LOG_DEBUG("%s enter\n",__func__);
	if(client)
		dsm_unbind_client(client);
	DSM_LOG_DEBUG("%s exit\n",__func__);
	return 0;
}

static long dsm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct dsm_client *client = (struct dsm_client *)file->private_data;
	long ret = 0;
	int error = 0;
	char buff[CLIENT_NAME_LEN]={0};

	DSM_LOG_DEBUG("%s enter,\n",__func__);

	switch (cmd) {
		case DSM_IOCTL_GET_CLIENT_COUNT:
			mutex_lock(&g_dsm_server.mtx_lock);
			error = g_dsm_server.client_count;
			mutex_unlock(&g_dsm_server.mtx_lock);
			DSM_LOG_INFO("client count :%d\n", error);
			ret = copy_int_to_user(argp, error);
			break;
		case DSM_IOCTL_BIND:
			if (copy_from_user(buff, argp, CLIENT_NAME_LEN)){
				DSM_LOG_ERR("copy from user failed\n");
				ret = -EFAULT;
			}else{
				DSM_LOG_DEBUG("try bind client %s\n", buff);
				client = dsm_find_client(buff);
				if(client){
					dsm_bind_client(client);
					file->private_data = (void *)client;
				}
				else{
					DSM_LOG_ERR("dsm find client %s failed\n", buff);
					ret = -ENXIO;
				}
			}
			break;
		case DSM_IOCTL_POLL_CLIENT_STATE:
			if(client && client->cops && client->cops->poll_state){
				error = client->cops->poll_state();
				DSM_LOG_INFO("poll %s state result :%d\n", client->client_name, error);
				ret = copy_int_to_user(argp, error);
			}
			else{
				DSM_LOG_ERR("dsm client not bound or poll not support\n");
				ret = -ENXIO;
			}
			break;
		case DSM_IOCTL_FORCE_DUMP:
			if (copy_from_user(buff, argp, UINT_BUF_MAX)){
				DSM_LOG_ERR("copy from user failed\n");
				ret = -EFAULT;
			}else{
				if(client && client->cops && client->cops->dump_func){
					if(!dsm_client_ocuppy(client)){
						client->error_no = dsm_atoi(buff);
						client->used_size = client->cops->dump_func(client->error_no,(void *)client->dump_buff, (int)client->buff_size);
						set_bit(CBUFF_READY_BIT, &client->buff_flag);
					}
					else{
						DSM_LOG_INFO("client %s's buff ocuppy failed\n", client->client_name);
						ret = -EBUSY;
					}
				}
				else{
					DSM_LOG_ERR("dsm client not bound or dump not support\n");
					ret = -ENXIO;
				}
			}
			break;
		case DSM_IOCTL_GET_CLIENT_ERROR:
			if(client)
				ret = copy_int_to_user(argp, client->error_no);
			else{
				DSM_LOG_ERR("dsm find client failed\n");
				ret = -ENXIO;
			}
			break;
		default:
			DSM_LOG_ERR("unknown ioctl command :%d\n", cmd);
			break;
	}

	DSM_LOG_DEBUG("%s exit\n",__func__);
	return ret;
}

static const struct file_operations dsm_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.unlocked_ioctl	= dsm_ioctl,
	.open		= dsm_open,
	.release		= dsm_close,
	.read			= dsm_read,
	.write		= dsm_write,
	.poll			= dsm_poll,
};

//cnotify format: |client name|,|error no|,|contents|
static ssize_t dsm_cnotify_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	char client_name[CLIENT_NAME_LEN]={0};
	int size;
	int error_no = 0;
	struct dsm_client *client = NULL;
	char *strings = NULL;
	char *ptr;

	DSM_LOG_DEBUG("%s enter\n",__func__);
	strings = (char *)kzalloc(count, GFP_KERNEL);
	if(!strings){
		DSM_LOG_ERR("dsm write malloc failed\n");
		goto out;
	}

	memcpy(strings, buf, count);
	/*get client name*/
	ptr = dsm_strtok(strings, ",");
	if(ptr){
		size = strlen(ptr);
		size = (size < CLIENT_NAME_LEN) ? size : (CLIENT_NAME_LEN - 1);
		memcpy(client_name, ptr, size);
	}
	/*get error no*/
	ptr = dsm_strtok(NULL, ",");
	if(ptr)
		error_no = dsm_atoi(ptr);
	/*get notify content*/
	ptr = dsm_strtok(NULL, ",");

	DSM_LOG_INFO("client name - %s, error no - %d\n", client_name, error_no);
	if(ptr)
		DSM_LOG_INFO("content - %s\n", ptr);

	client = dsm_find_client(client_name);
	if(client && (!dsm_client_ocuppy(client))){
		DSM_LOG_DEBUG("dsm write find client - %s\n", client_name);
		if(ptr)
			dsm_client_copy(client, ptr, strlen(ptr));
		dsm_client_notify(client, error_no);
	}else
		DSM_LOG_INFO("dsm notify can't find client - %s\n", client_name);

out:
	if(strings)
		kfree(strings);
	DSM_LOG_DEBUG("%s exit\n",__func__);
	return count;
}

static struct device_attribute dsm_interface_attrs[] = {
	__ATTR(client_notify, S_IWUSR, NULL, dsm_cnotify_store),
};

static struct dsm_dev dsm_pub_clients[] = {
/*	[0] = {
		.name = "smartpa",
		.fops = NULL,
		.buff_size = 1024,
	},
*/
};

static void __init dsm_register_public_client(void)
{
	int index;
	struct dsm_client *client = NULL;

	for(index = 0; index < ARRAY_SIZE(dsm_pub_clients); index++){
		client = dsm_register_client(&dsm_pub_clients[index]);
		if(client)
			DSM_LOG_DEBUG("register %d public client - %s success\n", index,
				dsm_pub_clients[index].name);
		else
			DSM_LOG_ERR("register %d public client - %s failed\n", index,
				dsm_pub_clients[index].name);
	}
}

static struct miscdevice dsm_miscdev = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "dsm",
	.fops		= &dsm_fops,
};

static int __init dsm_init(void)
{
	int ret = -EIO;
	int i;

	memset(&g_dsm_server, 0, sizeof(struct dsm_server));
	g_dsm_server.server_state = DSM_SERVER_UNINIT;
	mutex_init(&g_dsm_server.mtx_lock);

	g_dsm_server.dsm_wq = create_singlethread_workqueue("dsm_wq");
	if (IS_ERR(g_dsm_server.dsm_wq)){
		DSM_LOG_ERR("alloc workqueue failed\n");
		goto out;
	}

	INIT_WORK(&dsm_work, dsm_work_func);

	ret = misc_register(&dsm_miscdev);
	if(ret){
		DSM_LOG_ERR("misc register failed\n");
		goto out;
	}

	g_dsm_server.server_state = DSM_SERVER_INITED;

	for (i = 0; i < ARRAY_SIZE(dsm_interface_attrs); i++) {
		ret = device_create_file(dsm_miscdev.this_device,
				&dsm_interface_attrs[i]);
		if (ret < 0)
			DSM_LOG_ERR("creating sysfs attribute %s failed: %d\n",
					dsm_interface_attrs[i].attr.name, ret);
	}

	dsm_register_public_client();

out:
	DSM_LOG_INFO("%s called, ret %d\n", __func__, ret);
	return ret;
}
subsys_initcall(dsm_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Device state monitor");