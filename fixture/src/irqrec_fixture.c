// SPDX-License-Identifier: GPL-2.0
/* QEMU lab board-description fixture. This is not part of the device driver. */
#include <linux/gpio/machine.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/property.h>

static struct platform_device *irqrec_pdev;

static const struct property_entry irqrec_properties[] = {
	PROPERTY_ENTRY_U32("debounce-us", 50000),
	PROPERTY_ENTRY_U32("queue-depth", 64),
	{ }
};

static struct gpiod_lookup_table irqrec_gpio_table = {
	.dev_id = "irqrec",
	.table = {
		GPIO_LOOKUP("irqrec-event", U16_MAX, "event", GPIO_ACTIVE_HIGH),
		{ }
	},
};

static int __init irqrec_fixture_init(void)
{
	struct platform_device_info device_info = {
		.name = "irqrec",
		.id = PLATFORM_DEVID_NONE,
		.properties = irqrec_properties,
	};

	gpiod_add_lookup_table(&irqrec_gpio_table);
	irqrec_pdev = platform_device_register_full(&device_info);
	if (IS_ERR(irqrec_pdev)) {
		gpiod_remove_lookup_table(&irqrec_gpio_table);
		return PTR_ERR(irqrec_pdev);
	}
	return 0;
}

static void __exit irqrec_fixture_exit(void)
{
	platform_device_unregister(irqrec_pdev);
	gpiod_remove_lookup_table(&irqrec_gpio_table);
}

module_init(irqrec_fixture_init);
module_exit(irqrec_fixture_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("QEMU gpio-sim fixture for irqrec");

