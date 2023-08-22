/* 
   Mod by (TCTSZ) zhaohong.chen@tcl.com 20150729 for creating current_sensor node
*/
#include "camera_tct_func.h"

static struct kobject *sensor_node = NULL;
static const char *sub_name,*sub_size = NULL;
static const char *main_name,*main_size = NULL;
static const char *cur_name,*cur_size = NULL;
static const char *source= NULL;
char dynamic_regs_debug = 0;
static struct msm_sensor_ctrl_t *cur_ctrl = NULL;
static int32_t cur_reg_addr = -1;
#ifdef JRD_PROJECT_POP45
extern uint8_t fl_pos;
#endif

#if defined(JRD_PROJECT_PIXI464G) || defined(JRD_PROJECT_PIXI464GCRICKET)
int cur_position = -1;
#endif

/****************************** Sys IO func ************************************************/

static ssize_t sub_sensor_info_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	if(sub_name && sub_size)
	{
		sprintf(buf, "%s %s\n",sub_name,sub_size);
		ret = strlen(buf) + 1;
	}
	return ret;
}

static ssize_t main_sensor_info_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	if(main_name && main_size)
	{
		sprintf(buf, "%s %s %s\n",main_name,main_size,source);
		ret = strlen(buf) + 1;
	}
	return ret;
}

static ssize_t cur_sensor_info_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	if(cur_name && cur_size)
	{
		sprintf(buf, "%s %s %s\n",cur_name,cur_size, source);
		ret = strlen(buf) + 1;
	}
	return ret;
}

static ssize_t dynamic_regs_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	sprintf(buf, "%d\n",dynamic_regs_debug);
	ret = strlen(buf) + 1;
	return ret;
}

static ssize_t dynamic_regs_set(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	CDBG("%s E\n",__func__);
	if(*buf == '0')
	{
		dynamic_regs_debug = 0;
	}
	else if(*buf == '1')
	{
		dynamic_regs_debug = 1;
	}
	else
	{
		CAM_ERR("%s set noting\n",__func__);
	}
        if(strstr(buf,"dynamic"))
		camera_dynamic_regs_debug(cur_ctrl);
	CDBG("%s X\n",__func__);
	return count;
}

static ssize_t reg_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	int32_t rc = -1;
	uint16_t cur_reg_data = -1;

	if(cur_ctrl == NULL || cur_reg_addr < 0)
	{
		CAM_ERR("Plz open camera and then set the reg !\n");
		sprintf(buf, "Plz open camera and set reg !\n");
	}
	else
	{
		rc = cur_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
			cur_ctrl->sensor_i2c_client, (uint16_t)cur_reg_addr,
			&cur_reg_data, MSM_CAMERA_I2C_BYTE_DATA);
		
		if( rc < 0 )
		{
			CAM_ERR("read 0x%x failed!\n",cur_reg_addr);
			sprintf(buf, "read 0x%x failed\n",cur_reg_addr);
		}
		else
		{
			sprintf(buf, "0x%x : 0x%x\n",cur_reg_addr,cur_reg_data);
		}
	}
	
	ret = strlen(buf) + 1;
	return ret;
}

static ssize_t reg_set(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	char *pos = (char*)buf;
	
	CDBG("%s,buf=%s,count=%d  E\n",__func__,buf,count);

	if(*pos != '0' || (*(pos+1) != 'x' && *(pos+1) != 'X') || count <= 4)
	{
		CAM_ERR("Invaild!\n");
		return count;
	}
	else
	{
		pos += 2;
		cur_reg_addr = 0;
	}
	
	while( pos < (buf+count))
	{
		if(*pos >= '0' && *pos <= '9')
		{
			cur_reg_addr = cur_reg_addr * 16 + *pos - '0';
		}
		else if(*pos >= 'A' && *pos <= 'F')
		{
			cur_reg_addr = cur_reg_addr * 16 + *pos - 'A' + 10;
		}
		else if(*pos >= 'a' && *pos <= 'f')
		{
			cur_reg_addr = cur_reg_addr * 16 + *pos - 'a' + 10;
		}
		else
		{
			CAM_ERR("%s,parse reg error:%c\n",__func__,*pos);
			break;
		}
		pos++;
	}

	CDBG("%s,reg=0x%x(%d)  X\n",__func__,cur_reg_addr,cur_reg_addr);
	return count;
}


/******************************Sensor_name match func************************************************/
static int32_t sensor_name_match(char* sensor_name,int position){
	int32_t ret = -1;
	char * match_name=NULL;
	char * match_size=NULL;	

	if(strstr(sensor_name,"ov8858")){
		match_name="OV8858";
		match_size="8M";
		ret = 0;
	}
	
	if(strstr(sensor_name,"ov5670")){
		match_name="OV5670";
		match_size="5M";
		ret = 0;
	}


#if defined (JRD_PROJECT_MICKEY6TFUMTS5046G)
	if(strstr(sensor_name,"ov5675")){
		match_name="ov5675_sunrise";
		match_size="5M";
		ret = 0;
	}
	if(strstr(sensor_name,"ov8856_qtech")){
		match_name="ov8856_qtech";
		match_size="8M";
		ret = 0;
	}

#endif
/*[BUGFIX]-Mod-BEGIN by TCTSZ.(gaoxiang.zou@tcl.com),  12/18/2015*/	
#if defined(JRD_PROJECT_PIXI464G) || defined(JRD_PROJECT_PIXI464GCRICKET)

	if(strstr(sensor_name,"ov8858_sunny")){
		match_name="ov8858_sunny_rear_1st_zgx";
		match_size="8M";
		ret = 0;
	}
	
	if(strstr(sensor_name,"ov5670_qtech")){
		match_name="ov5670_qtech_front_1st_zgx";
		match_size="5M";
		ret = 0;
	}
#endif

/* Add by jinlin.Hu for Buzz6T_4G_gophone MMI. 2016-11-2. Start.*/
#if defined(JRD_PROJECT_BUZZ6T4GGOPHONE)||defined(JRD_PROJECT_BUZZ6T4GTFUMTS) || defined(JRD_PROJECT_BUZZ6T4GTFEVDO)
    if(strstr(sensor_name,"ov5675")){
                match_name="ov5675_sunrise";
                match_size="5M";
                ret = 0;
        }
    if(strstr(sensor_name,"gc2375")){
                match_name="gc2375_ewlley";
                match_size="2M";
                ret = 0;
        }
    //add by jin.xia for 2nd supply rear camera,2016-10-14
    if(strstr(sensor_name,"gc5005")){
                match_name="gc5005_ewlley";
                match_size="5M";
                ret = 0;
        }
    //end add

    if(strstr(sensor_name,"hi553")){
                match_name="hi553";
                match_size="5M";
                ret = 0;
    }

    if(strstr(sensor_name,"sp2509")){
                match_name="sp2509_sunrise";
                match_size="2M";
                ret = 0;
    }
#endif
/* Add by jinlin.Hu for Buzz6T_4G_gophone MMI. 2016-11-2. End.*/
/* Add by yinsheng.fu for MICKEY6TFUMTS MMI. 2016-11-2. Begin.*/
#if defined(JRD_PROJECT_MICKEY6TFUMTS)||defined(JRD_PROJECT_MICKEY6TFUMTS)
        if(strstr(sensor_name,"ov5675")){
		match_name="ov5675_sunrise";
		match_size="5M";
		ret = 0;
	}
    if(strstr(sensor_name,"gc2375")){
		match_name="gc2375_ewlley";
		match_size="2M";
		ret = 0;
	}
#endif
/* Add by yinsheng.fu for MICKEY6TFUMTS MMI. 2016-11-2. End.*/
	/*[BUGFIX]-Mod-END by TCTSZ.(gaoxiang.zou@tcl.com), */
    //add  by tanli 2016-8-10 
//mod by jin.xia for mickey6 tfevdo/vzw ,2016-11-1
#if defined(JRD_PROJECT_MICKEY6TFEVDO) || defined(JRD_PROJECT_MICKEY6VZW) //||defined(JRD_PROJECT_MICKEY6TFUMTS) || defined(JRD_PROJECT_BUZZ6T4GGOPHONE) 
        if(strstr(sensor_name,"ov5675")){
		match_name="ov5675_sunrise";
		match_size="5M";
		ret = 0;
	}
    if(strstr(sensor_name,"gc2375")){
		match_name="gc2375_ewlley";
		match_size="2M";
		ret = 0;
	}
    //add by jin.xia for 2nd supply rear camera,2016-10-14
    if(strstr(sensor_name,"gc5005")){
		match_name="gc5005_ewlley";
		match_size="5M";
		ret = 0;
	}
	//add by jin.xia for 2nd front camera ,2016-10-31
	if(strstr(sensor_name,"sp2509")){
		match_name="sp2509_sunrise";
		match_size="2M";
		ret = 0;
        }
	//add by jin.xia for mickey6 vzw 1st camera ,2016-11-1
	//pr_err("tct_camera_info sensor_name :%s \n",sensor_name);
	if(strstr(sensor_name,"ov5675_ewelly")){
		match_name="ov5675_ewelly";
		match_size="5M";
		ret = 0;
        }
        if(strstr(sensor_name,"ov8856_qtech")){
		match_name="ov8856_qtech";
		match_size="8M";
		ret = 0;
        }
    //end add

#else
	if(strstr(sensor_name,"s5k5e8")){
		match_name="S5K5E8";
		match_size="5M";
		ret = 0;
	}
	
	if(strstr(sensor_name,"s5k5e2")){
		match_name="S5K5E2";
		match_size="5M";
		ret = 0;
	}

	if(strstr(sensor_name,"gc2355")){
		match_name="GC2355";
		match_size="2M";

		ret = 0;
	}

	if(strstr(sensor_name,"ov2680")){
		match_name="OV2680";
		match_size="2M";
		ret = 0;
	}

	if(strstr(sensor_name,"sp2508")){
		match_name="SP2508";
		match_size="2M";
		ret = 0;
	}

	if(strstr(sensor_name,"gc0310")){
		match_name="GC0310";
		match_size="0.3M";
		ret = 0;
	}
    //add by jin.xia for sensor s5k4h8 Hi553 info 2015-12-23
    if(strstr(sensor_name,"s5k4h8")){
		match_name="s5k4h8";
		match_size="8M";
		ret = 0;
	}
	//mod sensor name by jin.xia@tcl.com,2016-1-14
	if(strstr(sensor_name,"hi553")){
		match_name="Hi553";
		match_size="5M";
		ret = 0;
	}
       //add  by jin.xia 2016-4-27, 
        if(strstr(sensor_name,"ov8856")){
		match_name="ov8856";
		match_size="8M";
		ret = 0;
	}
    //end add
#endif	
  	if(strstr(sensor_name,"gc2385")){
		match_name="gc2385";
		match_size="2M";
		source = "1s";
		ret = 0;
	}
  	if(strstr(sensor_name,"gc2395")){
		match_name="gc2395";
		match_size="2M";
		source = "2s";
		ret = 0;
	}
  	if(strstr(sensor_name,"bf2253L")){
		match_name="bf2253L";
		match_size="2M";
		source = "2s";
		ret = 0;
	}
  	if(strstr(sensor_name,"gc030a")){
		match_name="gc030a";
		match_size="0.3M";
		source = "1s";
		ret = 0;
	}
  	if(strstr(sensor_name,"bf20a1")){
		match_name="bf20a1";
		match_size="0.3M";
		source = "2s";
		ret = 0;
	}
	
	if(position)
	{
		sub_name=match_name;
		sub_size=match_size;
	}
	else
	{
		main_name=match_name;
		main_size=match_size;	
	}

	return ret;
};

/*****************************************Gobal Func********************************************/

void cur_sensor_update(struct msm_sensor_ctrl_t *s_ctrl)
{	
	if( s_ctrl == NULL )
	{
		CAM_ERR("s_ctrl = NULL\n");
		return;
	}
	
	if(s_ctrl->sensordata->sensor_info->position)
	{
		cur_name=sub_name;
		cur_size=sub_size;
	}
	else
	{
		cur_name=main_name;
		cur_size=main_size;
	}
	
	cur_ctrl = s_ctrl;
	cur_reg_addr = -1;
	
	#ifdef JRD_PROJECT_POP45
	fl_pos = (int)s_ctrl->sensordata->sensor_info->position;
	CAM_ERR("%s: fl_pos:%d\n",__func__,fl_pos);
	#endif
	
	/*[BUGFIX]-Mod-BEGIN by TCTSZ.(gaoxiang.zou@tcl.com),  12/18/2015*/	
#if defined(JRD_PROJECT_PIXI464G) || defined(JRD_PROJECT_PIXI464GCRICKET)
	cur_position = (int)s_ctrl->sensordata->sensor_info->position;
	printk("%s: cur_position:%d\n",__func__,cur_position);
#endif
	
}


static DEVICE_ATTR(sub_sensor, 0444, sub_sensor_info_show, NULL);
static DEVICE_ATTR(main_sensor, 0444, main_sensor_info_show, NULL);
static DEVICE_ATTR(cur_sensor, 0444, cur_sensor_info_show, NULL);
static DEVICE_ATTR(dynamic_regs, 0664, dynamic_regs_show, dynamic_regs_set);
static DEVICE_ATTR(reg, 0664, reg_show, reg_set);

int32_t sensor_sysfs_init(char* sensor_name,int position){	
	int32_t ret = 0;
	CDBG("%s E\n",__func__);
	if(sensor_name==NULL)
	{
		CAM_ERR("%s NULL bayer sensor name!\n",__func__);
		return -1;
	}
	if((position!=0)&&(position!=1))
	{
		CAM_ERR("%s Error position!\n",__func__);
		return -1;
	}
	CDBG("%s,name=%s,position=%d\n",__func__,sensor_name,position);
	ret = sensor_name_match(sensor_name,position);
	if(ret!=0){
		CAM_ERR("sensor_name not found!\n");
		return -1;
	}
	if(sensor_node==NULL)
	{
		sensor_node = kobject_create_and_add("camera", NULL);
		if (sensor_node == NULL) {
			CAM_ERR("%s: kobject_create_and_add failed!\n",__func__);
			return -1;
		}
	}
	CDBG("sysfs_create_file\n");

	if(position)
	{
		//front sensor
		ret = sysfs_create_file(sensor_node, &dev_attr_sub_sensor.attr);
		//YUV dynamic regs debug
		ret = sysfs_create_file(sensor_node, &dev_attr_dynamic_regs.attr);
	}
	else
	{
		//rear sensor
		ret = sysfs_create_file(sensor_node, &dev_attr_main_sensor.attr);
		//cur_sensor node
		ret = sysfs_create_file(sensor_node, &dev_attr_cur_sensor.attr);
		//dynamic reg output
		ret = sysfs_create_file(sensor_node, &dev_attr_reg.attr);
	}

	if (ret) 
	{
		CAM_ERR("sysfs_create_file failed!\n");
		//kobject_del(sensor_node);
		//return -1;
	}
	CDBG("%s X\n",__func__);
	return ret;
};

/**************************************YUV Dynamic Regs Debug Func**********************************/
static int32_t tct_i2c_write_table(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_camera_i2c_reg_conf *table,
		int num)
{
	int32_t i = 0;
	int32_t rc = 0;
	
	for (i = 0; i < num; ++i) 
	{
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
			s_ctrl->sensor_i2c_client, table->reg_addr,
			table->reg_data, MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0) 
		{
			CDBG("redo: %s\n",__func__);
			msleep(100);
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
				s_ctrl->sensor_i2c_client, table->reg_addr,
				table->reg_data, MSM_CAMERA_I2C_BYTE_DATA);
			if (rc < 0)
			{
				CAM_ERR("%s failed!",__func__);
				break;
			}
		}
		table++;
	}
	return rc;
}

int32_t camera_dynamic_regs_debug(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct file *file_p = NULL;
	uint32_t file_size = 0;
	mm_segment_t fs = {0};
	loff_t pos = 0;
	uint8_t *data_buf = NULL;
	uint8_t *data_current_p = NULL;
	struct msm_camera_i2c_reg_conf *init_reg = NULL;
	uint32_t reg_num = 0;
	uint16_t reg_buf = 0; //addr, data buf
	uint8_t parse_state = 0; //000:before parse addr, 001:parse addr, 010:before parse data, 100:parse data

	CDBG("%s,%d E\n",__func__,__LINE__);

	fs = get_fs(); 
	set_fs(KERNEL_DS); 
	file_p = filp_open("/data/init_reg.txt", O_RDONLY , 0); //file open
	if (IS_ERR(file_p))
	{ 
		CAM_ERR("%s,open init reg file error\n",__func__); 
		return -1; 
	}
	file_size = vfs_llseek(file_p, 0, SEEK_END);
	if (file_size)
	{
	    data_buf = (uint8_t*)kzalloc(file_size + 1, GFP_KERNEL);
	    if (data_buf)
	    {
		    vfs_read(file_p, data_buf, file_size, &pos);
	    }
	}
	filp_close(file_p, NULL); //file close
	set_fs(fs);

	if (!file_size)
	{
	    CAM_ERR("%s,init file is empty\n",__func__);
	    rc = -1;
	    goto end;
	}
	if (!data_buf)
	{
		CAM_ERR("%s,kzalloc data buf fail\n",__func__);
		rc = -1;
		goto end;
	}
	init_reg = (struct msm_camera_i2c_reg_conf*)kzalloc(1000 * sizeof(struct msm_camera_i2c_reg_conf), GFP_KERNEL);
	if (!init_reg)
	{
		CAM_ERR("%s,kzalloc init reg fail\n",__func__);
		rc = -1;
		goto end;
	}

	//Begin parsing file data
	data_current_p = data_buf;
	while (data_current_p < (data_buf + file_size))
	{
		//skip "//"
		if (*data_current_p == '/' && *(data_current_p + 1) == '/')
		{
			uint8_t *cs_p = strchr(data_current_p, '\n');
			if (cs_p)
			{
				data_current_p = cs_p + 1;
			}
			else
			{
				data_current_p = data_buf + file_size;
			}
			continue;
		}
		//skip "/*", "*/"
		if (*data_current_p == '/' && *(data_current_p + 1) == '*')
		{
			uint8_t *cs_p = strstr(data_current_p, "*/");
			if (cs_p)
			{
				data_current_p = cs_p + 2;
			}
			else
			{
				data_current_p = data_buf + file_size;
			}
			continue;
		}
		//skip "0x", "0X"
		if (*data_current_p == '0' && (*(data_current_p + 1) == 'x' || *(data_current_p + 1) == 'X'))
		{
			data_current_p += 2;
			continue;
		}
		//skip "REG". "reg"
		if ((*data_current_p == 'R' && *(data_current_p + 1) == 'E' && *(data_current_p + 2) == 'G') \
			|| (*data_current_p == 'r' && *(data_current_p + 1) == 'e' && *(data_current_p + 2) == 'g'))
		{
			data_current_p += 3;
			continue;
		}

		if (!(*data_current_p >= '0' && *data_current_p <= '9') && 
			!(*data_current_p >= 'a' && *data_current_p <= 'f') && 
			!(*data_current_p >= 'A' && *data_current_p <= 'F'))
		{
			if (parse_state == 1) //after parse addr
			{
				parse_state = (1 << 1);
				//set reg_addr
				init_reg[reg_num].reg_addr = reg_buf;
			}
			else if (parse_state == (1 << 2)) //after parse data
			{
				parse_state = 0;
				//set reg_data
				init_reg[reg_num].reg_data = reg_buf;
				CDBG("%s,id:%u,addr:0x%x,data:0x%x\n", 
					__func__,
					reg_num, 
					init_reg[reg_num].reg_addr,
					init_reg[reg_num].reg_data);
				reg_num++; //init reg number increase 1
			}
			data_current_p++;
			continue;
		}
		if (parse_state == 0 || parse_state == (1 << 1)) //before parse, set buf 0
		{
			reg_buf = 0;
		}
		if (parse_state == 0 || parse_state == 1) //parse addr
		{
			parse_state = 1;
		} 
		else if (parse_state == (1 << 1) || parse_state == (1 << 2)) //parse data
		{
			parse_state = (1 << 2);
		} 
		else
		{
			CAM_ERR("%s,parse state error:%u\n",__func__,parse_state);
			rc = -1;
			goto end;
		}
		reg_buf *= 16;
		if (*data_current_p >= '0' && *data_current_p <= '9')
		{
			reg_buf += *data_current_p - '0';
		}
		else if (*data_current_p >= 'a' && *data_current_p <= 'f')
		{
			reg_buf += *data_current_p - 'a' + 10;
		}
		else if (*data_current_p >= 'A' && *data_current_p <= 'F')
		{
			reg_buf += *data_current_p - 'A' + 10;
		}
		else
		{
			CAM_ERR("%s,parse reg buf error:%c\n",__func__,*data_current_p);
			rc = -1;
			goto end;
		}
		data_current_p++;
	}
	//End parsing file data

	if (!reg_num)
	{
		CAM_ERR("%s,file init reg number is zero\n",__func__);
		rc = -1;
	}
	else
	{
		//init setting
		rc = tct_i2c_write_table(s_ctrl,init_reg,reg_num);
		CDBG("i2c_write_table,rc=%d",rc); //if rc<0 ,  fail
	}
end:
	if (data_buf)
		kfree(data_buf); //free data buf
	if (init_reg)
		kfree(init_reg); //free init reg buf

	CDBG("%s,%d,rc=%d X\n",__func__,__LINE__,rc);
	return rc;
}
