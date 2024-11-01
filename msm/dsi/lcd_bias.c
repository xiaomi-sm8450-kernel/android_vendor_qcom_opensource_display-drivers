#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include "lcd_bias.h"

static struct i2c_client *panel_pw_cust;
#define LCD_BIAS_CUST_DRV_NAME "lcd_bias"

#define LCD_BIAS_POWER 0x14                 /*AVDD/AVEE voltage, set 6V*/
#define LCD_BIAS_AVDD_ADDR 0x00       /*AVDD_ADDR*/
#define LCD_BIAS_AVEE_ADDR 0x01        /*AVEE_ADDR*/

#define LCD_BIAS_DEBUG    1
#define BIAS_LOG(fmt, args...)    pr_info("[lcd_bias] %s %d: " fmt, __func__, __LINE__, ##args)
#define BIAS_ERR(fmt, args...)    pr_err("[lcd_bias] %s %d: " fmt, __func__, __LINE__, ##args)
#if LCD_BIAS_DEBUG
#define BIAS_DEBUG(fmt, args...)    pr_err("[lcd_bias] %s %d: " fmt, __func__, __LINE__, ##args)
#endif

struct bias_data
{
    int reg_value;
};

inline int set_lcd_bias_by_index(bool power)
{
    unsigned int rc = -EINVAL;
    if (!panel_pw_cust) {
        BIAS_ERR("lcd_bias not registered\n");
        return rc;
    }

    rc = i2c_smbus_write_byte_data(panel_pw_cust, LCD_BIAS_AVDD_ADDR, LCD_BIAS_POWER);
    if (rc < 0) {
        BIAS_ERR("Failed to write 0x%02x to 0x00 register\n", LCD_BIAS_POWER);
    }
    rc = i2c_smbus_write_byte_data(panel_pw_cust, LCD_BIAS_AVEE_ADDR, LCD_BIAS_POWER);
    if (rc < 0) {
        BIAS_ERR("Failed to write 0x%02x to 0x01 register\n", LCD_BIAS_POWER);
    }

    return rc;
}

static ssize_t lcd_bias_register_store(struct device *dev,
                struct device_attribute *attr, const char *buf, size_t count)
{
    int reg_addr = 0, reg_val = 0;

    if (sscanf(buf, "%d %d", &reg_addr, &reg_val) != 2) {
        BIAS_ERR("Invalid input format for write_reg\n");
        return -EINVAL;
    }

    if(0!= i2c_smbus_write_byte_data(panel_pw_cust, (unsigned char)reg_addr,
                                (unsigned char)reg_val)){
        BIAS_ERR("Failed to write to I2C device\n");
        return -EINVAL;
    }

    BIAS_LOG("Write Successfully\n");
    return count;
}

static ssize_t lcd_bias_register_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
        size_t count = 0;
        u8 reg[0x04] = { 0 };
        int i = 0;

        for (i = 0; i < 0x4; i++) {
                reg[i] = i2c_smbus_read_byte_data(panel_pw_cust, i);
                count += sprintf(&buf[count], "0x%02x: 0x%02x\n", i, reg[i]);
        }

        return count;
}

static DEVICE_ATTR(lcd_bias, S_IWUSR | S_IRUGO, lcd_bias_register_show,
                lcd_bias_register_store);

static struct attribute *lcd_bias_attrs[] = {
    &dev_attr_lcd_bias.attr,
    NULL,
};

static struct attribute_group lcd_bias_group = {
    .attrs = lcd_bias_attrs,
};

static int lcd_bias_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
    struct bias_data * pdata;
    int ret;

    BIAS_LOG("probed start\n");
    if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C|I2C_FUNC_SMBUS_BYTE_DATA)) {
        BIAS_ERR("I2C functionality not Supported\n");
        return -ENOTSUPP;
    }

    pdata = devm_kzalloc(&i2c->dev,sizeof(struct bias_data), GFP_KERNEL);
    if (!pdata) {
        BIAS_ERR("Failed to allocate memory for pdata\n");
        return -ENOMEM;
    }

    ret = of_property_read_u32(i2c->dev.of_node, "reg", &pdata->reg_value);
    if (ret) {
        BIAS_ERR("Failed to read reg property from DT\n");
        return ret;
    }

    BIAS_LOG("probed at address 0x%02x\n", i2c->addr);
    panel_pw_cust = i2c;

    return sysfs_create_group(&i2c->dev.kobj, &lcd_bias_group);
}

static int lcd_bias_remove(struct i2c_client *i2c)
{
    sysfs_remove_group(&i2c->dev.kobj, &lcd_bias_group);
    BIAS_LOG("removed from address 0x%02x\n", i2c->addr);
    return 0;
}

static void lcd_bias_shutdown(struct i2c_client *i2c)
{
    sysfs_remove_group(&i2c->dev.kobj, &lcd_bias_group);
    BIAS_LOG("shutdown from address 0x%02x\n", i2c->addr);
}

static const struct i2c_device_id lcd_bias_id[] = {
    { LCD_BIAS_CUST_DRV_NAME, 0 },
    {}
};
static struct of_device_id bias_match_table[] = {
    { .compatible = "lct,lcd_bias",},
    { },
};

static struct i2c_driver lcd_bias_driver = {
    .driver = {
        .name = LCD_BIAS_CUST_DRV_NAME,
        .owner = THIS_MODULE,
        .of_match_table = bias_match_table,
    },
    .probe = lcd_bias_probe,
    .remove = lcd_bias_remove,
    .shutdown = lcd_bias_shutdown,
    .id_table = lcd_bias_id,
};

void lcd_bias_i2c_init(void)
{
    i2c_add_driver(&lcd_bias_driver);
}

void lcd_bias_i2c_exit(void)
{
    i2c_del_driver(&lcd_bias_driver);
}

MODULE_AUTHOR("LCD BIAS");
MODULE_DESCRIPTION("A Custom LCD Bias Control I2C Driver with Read/Write Reg Support");
MODULE_LICENSE("GPL");
