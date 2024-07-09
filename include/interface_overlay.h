#ifndef INTERFACE_OVERLAY_H
#define INTERFACE_OVERLAY_H

struct hw_config
{
	int valid;

	int fiq_debugger;
#ifdef CONFIG_RK3568_TB3N
	int uart4, uart9;
	int i2c5, i2s3_2ch, spi3, spdif_8ch;
	int pwm12, pwm13, pwm14, pwm15;

	int com1, com2;
#endif

#ifdef CONFIG_RK3566_TB3
	int uart0, uart1, uart4, uart9;
	int i2c1, i2c5, i2s3_2ch, spi2, spi3, spdif_8ch;
	int pwm0, pwm1, pwm2, pwm5, pwm7, pwm8, pwm9, pwm12, pwm13, pwm14, pwm15;
	int xin32k;


	int hdmi, dsi0;
#endif
	int auto_ums;

	int overlay_count;
	char **overlay_file;
};

#ifdef CONFIG_RK3568_TB3N
void set_lan_status(struct fdt_header *);
#endif

void parse_cmdline(void);

void parse_hw_config(struct hw_config *);

struct fdt_header *resize_working_fdt(void);

void handle_hw_conf(cmd_tbl_t *, struct fdt_header *, struct hw_config *);

#endif
