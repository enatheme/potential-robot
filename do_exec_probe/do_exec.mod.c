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
	{ 0x4b19796d, "module_layout" },
	{ 0x72df2f2a, "up_read" },
	{ 0xd66eb359, "call_usermodehelper_fns" },
	{ 0xd0f0d945, "down_read" },
	{ 0xd68f27d4, "get_task_mm" },
	{ 0xcad4e7f, "pid_task" },
	{ 0x227e34b5, "find_get_pid" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x38cae44b, "access_remote_vm" },
	{ 0x20c55ae0, "sscanf" },
	{ 0xb6ed1e53, "strncpy" },
	{ 0x17600056, "filp_open" },
	{ 0xce9a7f81, "filp_close" },
	{ 0x14cebbae, "vfs_read" },
	{ 0xcd1516df, "register_kprobe" },
	{ 0x50eedeb8, "printk" },
	{ 0x9d62c898, "unregister_kprobe" },
	{ 0x272ffb65, "current_task" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

