// SPDX-License-Identifier: GPL-2.0
/*
 * Learner scaffold. Follow the five labs in order and preserve each verified
 * increment as you build the complete event recorder.
 */
#include <linux/module.h>
#include <linux/platform_device.h>

static int irqrec_probe(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "irqrec learner scaffold bound\n");
	return 0;
}

static void irqrec_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "irqrec learner scaffold removed\n");
}

static struct platform_driver irqrec_driver = {
	.probe = irqrec_probe,
	.remove = irqrec_remove,
	.driver = { .name = "irqrec" },
};
module_platform_driver(irqrec_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Learner scaffold for the GPIO interrupt event recorder");

