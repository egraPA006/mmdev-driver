/**
 * Sysfs:       /sys/devices/platform/soc/1c2a800.mmdev
 * Misc device: /dev/mmdev
 * 
⢀⡴⠑⡄⠀⠀⠀⠀⠀⠀⠀⣀⣀⣤⣤⣤⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀ 
⠸⡇⠀⠿⡀⠀⠀⠀⣀⡴⢿⣿⣿⣿⣿⣿⣿⣿⣷⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀ 
⠀⠀⠀⠀⠑⢄⣠⠾⠁⣀⣄⡈⠙⣿⣿⣿⣿⣿⣿⣿⣿⣆⠀⠀⠀⠀⠀⠀⠀⠀ 
⠀⠀⠀⠀⢀⡀⠁⠀⠀⠈⠙⠛⠂⠈⣿⣿⣿⣿⣿⠿⡿⢿⣆⠀⠀⠀⠀⠀⠀⠀ 
⠀⠀⠀⢀⡾⣁⣀⠀⠴⠂⠙⣗⡀⠀⢻⣿⣿⠭⢤⣴⣦⣤⣹⠀⠀⠀⢀⢴⣶⣆ 
⠀⠀⢀⣾⣿⣿⣿⣷⣮⣽⣾⣿⣥⣴⣿⣿⡿⢂⠔⢚⡿⢿⣿⣦⣴⣾⠁⠸⣼⡿ 
⠀⢀⡞⠁⠙⠻⠿⠟⠉⠀⠛⢹⣿⣿⣿⣿⣿⣌⢤⣼⣿⣾⣿⡟⠉⠀⠀⠀⠀⠀ 
⠀⣾⣷⣶⠇⠀⠀⣤⣄⣀⡀⠈⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀ 
⠀⠉⠈⠉⠀⠀⢦⡈⢻⣿⣿⣿⣶⣶⣶⣶⣤⣽⡹⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀ 
⠀⠀⠀⠀⠀⠀⠀⠉⠲⣽⡻⢿⣿⣿⣿⣿⣿⣿⣷⣜⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀ 
⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⣷⣶⣮⣭⣽⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀ 
⠀⠀⠀⠀⠀⠀⣀⣀⣈⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠇⠀⠀⠀⠀⠀⠀⠀ 
⠀⠀⠀⠀⠀⠀⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠃⠀⠀⠀⠀⠀⠀⠀⠀ 
⠀⠀⠀⠀⠀⠀⠀⠹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠟⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀ 
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠛⠻⠿⠿⠿⠿⠛⠉
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/mod_devicetable.h>
#include <linux/of_device.h>
#include <linux/kern_levels.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/delay.h>

#define MISCREGSIZE 2
#define KBUFFERSIZE 16
#define USLEEP_MIN  1000
#define USLEEP_MAX  10000

#define ENABLE_BIT      0
#define IRQ_BIT         1
#define FREQ_BIT        2
#define INITW_BIT       0


#define VALUE_MASK      0xFFFF
#define CONFIG_OFF      0x0
#define STATUS_OFF      0x4
#define INITVAL_OFF     0x8
#define DATA_OFF        0xC

#define BIT_CLR(x) (~(1 << (x)))
#define BIT_SET(x) (1 << (x))

/**
 * "Private" memory for each device
 */
struct mmdev_priv_mem {
    void __iomem *regs;
    int irq;
    struct miscdevice misc_dev;
    struct device *dev;
    struct work_struct work;
};

static int mm_open(struct inode *inode, struct file *file)
{
    struct mmdev_priv_mem *priv = container_of(file->private_data, struct mmdev_priv_mem, misc_dev);
    struct device *dev = priv->dev;
    file->private_data = priv;
	if(file->f_mode & FMODE_READ) 
		dev_info(dev, "Open called with read permissions\n");
	if(file->f_mode & FMODE_WRITE) 
		dev_info(dev, "Open called with write permissions\n");
	return 0;
}

static int mm_close(struct inode *inode, struct file *file)
{
    struct mmdev_priv_mem *priv = file->private_data;
    struct device *dev = priv->dev;
	dev_info(dev, "Close called\n");
	return 0;
}

/**
 * write initial value and start counter
 * if initw status reg marks error - return error
 * Enables device (without interrupts)
 */
static ssize_t mm_write(struct file *file, const char __user *user_buffer, size_t user_len, loff_t *ppos)
{
    struct mmdev_priv_mem *priv = file->private_data;
    struct device *dev = priv->dev;
    u32 val;
    int ret;
    char kbuf[KBUFFERSIZE];
    int status;
    
    if (user_len >= KBUFFERSIZE - 1)
        return -EINVAL;
    if (copy_from_user(kbuf, user_buffer, user_len))
        return -EFAULT;
    kbuf[user_len] = '\0';
    ret = kstrtouint(kbuf, 10, &val);
    if (ret)
        return ret;
    
    if (val > VALUE_MASK)
        return -EINVAL;
    
    iowrite32(val, priv->regs + INITVAL_OFF);
    status = ioread32(priv->regs + STATUS_OFF);
    if (status & BIT_SET(INITW_BIT))
        return -EINVAL;
    val = ioread32(priv->regs + CONFIG_OFF);
    val |= BIT_SET(ENABLE_BIT);
    iowrite32(val, priv->regs + CONFIG_OFF);
    dev_info(dev, "Write called\n");
    return user_len;
}


/**
 * read current data counter value
 */
static ssize_t mm_read(struct file *file, char __user *user_buffer, size_t user_len, loff_t *ppos)
{
    struct mmdev_priv_mem *priv = file->private_data;
    u32 reg_value;
    char buf[KBUFFERSIZE];
    int len;
    size_t bytes_to_copy;

    reg_value = ioread32(priv->regs + DATA_OFF);
    reg_value &= VALUE_MASK;

    len = snprintf(buf, sizeof(buf), "%hu\n", reg_value);

    if (*ppos >= len)
        return 0;

    bytes_to_copy = min_t(size_t, user_len, len - *ppos);

    if (copy_to_user(user_buffer, buf + *ppos, bytes_to_copy)) {
        dev_err(priv->dev, "Failed to copy data to userspace\n");
        return -EFAULT;
    }
    *ppos += bytes_to_copy;

    dev_info(priv->dev, "Read called\n");

    return bytes_to_copy;
}


static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = mm_read,
	.write = mm_write,
	.open = mm_open,
	.release = mm_close,
};

static ssize_t enable_show(struct device *dev, struct device_attribute *attr,
    char *buf)
{
    struct mmdev_priv_mem *priv = dev_get_drvdata(dev);
    u32 data = ioread32(priv->regs + CONFIG_OFF);
    return sysfs_emit(buf, "%d\n", data & BIT_SET(ENABLE_BIT));
}

static ssize_t enable_store(struct device *dev, struct device_attribute *attr,
    const char *buf, size_t count)
{
    struct mmdev_priv_mem *priv = dev_get_drvdata(dev);
    if (count > MISCREGSIZE) {
        dev_warn(dev, "Enable store: undefined sequence, did not write, count: %zu", count);
        return count;
    }
    u32 cur = ioread32(priv->regs + CONFIG_OFF);
    switch (buf[0])
    {
    case '0':
        cur &= BIT_CLR(ENABLE_BIT);
        break;
    case '1':
        cur |= BIT_SET(ENABLE_BIT);
        break;
    default:
        dev_warn(dev, "Enable store: undefined sequence, did not write, char: %c", buf[0]);
        return count;
    }
    iowrite32(cur, priv->regs + CONFIG_OFF);
    return count;
}

static DEVICE_ATTR_RW(enable);

static ssize_t irq_status_show(struct device *dev, struct device_attribute *attr,
    char *buf)
{
    struct mmdev_priv_mem *priv = dev_get_drvdata(dev);
    u32 data = ioread32(priv->regs + STATUS_OFF);
    return sysfs_emit(buf, "%d\n", (data & BIT_SET(IRQ_BIT)) >> 1);
}

static DEVICE_ATTR_RO(irq_status);


static ssize_t irq_enable_show(struct device *dev, struct device_attribute *attr,
    char *buf)
{
    struct mmdev_priv_mem *priv = dev_get_drvdata(dev);
    u32 data = ioread32(priv->regs + CONFIG_OFF);
    return sysfs_emit(buf, "%d\n", (data & BIT_SET(IRQ_BIT)) >> 1);
}

static ssize_t irq_enable_store(struct device *dev, struct device_attribute *attr,
    const char *buf, size_t count)
{
    struct mmdev_priv_mem *priv = dev_get_drvdata(dev);
    if (count > MISCREGSIZE) {
        dev_warn(dev, "Interrupt store: undefined sequence, did not write, count: %zu", count);
        return count;
    }
    u32 cur = ioread32(priv->regs + CONFIG_OFF);
    switch (buf[0])
    {
    case '0':
        cur &= BIT_CLR(IRQ_BIT);
        break;
    case '1':
        cur |= BIT_SET(IRQ_BIT);
        break;
    default:
        dev_warn(dev, "Interrupt store: undefined sequence, did not write, char: %c", buf[0]);
        return count;
    }
    iowrite32(cur, priv->regs + CONFIG_OFF);
    return count;
}

static DEVICE_ATTR_RW(irq_enable);

static ssize_t freq_show(struct device *dev, struct device_attribute *attr,
    char *buf)
{
    struct mmdev_priv_mem *priv = dev_get_drvdata(dev);
    u32 data = ioread32(priv->regs + CONFIG_OFF);
    return sysfs_emit(buf, "%d\n", (data & BIT_SET(FREQ_BIT)) >> 2);
}

static ssize_t freq_store(struct device *dev, struct device_attribute *attr,
    const char *buf, size_t count)
{
    struct mmdev_priv_mem *priv = dev_get_drvdata(dev);
    if (count > MISCREGSIZE) {
        dev_warn(dev, "Frequency store: undefined sequence, did not write, count: %zu", count);
        return count;
    }
    u32 cur = ioread32(priv->regs + CONFIG_OFF);
    switch (buf[0])
    {
    case '0':
        cur &= BIT_CLR(FREQ_BIT);
        break;
    case '1':
        cur |= BIT_SET(FREQ_BIT);
        break;
    default:
        dev_warn(dev, "Frequency store: undefined sequence, did not write, char: %c", buf[0]);
        return count;
    }
    iowrite32(cur, priv->regs + CONFIG_OFF);
    return count;
}

static DEVICE_ATTR_RW(freq);

static ssize_t list_freq_show(struct device *dev, struct device_attribute *attr,
    char *buf)
{
    static char ans[] = "Available frequencies:\n0 - normal (1 Hz),\n1 - fast (2 Hz)\n";
    return sysfs_emit(buf, ans);
}

static DEVICE_ATTR_RO(list_freq);

static ssize_t init_val_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct mmdev_priv_mem *priv = dev_get_drvdata(dev);
    u32 data = ioread32(priv->regs + INITVAL_OFF);
    data &= VALUE_MASK;
    return sysfs_emit(buf, "%d\n", data);
}

/**
 * Does not care if the INITW is up, for safe write use write() misc device func
 */
static ssize_t init_val_store(struct device *dev, struct device_attribute *attr,
    const char *buf, size_t count)
{
    struct mmdev_priv_mem *priv = dev_get_drvdata(dev);
    u32 val;
    int ret = kstrtouint(buf, 10, &val);
    if (ret) {
        dev_warn(dev, "Init store: could not convert string to integer\n");
        return ret;
    }
    if (val > VALUE_MASK) {
        dev_warn(dev, "Value %u exceeds 16-bit maximum\n", val);
        val &= VALUE_MASK;
    }
    iowrite32(val, priv->regs + INITVAL_OFF);
    return count;
}

static DEVICE_ATTR_RW(init_val);

static ssize_t data_show(struct device *dev, struct device_attribute *attr,
    char *buf)
{
    struct mmdev_priv_mem *priv = dev_get_drvdata(dev);
    u32 data = ioread32(priv->regs + DATA_OFF);
    data &= VALUE_MASK;
    return sysfs_emit(buf, "%d\n", data);
}

static DEVICE_ATTR_RO(data);

static struct attribute *mmdev_attrs[] = {
    &dev_attr_enable.attr,
    &dev_attr_irq_status.attr,
    &dev_attr_irq_enable.attr,
    &dev_attr_freq.attr,
    &dev_attr_list_freq.attr,
    &dev_attr_init_val.attr,
    &dev_attr_data.attr,
    NULL,
};
ATTRIBUTE_GROUPS(mmdev);


static void mmdev_interrupt_func(struct work_struct *work){
    struct mmdev_priv_mem *priv = container_of(work, struct mmdev_priv_mem, work);
    usleep_range(USLEEP_MIN, USLEEP_MAX);
    dev_info(priv->dev, "Scheduled interupt message\n");
}

static irqreturn_t mmdev_interrupt(int irq, void *dev_id)
{
    struct mmdev_priv_mem *priv = dev_id;
    u32 cur = ioread32(priv->regs + STATUS_OFF);
    if (!(cur & BIT_SET(IRQ_BIT)))
        return IRQ_NONE;
    cur &= BIT_CLR(IRQ_BIT);
    iowrite32(cur, priv->regs + STATUS_OFF);
    schedule_work(&priv->work);
    return IRQ_HANDLED;
}

static int mmdev_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct resource *res;
    int ret;
    struct mmdev_priv_mem *priv;
    u32 cur;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0); // read reg field
    if (!res) {
        dev_err(dev, "No memory resource specified\n");
        return -ENODEV;
    }

    priv = devm_kzalloc(dev, sizeof(struct mmdev_priv_mem), GFP_KERNEL);
    if (!priv) {
        return -ENOMEM;
    }
    priv->dev = dev;
    priv->regs = devm_ioremap_resource(dev, res);
    INIT_WORK(&priv->work, mmdev_interrupt_func);
    priv->irq = platform_get_irq(pdev, 0);
    if (priv->irq < 0) {
        dev_err(dev, "No IRQ specified\n");
        return priv->irq;
    }

    ret = devm_request_irq(dev, priv->irq, mmdev_interrupt,
        0, dev_name(dev), priv);
    if (ret) {
        dev_err(dev, "Failed to request IRQ %d: %d\n", priv->irq, ret);
        return ret;
    }

    priv->misc_dev.name = "mmdev";
    priv->misc_dev.minor = MISC_DYNAMIC_MINOR;
    priv->misc_dev.fops = &fops;

    ret = misc_register(&priv->misc_dev);
    if (ret) {
        dev_err(dev, "Failed to register misc device: %d\n", ret);
        return ret;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &mmdev_group);
    if (ret) {
        misc_deregister(&priv->misc_dev);
        dev_err(dev, "Failed to create sysfs group\n");
        return ret;
    }

    cur = ioread32(priv->regs + STATUS_OFF);
    cur &= BIT_CLR(IRQ_BIT);
    iowrite32(cur, priv->regs + STATUS_OFF);

    platform_set_drvdata(pdev, priv); // save private data for this device
    
    dev_info(dev, "Device probed successfully, irq is %d\n", priv->irq);
    return 0;
}

static int mmdev_remove(struct platform_device *pdev)
{
    struct mmdev_priv_mem *priv = platform_get_drvdata(pdev);
    misc_deregister(&priv->misc_dev);
    sysfs_remove_group(&pdev->dev.kobj, &mmdev_group);
    cancel_work_sync(&priv->work);
    dev_info(&pdev->dev, "Device removed\n");
    return 0;
}

/**
 * Register our driver for the device
 */
static const struct of_device_id mmdev_of_match[] = {
    { .compatible = "ymp,mmdev" },
    { }
};
MODULE_DEVICE_TABLE(of, mmdev_of_match);

/**
 * General info of our driver
 */
static struct platform_driver mmdev_driver = {
    .driver = {
        .name = "mmdev",
        .of_match_table = mmdev_of_match,
    },
    .probe = mmdev_probe,
    .remove = mmdev_remove,
};

static int __init mmdev_init(void)
{
    pr_info("Init test module start\n");
    if(platform_driver_register(&mmdev_driver)) {
		pr_err("dt_probe - Error! Could not load driver\n");
		return -1;
	}
    pr_info("Init test module finish\n");
    return 0;
}

static void __exit mmdev_exit(void)
{
    platform_driver_unregister(&mmdev_driver);
    pr_info("Bye from test module!\n");
}

module_init(mmdev_init);
module_exit(mmdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Egor Pustovoytenko puseg2006@gmail.com");
MODULE_DESCRIPTION("Driver for YMP MMDEV device");