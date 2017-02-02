#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xf1d969b2, "module_layout" },
	{ 0x3ce4ca6f, "disable_irq" },
	{ 0x4b3dde51, "cdev_del" },
	{ 0x1297b2e9, "cdev_init" },
	{ 0x4d7272e4, "migrate_enable" },
	{ 0x67c2fa54, "__copy_to_user" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x86c8ad2e, "malloc_sizes" },
	{ 0x8b894aa3, "__rt_mutex_init" },
	{ 0x886fa8d9, "device_destroy" },
	{ 0x432fd7f6, "__gpio_set_value" },
	{ 0x403f9529, "gpio_request_one" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x6b3c2750, "rt_spin_lock" },
	{ 0x45e85b44, "rt_spin_unlock" },
	{ 0xbf8b2fd3, "__init_waitqueue_head" },
	{ 0x5f754e5a, "memset" },
	{ 0x27e1a049, "printk" },
	{ 0xaa9e7a50, "device_create" },
	{ 0x1c132024, "request_any_context_irq" },
	{ 0x49608959, "migrate_disable" },
	{ 0xbaa8b11b, "__rt_spin_lock_init" },
	{ 0xf7c184d4, "cdev_add" },
	{ 0x9bf67b84, "kmem_cache_alloc" },
	{ 0x11f447ce, "__gpio_to_irq" },
	{ 0x1000e51, "schedule" },
	{ 0xc2165d85, "__arm_iounmap" },
	{ 0x5952a882, "__raw_spin_lock_init" },
	{ 0x72c277a4, "__wake_up" },
	{ 0xfe990052, "gpio_free" },
	{ 0x37a0cba, "kfree" },
	{ 0x9e653656, "prepare_to_wait" },
	{ 0xd9aad86a, "class_destroy" },
	{ 0xc8ef03b0, "finish_wait" },
	{ 0x40a6f522, "__arm_ioremap" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0xac8f37b2, "outer_cache" },
	{ 0xccadb82a, "__class_create" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0xf20dabd8, "free_irq" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "1F8A431C9F7AF3B49E5D213");
