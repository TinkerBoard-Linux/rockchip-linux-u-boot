#include <common.h>
#include <malloc.h>
#include <mapmem.h>
#include <adc.h>
#include <interface_overlay.h>

#define MAX_OVERLAY_NAME_LENGTH	128

#ifdef CONFIG_RK3568_TB3N
#define SARADC_DETECT_NUM	2
static int adc4_odmid = -1, adc5_prjid = -1;
#endif

static char *devtype, *devnum, *file_addr;
static unsigned long fdt_addr_r;

static void verify_devinfo(void)
{
	if (!devtype || !devnum || !file_addr || !fdt_addr_r) {
		devtype = env_get("devtype");
		devnum = env_get("devnum");
		file_addr = env_get("temp_file_addr");
		fdt_addr_r = env_get_ulong("fdt_addr_r", 16, 0);
	}

#ifdef CONFIG_RK3568_TB3N
	if (adc4_odmid == -1 || adc5_prjid == -1) {
		unsigned int in_voltage_raw[SARADC_DETECT_NUM];
		float voltage_scale = 1.8066, voltage_raw[SARADC_DETECT_NUM], vresult[SARADC_DETECT_NUM];
		int ret, adc_channel[SARADC_DETECT_NUM] = {4, 5}, id[SARADC_DETECT_NUM];

		for (int i = 0; i < SARADC_DETECT_NUM; i++) {
			ret = adc_channel_single_shot("saradc", adc_channel[i], &in_voltage_raw[i]);
			if (ret)
				id[i] = -1;
			else {
				voltage_raw[i] = (float)in_voltage_raw[i];
				vresult[i] = voltage_raw[i] * voltage_scale;

				if (vresult[i] < 1950 && vresult[i] > 1650)
					id[i] = 18;
				else if (vresult[i] < 1650 && vresult[i] > 1350)
					id[i] = 15;
				else if (vresult[i] < 1350 && vresult[i] > 1050)
					id[i] = 12;
				else if (vresult[i] < 1050 && vresult[i] > 750)
					id[i] = 9;
				else if (vresult[i] < 750 && vresult[i] > 450)
					id[i] = 6;
				else if (vresult[i] < 450 && vresult[i] > 150)
					id[i] = 3;
				else if (vresult[i] < 150)
					id[i] = 0;
			}
		}

		adc4_odmid = id[0];
		adc5_prjid = id[1];
	}
#endif
}

static unsigned long hw_skip_comment(char *text)
{
	int i = 0;
	if(*text == '#') {
		while(*(text + i) != 0x00)
		{
			if(*(text + (i++)) == 0x0a)
				break;
		}
	}
	return i;
}

static unsigned long hw_skip_line(char *text)
{
	if(*text == 0x0a)
		return 1;
	else
		return 0;
}

static unsigned long get_intf_value(char *text, struct hw_config *hw_conf)
{
	int i = 0;

	if(memcmp(text, "fiq_debugger=", 13) == 0) {
		i = 13;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->fiq_debugger = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->fiq_debugger = -1;
			i = i + 3;
		} else
			goto invalid_line;
#ifdef CONFIG_RK3568_TB3N
	} else if(memcmp(text, "uart4=", 6) == 0) {
		i = 6;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->uart4 = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->uart4 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "i2c5=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->i2c5 = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->i2c5 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "uart9=", 6) == 0) {
		i = 6;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->uart9 = 1;
			i = i + 2;
			hw_conf->pwm12 = -1;
			hw_conf->pwm13 = -1;
			hw_conf->spi3 = -1;
			hw_conf->i2s3_2ch = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->uart9 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm12=", 6) == 0) {
		i = 6;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm12 = 1;
			i = i + 2;
			hw_conf->uart9 = -1;
			hw_conf->spi3 = -1;
			hw_conf->i2s3_2ch = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm12 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm13=", 6) == 0) {
		i = 6;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm13 = 1;
			i = i + 2;
			hw_conf->uart9 = -1;
			hw_conf->spi3 = -1;
			hw_conf->i2s3_2ch = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm13 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm14=", 6) == 0) {
		i = 6;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm14 = 1;
			i = i + 2;
			hw_conf->spi3 = -1;
			hw_conf->i2s3_2ch = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm14 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm15=", 6) == 0) {
		i = 6;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm15 = 1;
			i = i + 2;
			hw_conf->spi3 = -1;
			hw_conf->i2s3_2ch = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm15 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "spdif_8ch=", 10) == 0) {
		i = 10;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->spdif_8ch = 1;
			i = i + 2;
			hw_conf->spi3 = -1;
			hw_conf->i2s3_2ch = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->spdif_8ch = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "spi3=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->spi3 = 1;
			i = i + 2;
			hw_conf->uart9 = -1;
			hw_conf->pwm12 = -1;
			hw_conf->pwm13 = -1;
			hw_conf->pwm14 = -1;
			hw_conf->pwm15 = -1;
			hw_conf->spdif_8ch = -1;
			hw_conf->i2s3_2ch = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->spi3 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "i2s3_2ch=", 9) == 0) {
		i = 9;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->i2s3_2ch = 1;
			i = i + 2;
			hw_conf->uart9 = -1;
			hw_conf->pwm12 = -1;
			hw_conf->pwm13 = -1;
			hw_conf->pwm14 = -1;
			hw_conf->pwm15 = -1;
			hw_conf->spdif_8ch = -1;
			hw_conf->spi3 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->i2s3_2ch = -1;
			i = i + 3;
		} else
			goto invalid_line;
#endif
#ifdef CONFIG_RK3566_TB3
	} else if(memcmp(text, "uart0=", 6) == 0) {
		i = 6;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->uart0 = 1;
			i = i + 2;
			hw_conf->pwm0 = -1;
			hw_conf->pwm1 = -1;
			hw_conf->pwm2 = -1;
			hw_conf->pwm5 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->uart0 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "uart1=", 6) == 0) {
		i = 6;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->uart1 = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->uart1 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "uart4=", 6) == 0) {
		i = 6;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->uart4 = 1;
			i = i + 2;
			hw_conf->pwm8 = -1;
			hw_conf->pwm9 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->uart4 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "i2c1=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->i2c1 = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->i2c1 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "i2c5=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->i2c5 = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->i2c5 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "i2s3_2ch=", 9) == 0) {
		i = 9;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->i2s3_2ch = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->i2s3_2ch = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "spi2=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->spi2 = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->spi2 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "spi3=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->spi3 = 1;
			i = i + 2;
			hw_conf->pwm12 = -1;
			hw_conf->pwm13 = -1;
			hw_conf->pwm14 = -1;
			hw_conf->pwm15 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->spi3 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "spdif_8ch=", 10) == 0) {
		i = 10;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->spdif_8ch = 1;
			i = i + 2;
			hw_conf->pwm15 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->spdif_8ch = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm0=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm0 = 1;
			i = i + 2;
			hw_conf->uart0 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm0 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm1=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm1 = 1;
			i = i + 2;
			hw_conf->uart0 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm1 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm2=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm2 = 1;
			i = i + 2;
			hw_conf->uart0 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm2 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm5=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm5 = 1;
			i = i + 2;
			hw_conf->uart0 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm5 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm7=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm7 = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm7 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm8=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm8 = 1;
			i = i + 2;
			hw_conf->uart4 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm8 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm9=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm9 = 1;
			i = i + 2;
			hw_conf->uart4 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm9 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm12=", 6) == 0) {
		i = 6;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm12 = 1;
			i = i + 2;
			hw_conf->spi3 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm12 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm13=", 6) == 0) {
		i = 6;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm13 = 1;
			i = i + 2;
			hw_conf->spi3 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm13 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm14=", 6) == 0) {
		i = 6;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm14 = 1;
			i = i + 2;
			hw_conf->spi3 = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm14 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "pwm15=", 6) == 0) {
		i = 6;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->pwm15 = 1;
			i = i + 2;
			hw_conf->spi3 = -1;
			hw_conf->spdif_8ch = -1;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->pwm15 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if(memcmp(text, "xin32k=", 7) == 0) {
		i = 7;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->xin32k = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->xin32k = -1;
			i = i + 3;
		} else
			goto invalid_line;
#endif
	} else
		goto invalid_line;

	while(*(text + i) != 0x00)
	{
		if(*(text + (i++)) == 0x0a)
			break;
	}
	return i;

invalid_line:
	//It's not a legal line, skip it.
	//printf("get_value: illegal line\n");
	while(*(text + i) != 0x00)
	{
		if(*(text + (i++)) == 0x0a)
			break;
	}
	return i;
}

static unsigned long get_conf_value(char *text, struct hw_config *hw_conf)
{
	int i = 0;
	if (memcmp(text, "auto_ums=", 9) == 0) {
		i = 9;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->auto_ums = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->auto_ums = -1;
			i = i + 3;
		} else
			goto invalid_line;
#ifdef CONFIG_RK3568_TB3N
	} else if (memcmp(text, "COM1=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->com1 = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->com1 = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if (memcmp(text, "COM2=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->com2 = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->com2 = -1;
			i = i + 3;
		} else
			goto invalid_line;
#endif
#ifdef CONFIG_RK3566_TB3
	} else if (memcmp(text, "HDMI=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->hdmi = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			if (hw_conf->dsi0 != -1)
				hw_conf->hdmi = -1;
			i = i + 3;
		} else
			goto invalid_line;
	} else if (memcmp(text, "DSI0=", 5) == 0) {
		i = 5;
		if(memcmp(text + i, "on", 2) == 0) {
			hw_conf->dsi0 = 1;
			i = i + 2;
		} else if(memcmp(text + i, "off", 3) == 0) {
			hw_conf->hdmi = 1;
			hw_conf->dsi0 = -1;
			i = i + 3;
		} else
			goto invalid_line;
#endif
	} else
		goto invalid_line;

	while(*(text + i) != 0x00)
	{
		if(*(text + (i++)) == 0x0a)
			break;
	}
	return i;

invalid_line:
	//It's not a legal line, skip it.
	//printf("get_value: illegal line\n");
	while(*(text + i) != 0x00)
	{
		if(*(text + (i++)) == 0x0a)
			break;
	}
	return i;
}

static int set_hw_property(struct fdt_header *working_fdt, char *path, char *property, char *value, int length)
{
	int offset;
	int ret;

	printf("set_hw_property: %s %s %s\n", path, property, value);
	offset = fdt_path_offset (working_fdt, path);
	if (offset < 0) {
		printf("libfdt fdt_path_offset() returned %s\n", fdt_strerror(offset));
		return -1;
	}
	ret = fdt_setprop(working_fdt, offset, property, value, length);
	if (ret < 0) {
		printf("libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
		return -1;
	}

	return 0;
}

static unsigned long get_append(char *text, int *rootset)
{
	int i = 0;
	int append_len = 0;

	while(*(text + i) != 0x00)
	{
		if (memcmp(text, "root=", 5) == 0)
			*rootset = 1;

		if(*(text + i) == 0x20 || *(text + i) == 0x0a) {
			append_len = i;
			i++;
			break;
		} else
			i++;
	}

	if (append_len) {
		char *append = (char*)calloc(append_len, sizeof(char));

		memcpy(append, text, append_len);
		printf("get append cmdline: %s\n", append);
		env_update("bootargs", append);
		free(append);
	}

	return i;
}

static int set_file_conf(char *text, struct hw_config *hw_conf, int start_point, int file_ptr)
{
	char *ptr;
	int name_length;

	name_length = file_ptr - start_point;

	if(name_length && name_length < MAX_OVERLAY_NAME_LENGTH) {
		ptr = (char*)calloc(MAX_OVERLAY_NAME_LENGTH, sizeof(char));
		memcpy(ptr, text + start_point, name_length);
		ptr[name_length] = 0x00;
		hw_conf->overlay_file[hw_conf->overlay_count] = ptr;
		hw_conf->overlay_count += 1;
	}
	//Pass a space for next string.
	start_point = file_ptr + 1;

	return start_point;
}

void get_overlay_count(char *text, struct hw_config *hw_conf)
{
	int i = 0;
	int start_point = 0;
	int overlay_count = 0;
	int name_length;

	while(*(text + i) != 0x00)
	{
		if(*(text + i) == 0x20 || *(text + i) == 0x0a) {
			name_length = i - start_point;
			if(name_length && name_length < MAX_OVERLAY_NAME_LENGTH)
				overlay_count += 1;
		}

		if(*(text + i) == 0x20)
			start_point = i + 1;
		else if(*(text + i) == 0x0a)
			break;
		i++;
	}

	hw_conf->overlay_file = (char**)calloc(overlay_count, sizeof(char*));
}

static unsigned long get_overlay(char *text, struct hw_config *hw_conf)
{
	int i = 0;
	int start_point = 0;

	hw_conf->overlay_count = 0;
	while(*(text + i) != 0x00)
	{
		if(*(text + i) == 0x20 || *(text + i) == 0x0a)
			start_point = set_file_conf(text, hw_conf, start_point, i);

		if(*(text + i) == 0x0a) {
			i++;
			break;
		} else
			i++;
	}

	return i;
}

static unsigned long hw_parse_property(char *text, struct hw_config *hw_conf)
{
	int i = 0;
	if(memcmp(text, "intf:", 5) == 0) {
		i = 5;
		i = i + get_intf_value(text + i, hw_conf);
	} else if (memcmp(text, "conf:",  5) == 0) {
		i = 5;
		i = i + get_conf_value(text + i, hw_conf);
	} else if(memcmp(text, "overlay=", 8) == 0) {
		i = 8;
		get_overlay_count(text + i, hw_conf);
		i = i + get_overlay(text + i, hw_conf);
	} else {
		printf("[conf] hw_parse_property: illegal line\n");
		//It's not a legal line, skip it.
		while(*(text + i) != 0x00) {
			if(*(text + (i++)) == 0x0a)
				break;
		}
	}
	return i;
}

#ifdef CONFIG_RK3568_TB3N
void set_lan_status(struct fdt_header *working_fdt)
{
	verify_devinfo();

	if ((adc4_odmid == 15 && adc5_prjid == 18) || (adc4_odmid == 18 && adc5_prjid == 12)) {
		printf("Detect the SKU without LAN1\n");
		set_hw_property(working_fdt, "/ethernet@fe010000", "status", "disabled", 9);
	}
}
#endif

void parse_cmdline(void)
{
	unsigned long count, offset = 0, addr, size;
	static char *fs_argv[5];

	int valid = 0, rootset = 0;

	verify_devinfo();

	addr = simple_strtoul(file_addr, NULL, 16);
	if (!addr)
		printf("Can't set addr\n");

	fs_argv[0] = "ext2load";
	fs_argv[1] = "mmc";

	if (!strcmp(devnum, "0"))
		fs_argv[2] = "0:7";
	else if (!strcmp(devnum, "1"))
		fs_argv[2] = "1:7";
	else {
		printf("Invalid devnum\n");
		goto end;
	}

	fs_argv[3] = file_addr;
	fs_argv[4] = "cmdline.txt";

	if (do_ext2load(NULL, 0, 5, fs_argv)) {
		printf("[cmdline] do_ext2load fail\n");
		goto end;
	}

	size = env_get_ulong("filesize", 16, 0);
	if (!size) {
		printf("[cmdline] Can't get filesize\n");
		goto end;
	}

	valid = 1;

	printf("cmdline.txt size = %lu\n", size);

	*((char *)file_addr + size) = 0x00;

	while(offset != size)
	{
		count = hw_skip_comment((char *)(addr + offset));
		if(count > 0) {
			offset = offset + count;
			continue;
		}
		count = hw_skip_line((char *)(addr + offset));
		if(count > 0) {
			offset = offset + count;
			continue;
		}
		count = get_append((char *)(addr + offset), &rootset);
		if(count > 0) {
			offset = offset + count;
			continue;
		} else {
			valid = -1;
			break;
		}
	}

end:
	printf("cmdline.txt valid = %d\n", valid);

	if (rootset == 0) {
		if (!strcmp(devnum, "0"))
			env_update("bootargs", "root=/dev/mmcblk0p8");	/* eMMC Boot */
		else if (!strcmp(devnum, "1"))
			env_update("bootargs", "root=/dev/mmcblk1p8");	/* SDcard Boot */
	}
}

void parse_hw_config(struct hw_config *hw_conf)
{
	unsigned long count, offset = 0, addr, size;
	static char *fs_argv[5];

	int valid = 0;

	char *tdevnum = env_get("devnum");
	char *tfile_addr = env_get("temp_file_addr");

	addr = simple_strtoul(tfile_addr, NULL, 16);
	if (!addr)
		printf("Can't set addr\n");

	fs_argv[0] = "ext2load";
	fs_argv[1] = "mmc";

	if (!strcmp(tdevnum, "0"))
		fs_argv[2] = "0:7";
	else if (!strcmp(tdevnum, "1"))
		fs_argv[2] = "1:7";
	else {
		printf("Invalid devnum\n");
		goto end;
	}

	fs_argv[3] = tfile_addr;
	fs_argv[4] = "config.txt";

	if (do_ext2load(NULL, 0, 5, fs_argv)) {
		printf("[conf] do_ext2load fail\n");
		goto end;
	}

	size = env_get_ulong("filesize", 16, 0);
	if (!size) {
		printf("[conf] Can't get filesize\n");
		goto end;
	}

	valid = 1;
	printf("config.txt size = %lu\n", size);

	*((char *)tfile_addr + size) = 0x00;

	while(offset != size)
	{
		count = hw_skip_comment((char *)(addr + offset));
		if(count > 0) {
			offset = offset + count;
			continue;
		}
		count = hw_skip_line((char *)(addr + offset));
		if(count > 0) {
			offset = offset + count;
			continue;
		}
		count = hw_parse_property((char *)(addr + offset), hw_conf);
		if(count > 0) {
			offset = offset + count;
			continue;
		} else {
			valid = -1;
			break;
		}
	}
end:
	hw_conf->valid = valid;
}

struct fdt_header *resize_working_fdt(void)
{
	struct fdt_header *working_fdt;
	int err;

	verify_devinfo();

	working_fdt = map_sysmem(fdt_addr_r, 0);
	err = fdt_open_into(working_fdt, working_fdt, (1024 * 1024));
	if (err != 0) {
		printf("libfdt fdt_open_into(): %s\n", fdt_strerror(err));
		return NULL;
	}

	printf("fdt magic number %x\n", working_fdt->magic);
	printf("fdt size %u\n", fdt_totalsize(working_fdt));

	return working_fdt;
}

#ifdef CONFIG_OF_LIBFDT_OVERLAY
static int fdt_valid(struct fdt_header **blobp)
{
	const void *blob = *blobp;
	int err;

	if (blob == NULL) {
		printf ("The address of the fdt is invalid (NULL).\n");
		return 0;
	}

	err = fdt_check_header(blob);
	if (err == 0)
		return 1;	/* valid */

	if (err < 0) {
		printf("libfdt fdt_check_header(): %s", fdt_strerror(err));
		/*
		 * Be more informative on bad version.
		 */
		if (err == -FDT_ERR_BADVERSION) {
			if (fdt_version(blob) < FDT_FIRST_SUPPORTED_VERSION) {
				printf (" - too old, fdt %d < %d", fdt_version(blob), FDT_FIRST_SUPPORTED_VERSION);
			}
			if (fdt_last_comp_version(blob) > FDT_LAST_SUPPORTED_VERSION) {
				printf (" - too new, fdt %d > %d", fdt_version(blob), FDT_LAST_SUPPORTED_VERSION);
			}
		}
		printf("\n");
		*blobp = NULL;
		return 0;
	}
	return 1;
}

static int merge_dts_overlay(cmd_tbl_t *cmdtp, struct fdt_header *working_fdt, char *overlay_name)
{
	unsigned long addr;
	struct fdt_header *blob;
	int ret;
	char overlay_file[MAX_OVERLAY_NAME_LENGTH] = "overlays/";

	static char *fs_argv[5];

	verify_devinfo();

	addr = simple_strtoul(file_addr, NULL, 16);
	if (!addr)
		printf("Can't set addr\n");

	strcat(overlay_file, overlay_name);
	strncat(overlay_file, ".dtbo", 6);

	fs_argv[0] = "ext2load";
	fs_argv[1] = "mmc";

	if (!strcmp(devnum, "0"))
		fs_argv[2] = "0:7";
	else if (!strcmp(devnum, "1"))
		fs_argv[2] = "1:7";
	else {
		printf("Invalid devnum\n");
		goto fail;
	}

	fs_argv[3] = file_addr;
	fs_argv[4] = overlay_file;

	if (do_ext2load(NULL, 0, 5, fs_argv)) {
		printf("[merge_dts_overlay] do_ext2load fail\n");
		goto fail;
	}

	blob = map_sysmem(addr, 0);
	if (!fdt_valid(&blob)) {
		printf("[merge_dts_overlay] fdt_valid is invalid\n");
		goto fail;
	} else
		printf("fdt_valid\n");

	ret = fdt_overlay_apply(working_fdt, blob);
	if (ret) {
		printf("[merge_dts_overlay] fdt_overlay_apply(): %s\n", fdt_strerror(ret));
		goto fail;
	}

	return 0;

fail:
	return -1;
}
#endif

void handle_hw_conf(cmd_tbl_t *cmdtp, struct fdt_header *working_fdt, struct hw_config *hw_conf)
{
	if(working_fdt == NULL)
		return;

#ifdef CONFIG_OF_LIBFDT_OVERLAY
	int i;
	for (i = 0; i < hw_conf->overlay_count; i++) {
		if (merge_dts_overlay(cmdtp, working_fdt, hw_conf->overlay_file[i]) < 0)
			printf("Can't merge dts overlay: %s\n", hw_conf->overlay_file[i]);
		else
			printf("Merged dts overlay: %s\n", hw_conf->overlay_file[i]);

		free(hw_conf->overlay_file[i]);
	}
	free(hw_conf->overlay_file);
#endif

#ifdef CONFIG_RK3568_TB3N
	if (hw_conf->uart4 == 1)
		set_hw_property(working_fdt, "/serial@fe680000", "status", "okay", 5);
	else if (hw_conf->uart4 == -1)
		set_hw_property(working_fdt, "/serial@fe680000", "status", "disabled", 9);

	if (hw_conf->i2c5 == 1)
		set_hw_property(working_fdt, "/i2c@fe5e0000", "status", "okay", 5);
	else if (hw_conf->i2c5 == -1)
		set_hw_property(working_fdt, "/i2c@fe5e0000", "status", "disabled", 9);

	if (hw_conf->uart9 == 1)
		set_hw_property(working_fdt, "/serial@fe6d0000", "status", "okay", 5);
	else if (hw_conf->uart9 == -1)
		set_hw_property(working_fdt, "/serial@fe6d0000", "status", "disabled", 9);

	if (hw_conf->pwm12 == 1)
		set_hw_property(working_fdt, "/pwm@fe700000", "status", "okay", 5);
	else if (hw_conf->pwm12 == -1)
		set_hw_property(working_fdt, "/pwm@fe700000", "status", "disabled", 9);

	if (hw_conf->pwm13 == 1)
		set_hw_property(working_fdt, "/pwm@fe700010", "status", "okay", 5);
	else if (hw_conf->pwm13 == -1)
		set_hw_property(working_fdt, "/pwm@fe700010", "status", "disabled", 9);

	if (hw_conf->pwm14 == 1)
		set_hw_property(working_fdt, "/pwm@fe700020", "status", "okay", 5);
	else if (hw_conf->pwm14 == -1)
		set_hw_property(working_fdt, "/pwm@fe700020", "status", "disabled", 9);

	if (hw_conf->pwm15 == 1)
		set_hw_property(working_fdt, "/pwm@fe700030", "status", "okay", 5);
	else if (hw_conf->pwm15 == -1)
		set_hw_property(working_fdt, "/pwm@fe700030", "status", "disabled", 9);

	if (hw_conf->spdif_8ch == 1)
		set_hw_property(working_fdt, "/spdif@fe460000", "status", "okay", 5);
	else if (hw_conf->spdif_8ch == -1)
		set_hw_property(working_fdt, "/spdif@fe460000", "status", "disabled", 9);

	if (hw_conf->spi3 == 1)
		set_hw_property(working_fdt, "/spi@fe640000", "status", "okay", 5);
	else if (hw_conf->spi3 == -1)
		set_hw_property(working_fdt, "/spi@fe640000", "status", "disabled", 9);

	if (hw_conf->i2s3_2ch == 1)
		set_hw_property(working_fdt, "/i2s@fe430000", "status", "okay", 5);
	else if (hw_conf->i2s3_2ch == -1)
		set_hw_property(working_fdt, "/i2s@fe430000", "status", "disabled", 9);

	if (hw_conf->fiq_debugger == 1)
		set_hw_property(working_fdt, "/fiq-debugger", "status", "okay", 5);
	else if (hw_conf->fiq_debugger == -1)
		set_hw_property(working_fdt, "/fiq-debugger", "status", "disabled", 9);

	if (hw_conf->com1 == 1)
		set_hw_property(working_fdt, "/uart0_enable", "status", "okay", 5);
	else if (hw_conf->com1 == -1)
		set_hw_property(working_fdt, "/uart0_enable", "status", "disabled", 9);

	if (hw_conf->com2 == 1)
		set_hw_property(working_fdt, "/uart8_enable", "status", "okay", 5);
	else if (hw_conf->com2 == -1)
		set_hw_property(working_fdt, "/uart8_enable", "status", "disabled", 9);
#endif

#ifdef CONFIG_RK3566_TB3
	if (hw_conf->uart0 == 1)
		set_hw_property(working_fdt, "/serial@fdd50000", "status", "okay", 5);
	else if (hw_conf->uart0 == -1)
		set_hw_property(working_fdt, "/serial@fdd50000", "status", "disabled", 9);

	if (hw_conf->uart1 == 1)
		set_hw_property(working_fdt, "/serial@fe650000", "status", "okay", 5);
	else if (hw_conf->uart1 == -1)
		set_hw_property(working_fdt, "/serial@fe650000", "status", "disabled", 9);

	if (hw_conf->uart4 == 1)
		set_hw_property(working_fdt, "/serial@fe680000", "status", "okay", 5);
	else if (hw_conf->uart4 == -1)
		set_hw_property(working_fdt, "/serial@fe680000", "status", "disabled", 9);

	if (hw_conf->i2c1 == 1)
		set_hw_property(working_fdt, "/i2c@fe5a0000", "status", "okay", 5);
	else if (hw_conf->i2c1 == -1)
		set_hw_property(working_fdt, "/i2c@fe5a0000", "status", "disabled", 9);

	if (hw_conf->i2c5 == 1)
		set_hw_property(working_fdt, "/i2c@fe5e0000", "status", "okay", 5);
	else if (hw_conf->i2c5 == -1)
		set_hw_property(working_fdt, "/i2c@fe5e0000", "status", "disabled", 9);

	if (hw_conf->i2s3_2ch == 1)
		set_hw_property(working_fdt, "/i2s@fe430000", "status", "okay", 5);
	else if (hw_conf->i2s3_2ch == -1)
		set_hw_property(working_fdt, "/i2s@fe430000", "status", "disabled", 9);

	if (hw_conf->spi2 == 1)
		set_hw_property(working_fdt, "/spi@fe630000", "status", "okay", 5);
	else if (hw_conf->spi2 == -1)
		set_hw_property(working_fdt, "/spi@fe630000", "status", "disabled", 9);

	if (hw_conf->spi3 == 1)
		set_hw_property(working_fdt, "/spi@fe640000", "status", "okay", 5);
	else if (hw_conf->spi3 == -1)
		set_hw_property(working_fdt, "/spi@fe640000", "status", "disabled", 9);

	if (hw_conf->spdif_8ch == 1)
		set_hw_property(working_fdt, "/spdif@fe460000", "status", "okay", 5);
	else if (hw_conf->spdif_8ch == -1)
		set_hw_property(working_fdt, "/spdif@fe460000", "status", "disabled", 9);

	if (hw_conf->pwm0 == 1)
		set_hw_property(working_fdt, "/pwm@fdd70000", "status", "okay", 5);
	else if (hw_conf->pwm0 == -1)
		set_hw_property(working_fdt, "/pwm@fdd70000", "status", "disabled", 9);

	if (hw_conf->pwm1 == 1)
		set_hw_property(working_fdt, "/pwm@fdd70010", "status", "okay", 5);
	else if (hw_conf->pwm1 == -1)
		set_hw_property(working_fdt, "/pwm@fdd70010", "status", "disabled", 9);

	if (hw_conf->pwm2 == 1)
		set_hw_property(working_fdt, "/pwm@fdd70020", "status", "okay", 5);
	else if (hw_conf->pwm2 == -1)
		set_hw_property(working_fdt, "/pwm@fdd70020", "status", "disabled", 9);

	if (hw_conf->pwm5 == 1)
		set_hw_property(working_fdt, "/pwm@fe6e0010", "status", "okay", 5);
	else if (hw_conf->pwm5 == -1)
		set_hw_property(working_fdt, "/pwm@fe6e0010", "status", "disabled", 9);

	if (hw_conf->pwm7 == 1)
		set_hw_property(working_fdt, "/pwm@fe6e0030", "status", "okay", 5);
	else if (hw_conf->pwm7 == -1)
		set_hw_property(working_fdt, "/pwm@fe6e0030", "status", "disabled", 9);

	if (hw_conf->pwm8 == 1)
		set_hw_property(working_fdt, "/pwm@fe6f0000", "status", "okay", 5);
	else if (hw_conf->pwm8 == -1)
		set_hw_property(working_fdt, "/pwm@fe6f0000", "status", "disabled", 9);

	if (hw_conf->pwm9 == 1)
		set_hw_property(working_fdt, "/pwm@fe6f0010", "status", "okay", 5);
	else if (hw_conf->pwm9 == -1)
		set_hw_property(working_fdt, "/pwm@fe6f0010", "status", "disabled", 9);

	if (hw_conf->pwm12 == 1)
		set_hw_property(working_fdt, "/pwm@fe700000", "status", "okay", 5);
	else if (hw_conf->pwm12 == -1)
		set_hw_property(working_fdt, "/pwm@fe700000", "status", "disabled", 9);

	if (hw_conf->pwm13 == 1)
		set_hw_property(working_fdt, "/pwm@fe700010", "status", "okay", 5);
	else if (hw_conf->pwm13 == -1)
		set_hw_property(working_fdt, "/pwm@fe700010", "status", "disabled", 9);

	if (hw_conf->pwm14 == 1)
		set_hw_property(working_fdt, "/pwm@fe700020", "status", "okay", 5);
	else if (hw_conf->pwm14 == -1)
		set_hw_property(working_fdt, "/pwm@fe700020", "status", "disabled", 9);

	if (hw_conf->pwm15 == 1)
		set_hw_property(working_fdt, "/pwm@fe700030", "status", "okay", 5);
	else if (hw_conf->pwm15 == -1)
		set_hw_property(working_fdt, "/pwm@fe700030", "status", "disabled", 9);

	if (hw_conf->xin32k == 1)
		set_hw_property(working_fdt, "/xin32k", "status", "okay", 5);
	else if (hw_conf->xin32k == -1)
		set_hw_property(working_fdt, "/xin32k", "status", "disabled", 9);

	if (hw_conf->hdmi == 1) {
		set_hw_property(working_fdt, "/hdmi@fe0a0000", "status", "okay", 5);
		set_hw_property(working_fdt, "/hdmi@fe0a0000/ports/port@0/endpoint@0", "status", "okay", 5);
	} else if (hw_conf->hdmi == -1) {
		set_hw_property(working_fdt, "/hdmi@fe0a0000", "status", "disabled", 9);
		set_hw_property(working_fdt, "/hdmi@fe0a0000/ports/port@0/endpoint@0", "status", "disabled", 9);
	}

	if (hw_conf->dsi0 == 1) {
		set_hw_property(working_fdt, "/dsi@fe060000", "status", "okay", 5);
		set_hw_property(working_fdt, "/dsi@fe060000/ports/port@0/endpoint@1", "status", "okay", 5);
	} else if (hw_conf->dsi0 == -1) {
		set_hw_property(working_fdt, "/dsi@fe060000", "status", "disabled", 9);
		set_hw_property(working_fdt, "/dsi@fe060000/ports/port@0/endpoint@1", "status", "disabled", 9);
	}
#endif
}
