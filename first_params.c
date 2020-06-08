/* first_params.c */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static char *message = "Hola world!";
module_param(message, charp, S_IWUSR);
MODULE_PARM_DESC(message, "The message to print");

static int __init first_init(void)
{
  pr_info("%s\n", message);
  return 0;
}

static void __exit first_exit(void)
{
  pr_info("Bye\n");
}

module_init(first_init);
module_exit(first_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("My first module with parameters");
MODULE_AUTHOR("The Doctor");