#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#define HIGH 1
#define LOW 0

int led[4] = {23, 24, 25, 1};
int sw[4] = {4, 17, 27, 22};
int flag = 0;	//switch1 led control
char v2 = 0;	//switch2 led control
int a = 0;	//set switch3 mode

static struct timer_list timer;
struct task_struct *thread_id = NULL;

/* function list */
static void timer_cb(struct timer_list *timer);
static void timer_cb2(struct timer_list *timer);

static void led_get(void){
 int ret, i;
 printk(KERN_INFO "led get ~~!\n");
 for(i=0; i<4; i++){
  ret = gpio_request(led[i], "LED");
  if(ret < 0)
   printk(KERN_INFO "led_module gpio_request failed\n");
 }
}

static void led_mode_0(void){
 printk(KERN_INFO "led_mode_0\n");
 timer_setup(&timer, timer_cb, 0);
 timer.expires = jiffies + HZ * 2;
 add_timer(&timer);
}

static void timer_cb(struct timer_list *timer){
 int  i;
 printk(KERN_INFO "timer 2sec\n");
// led on off 
 if(flag == 0){
  for(i=0; i<4; i++){	
   gpio_direction_output(led[i], HIGH);
  }
  flag = 1;
 }
 else{
  for(i=0; i<4; i++){
   gpio_direction_output(led[i], LOW);
  }
  flag = 0;
 }
 timer->expires = jiffies + HZ * 2;
 add_timer(timer);
}

static void timer_cb2(struct timer_list *timer){
 int i;
 printk(KERN_INFO "timer cb 222\n");
 for(i=0; i<4; i++){
  if(i == v2)
   gpio_direction_output(led[i], HIGH);
  else
   gpio_direction_output(led[i], LOW);
 }
 printk(KERN_INFO "timer cb 2 loop\n");
 v2 = v2 + 1;
 if(v2 >= 4)
  v2 = 0;
 timer->expires = jiffies + HZ*2;
 add_timer(timer);

}

static void led_mode_1(void)
{
 printk(KERN_INFO "led_mode_1\n");
 timer_setup(&timer, timer_cb2, 0);
 timer.expires = jiffies + HZ * 2;
 add_timer(&timer);
}

static void change(int num)
{
 int ret;
 if(gpio_get_value(led[num]) == 0)
  ret = gpio_direction_output(led[num], HIGH);
 else
  ret = gpio_direction_output(led[num], LOW);
}

irqreturn_t irq_handler(int irq, void *dev_id){
 int i;
 v2 = 0;
 flag = 0;
 del_timer(&timer);
 if(a == 0){
  for(i=0; i<4; i++){
   gpio_direction_output(led[i], LOW);
  }
 }

 switch(irq){
  case 60:
    printk(KERN_INFO "sw1\n");
   if(a == 0)
    led_mode_0();
   else
	change(0);
   break;
  case 61:
   printk(KERN_INFO "sw2\n");
   if(a == 0)
    led_mode_1();
   else
	change(1);
   break;
  case 62:
   printk(KERN_INFO "sw3\n");
   if(a == 0)
	a = 1;
   else
	change(2);
   break;
  case 63:
   printk(KERN_INFO "sw4\n");
   for(i=0; i<4; i++)
	gpio_direction_output(led[i], LOW);
   a = 0;
   break;
 }
 return 0;
}

static int switch_interrupt_init(void){
 int res, i;
 printk(KERN_INFO "sw interrupt init!\n");
 led_get();
 for(i=0; i<4; i++){
  res = gpio_request(sw[i], "sw");
  res = request_irq(gpio_to_irq(sw[i]), (irq_handler_t)irq_handler, IRQF_TRIGGER_RISING, "IRQ", (void*)(irq_handler));
  if(res < 0)
   printk(KERN_INFO "request_irq failed\n");
 }
 return 0;
}

static void switch_interrupt_exit(void){
 int i;
 printk(KERN_INFO "switch_interrupt_exit!\n");
 del_timer(&timer);
 for(i=0; i<4; i++){
  free_irq(gpio_to_irq(sw[i]), (void*)(irq_handler));
  gpio_free(sw[i]);
  gpio_free(led[i]);
 }
}

module_init(switch_interrupt_init);
module_exit(switch_interrupt_exit);
MODULE_LICENSE("GPL");


