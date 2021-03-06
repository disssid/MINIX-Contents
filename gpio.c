/*
 * GPIO driver. This driver acts as a file system to allow
 * reading and toggling of GPIO's.
 */
/* kernel headers */
#include <minix/driver.h>
#include <minix/drvlib.h>
#include <minix/vtreefs.h>
#include <minix/syslib.h>
#include <minix/log.h>
#include <minix/mmio.h>
#include <minix/gpio.h>
#include <minix/padconf.h>
#include <minix/type.h>
#include <minix/board.h>

/* system headers */
#include <sys/stat.h>
#include <sys/queue.h>
#include <sys/queue.h>

/* usr headers */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

/* local headers */


#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <lib.h>

/* used for logging */
static struct log log = {
	.name = "gpio",
	.log_level = LEVEL_INFO,
	.log_func = default_log
};

#define GPIO_UPDATE 1
#define GPIO_CB_READ 0
#define GPIO_CB_INTR_READ 1
#define GPIO_CB_ON 2
#define GPIO_CB_OFF 3

/* The vtreefs library provides callback data when calling
 * the read function of inode. gpio_cbdata is used here to
 * map between inodes and gpio's. VTreeFS is read-only. to work
 * around that issue for a single GPIO we create multiple virtual
 * files that can be *read* to read the gpio value and power on
 * and off the gpio.
 */
struct gpio_cbdata
{
	struct gpio *gpio;	/* obtained from the driver */
	int type;		/* read=0/on=1/off=2 */
	TAILQ_ENTRY(gpio_cbdata) next;
};

/* list of inodes used in this driver */
/* *INDENT-OFF* */
TAILQ_HEAD(gpio_cbdata_head, gpio_cbdata)
    gpio_cbdata_list = TAILQ_HEAD_INITIALIZER(gpio_cbdata_list);
/* *INDENT-ON* */

/* Sane file stats for a directory */
static struct inode_stat default_file_stat = {
	.mode = S_IFREG | 04,
	.uid = 0,
	.gid = 0,
	.size = 0,
	.dev = NO_DEV,
};

/* Buffer size for read requests */
#define DATA_SIZE	26

int
add_gpio_inode(char *name, int nr, int mode)
{
	/* Create 2 files nodes for "name" "nameon" and "nameoff" to read and
	 * set values as we don't support writing yet */
	char tmpname[200];
	struct gpio_cbdata *cb;
	struct gpio *gpio;

	/* claim and configure the gpio */
	if (gpio_claim("gpiofs", nr, &gpio)) {
		log_warn(&log, "Failed to claim GPIO %d\n", nr);
		return EIO;
	}
	assert(gpio != NULL);

	if (gpio_pin_mode(gpio, mode)) {
		log_warn(&log, "Failed to switch GPIO %d to mode %d\n", nr,
		    mode);
		return EIO;
	}

	/* read value */
	cb = malloc(sizeof(struct gpio_cbdata));
	if (cb == NULL) {
		return ENOMEM;
	}
	memset(cb, 0, sizeof(*cb));

	cb->type = GPIO_CB_READ;
	cb->gpio = gpio;

	snprintf(tmpname, 200, "%s", name);
	add_inode(get_root_inode(), tmpname, NO_INDEX, &default_file_stat, 0,
	    (cbdata_t) cb);
	TAILQ_INSERT_HEAD(&gpio_cbdata_list, cb, next);

	if (mode == GPIO_MODE_OUTPUT) {
		/* if we configured the GPIO pin as output mode also create
		 * two additional files to turn on and off the GPIO. */
		/* turn on */
		cb = malloc(sizeof(struct gpio_cbdata));
		if (cb == NULL) {
			return ENOMEM;
		}
		memset(cb, 0, sizeof(*cb));

		cb->type = GPIO_CB_ON;
		cb->gpio = gpio;

		snprintf(tmpname, 200, "%sOn", name);
		add_inode(get_root_inode(), tmpname, NO_INDEX,
		    &default_file_stat, 0, (cbdata_t) cb);
		TAILQ_INSERT_HEAD(&gpio_cbdata_list, cb, next);

		/* turn off */
		cb = malloc(sizeof(struct gpio_cbdata));
		if (cb == NULL) {
			return ENOMEM;
		}
		memset(cb, 0, sizeof(*cb));

		cb->type = GPIO_CB_OFF;
		cb->gpio = gpio;

		snprintf(tmpname, 200, "%sOff", name);
		add_inode(get_root_inode(), tmpname, NO_INDEX,
		    &default_file_stat, 0, (cbdata_t) cb);
		TAILQ_INSERT_HEAD(&gpio_cbdata_list, cb, next);
	} else {
		/* read interrupt */
		cb = malloc(sizeof(struct gpio_cbdata));
		if (cb == NULL) {
			return ENOMEM;
		}
		memset(cb, 0, sizeof(*cb));

		cb->type = GPIO_CB_INTR_READ;
		cb->gpio = gpio;

		snprintf(tmpname, 200, "%sIntr", name);
		add_inode(get_root_inode(), tmpname, NO_INDEX,
		    &default_file_stat, 0, (cbdata_t) cb);
		TAILQ_INSERT_HEAD(&gpio_cbdata_list, cb, next);
	}
	return OK;
}

static void
init_hook(void)
{
	/* This hook will be called once, after VTreeFS has initialized. */
	if (gpio_init()) {
		log_warn(&log, "Failed to init gpio driver\n");
	}

	struct machine  machine ;
	sys_getmachine(&machine);

	if (BOARD_IS_BBXM(machine.board_id)){
		add_gpio_inode("USR0", 149, GPIO_MODE_OUTPUT);
		add_gpio_inode("USR1", 150, GPIO_MODE_OUTPUT);
		add_gpio_inode("Button", 4, GPIO_MODE_INPUT);

		/* configure GPIO_144 to be exported */
		sys_padconf(CONTROL_PADCONF_UART2_CTS, 0x0000ffff,
		    PADCONF_MUXMODE(4) | PADCONF_PULL_MODE_PD_EN |
		    PADCONF_INPUT_ENABLE(1));
		sys_padconf(CONTROL_PADCONF_MMC2_DAT6, 0xffff0000,
		    (PADCONF_MUXMODE(4) | PADCONF_PULL_MODE_PD_EN |
			PADCONF_INPUT_ENABLE(1)) << 16);

		/* Added for demo purposes */
		add_gpio_inode("BigRedButton", 144, GPIO_MODE_INPUT);
		add_gpio_inode("BigRedButtonLed", 139, GPIO_MODE_OUTPUT);
	} else if ( BOARD_IS_BB(machine.board_id)){

		/* Export GPIO3_19 (P9-27 on BBB) output as GPIO1 */

		sys_padconf(CONTROL_CONF_MCASP0_FSR, 0xffffffff,
		    (CONTROL_CONF_PUTYPESEL | CONTROL_CONF_MUXMODE(7)));

		add_gpio_inode("GPIO1", (32 * 3) + 19, GPIO_MODE_OUTPUT);
		
		/* Export GPIO3_16 (P9-30 on BBB) output as GPIO2 */

		sys_padconf(CONTROL_CONF_MCASP0_FSR, 0xffffffff,
		    (CONTROL_CONF_PUTYPESEL | CONTROL_CONF_MUXMODE(7)));

		add_gpio_inode("GPIO2", (32 * 3) + 16, GPIO_MODE_OUTPUT);

		/* Export GPIO3_17 (P9-28 on BBB) output as GPIO3 */

		sys_padconf(CONTROL_CONF_MCASP0_FSR, 0xffffffff,
		    (CONTROL_CONF_PUTYPESEL | CONTROL_CONF_MUXMODE(7)));

		add_gpio_inode("GPIO3", (32 * 3) + 17, GPIO_MODE_OUTPUT);

		/* Export GPIO0_14 (P9-26 on BBB) output as GPIO4 */

		sys_padconf(CONTROL_CONF_MCASP0_FSR, 0xffffffff,
		    (CONTROL_CONF_PUTYPESEL | CONTROL_CONF_MUXMODE(7)));

		add_gpio_inode("GPIO4", (32 * 0) + 14, GPIO_MODE_OUTPUT);

		
		/* Export GPIO1_6 (P8-3 on BBB) output as GPIO5 */

		sys_padconf(CONTROL_CONF_MCASP0_FSR, 0xffffffff,
		    (CONTROL_CONF_PUTYPESEL | CONTROL_CONF_MUXMODE(7)));

		add_gpio_inode("GPIO5", (32 * 1) + 6, GPIO_MODE_OUTPUT);

		
		/* Export GPIO1_7 (P8-4 on BBB) output as GPIO6 */

		sys_padconf(CONTROL_CONF_MCASP0_FSR, 0xffffffff,
		    (CONTROL_CONF_PUTYPESEL | CONTROL_CONF_MUXMODE(7)));

		add_gpio_inode("GPIO6", (32 * 1) + 7, GPIO_MODE_OUTPUT);

		/* Export GPIO1_17 (P9-23 on BBB) input as RIGHT */

		/* assumes external pull-up resistor (10K) */
		sys_padconf(CONTROL_CONF_SPI0_D0, 0xffffffff, (CONTROL_CONF_RXACTIVE |
		    CONTROL_CONF_PUDEN | CONTROL_CONF_MUXMODE(7)));
		
		add_gpio_inode("RIGHT", (32 * 1) + 17, GPIO_MODE_INPUT);

	}
}

static ssize_t
    read_hook
    (struct inode *inode, char *ptr, size_t len, off_t offset, cbdata_t cbdata)
{
	/* This hook will be called every time a regular file is read. We use
	 * it to dyanmically generate the contents of our file. */
	int value;
	struct gpio_cbdata *gpio_cbdata = (struct gpio_cbdata *) cbdata;
	assert(gpio_cbdata->gpio != NULL);
	
	if (gpio_cbdata->type == GPIO_CB_ON
	    || gpio_cbdata->type == GPIO_CB_OFF) {
		/* turn on or off */
		if (gpio_set(gpio_cbdata->gpio,
			(gpio_cbdata->type == GPIO_CB_ON) ? 1 : 0))
			return EIO;
		return 0;
	}

	if (gpio_cbdata->type == GPIO_CB_INTR_READ) {
		/* reading interrupt */
		if (gpio_intr_read(gpio_cbdata->gpio, &value))
			return EIO;
	} else {
		/* reading */
		if (gpio_read(gpio_cbdata->gpio, &value))
			return EIO;
	}
	snprintf(ptr, DATA_SIZE, "%d\n", value);
	len = strlen(ptr);

	/* If the offset is at or beyond the end of the string, return EOF. */
	if (offset >= len)
		return 0;

	/* Otherwise, we may have to move the data to the start of ptr. */
	if (offset > 0) {
		len -= offset;

		memmove(ptr, &ptr[offset], len);
	}

	/* Return the resulting length. */
	return len;
}

static void
message_hook(message * m, int __unused ipc_status)
{
	message recvm = *m;
	struct inode *gpionode;
	char gpioname[10];
	char *ptr;
	int retval,r;
	endpoint_t caller;

	if(recvm.m_type == GPIO_UPDATE){
		printf("GPIO : received command from gpiodriver : GPIO# : %d , value : %d\n",recvm.m_m1.m1i1,recvm.m_m1.m1i2);
		if(recvm.m_m1.m1i2 == 1)
			snprintf(gpioname,10,"GPIO%dOn",recvm.m_m1.m1i1);
		else if (recvm.m_m1.m1i2 == 0)
			snprintf(gpioname,10,"GPIO%dOff",recvm.m_m1.m1i1);
		else{
			printf("Error! Invalid value");
			exit(-1);		
		}
		gpionode = get_inode_by_name(get_root_inode(), gpioname);
		//printf("inode of the GPIO : %s received, inode name : %s\n",gpioname,get_inode_name(gpionode));
		retval = read_hook(gpionode, ptr, 4096, 0, get_inode_cbdata(gpionode));
		caller = recvm.m_source;
		recvm.m_type = GPIO_UPDATE;
		recvm.m_m1.m1i1 = 1;

		r = ipc_sendnb(caller, &recvm);
		if (r != OK) {
			log_warn(&log, "sendnb() failed\n");
		}
	}
	else
		gpio_intr_message(m);
}

int
main(int argc, char **argv)
{

	struct fs_hooks hooks;
	struct inode_stat root_stat;

	/* Set and apply the environment */
	env_setargs(argc, argv);

	/* fill in the hooks */
	memset(&hooks, 0, sizeof(hooks));
	hooks.init_hook = init_hook;
	hooks.read_hook = read_hook;
	hooks.message_hook = message_hook;

	root_stat.mode = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH;
	root_stat.uid = 0;
	root_stat.gid = 0;
	root_stat.size = 0;
	root_stat.dev = NO_DEV;

	/* run VTreeFS */
	run_vtreefs(&hooks, 30, 0, &root_stat, 0, DATA_SIZE);

	return EXIT_SUCCESS;
}
