/**
 * @file   fase1.c
 * @author Alvaro Castillejo
 * @date   13 November 2021
 * @brief  A kernel module for controlling 4 buttons and 2 LEDs. 
 * @see http://www.derekmolloy.ie/
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>                 // Required for the GPIO functions
#include <linux/interrupt.h>            // Required for the IRQ code
#include <linux/kmod.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alvaro Castillejo");
MODULE_DESCRIPTION("A Button/LED test driver for the BBB");
MODULE_VERSION("0.1");

//GPIO asignation
static unsigned int gpioLEDR = 16;       
static unsigned int gpioLEDG = 20;
static unsigned int gpioButton1 = 26;    
static unsigned int gpioButton2 = 19;
static unsigned int gpioButton3 = 13;
static unsigned int gpioButton4 = 21;

static unsigned int irqNumberBttn1;          ///< Used to share the IRQ number within this file
static unsigned int irqNumberBttn2;
static unsigned int irqNumberBttn3;
static unsigned int irqNumberBttn4;

//Times pressed
static unsigned int numberPresses1 = 0;  ///< For information, store the number of button presses
static unsigned int numberPresses2 = 0;
static unsigned int numberPresses3 = 0;
static unsigned int numberPresses4 = 0;

//LED states
static bool	    ledOnR = 0;          
static bool		ledOnG = 0;

//Scripts variables
static char * envp[] = { "HOME=/", "TERM=linux", "PATH=/sbin:/bin:/usr/sbin:/usr/bin", NULL };
static char * argvA[] = { "/home/pi/scripts/scriptA.sh", NULL };
static char * argvB[] = { "/home/pi/scripts/scriptB.sh", NULL };
static char * argvC[] = { "/home/pi/scripts/scriptC.sh", NULL };
static char * argvD[] = { "/home/pi/scripts/scriptD.sh", NULL };

/// Function prototype for the custom IRQ handler function -- see below for the implementation
static irq_handler_t  ebbgpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point. In this example this
 *  function sets up the GPIOs and the IRQ
 *  @return returns 0 if successful
 */
static int __init ebbgpio_init(void){
   int result1 = 0;
   int result2 = 0;
   int result3 = 0;
   int result4 = 0;
   printk(KERN_INFO "fase1: Initializing the fase1 LKM\n");
   // Is the GPIO a valid GPIO number (e.g., the BBB has 4x32 but not all available)
   if (!gpio_is_valid(gpioLEDR)){
      printk(KERN_INFO "fase1: invalid LEDR GPIO\n");
      return -ENODEV;
   }
   if (!gpio_is_valid(gpioLEDG)){
      printk(KERN_INFO "fase1: invalid LEDG GPIO\n");
      return -ENODEV;
   }
   // Going to set up the LED. It is a GPIO in output mode and will be off by default
   ledOnR = false;
   gpio_request(gpioLEDR, "sysfs");          
   gpio_direction_output(gpioLEDR, ledOnR);   
   gpio_export(gpioLEDR, false);  

   ledOnG = false; 
   gpio_request(gpioLEDG, "sysfs");          
   gpio_direction_output(gpioLEDG, ledOnG);   
   gpio_export(gpioLEDG, false);             
			                    
   gpio_request(gpioButton1, "sysfs");       
   gpio_direction_input(gpioButton1);        
   gpio_set_debounce(gpioButton1, 200);      
   gpio_export(gpioButton1, false);          

   gpio_request(gpioButton2, "sysfs");       
   gpio_direction_input(gpioButton2);        
   gpio_set_debounce(gpioButton2, 200);      
   gpio_export(gpioButton2, false); 
   
   gpio_request(gpioButton3, "sysfs");       
   gpio_direction_input(gpioButton3);        
   gpio_set_debounce(gpioButton3, 200);      
   gpio_export(gpioButton3, false); 
   
   gpio_request(gpioButton4, "sysfs");       
   gpio_direction_input(gpioButton4);        
   gpio_set_debounce(gpioButton4, 200);      
   gpio_export(gpioButton4, false); 
		                   
   // Perform a quick test to see that the button is working as expected on LKM load
   printk(KERN_INFO "fase1: The button state is currently: %d\n", gpio_get_value(gpioButton1));

   // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
   irqNumberBttn1 = gpio_to_irq(gpioButton1);
   printk(KERN_INFO "fase1: The buttonA is mapped to IRQ: %d\n", irqNumberBttn1);
   irqNumberBttn2 = gpio_to_irq(gpioButton2);
   printk(KERN_INFO "fase1: The buttonB is mapped to IRQ: %d\n", irqNumberBttn2);
   irqNumberBttn3 = gpio_to_irq(gpioButton3);
   printk(KERN_INFO "fase1: The buttonC is mapped to IRQ: %d\n", irqNumberBttn3);
   irqNumberBttn4 = gpio_to_irq(gpioButton4);
   printk(KERN_INFO "fase1: The buttonD is mapped to IRQ: %d\n", irqNumberBttn4);

   // This next call requests an interrupt line
   result1 = request_irq(irqNumberBttn1,             
                        (irq_handler_t) ebbgpio_irq_handler, 
                        IRQF_TRIGGER_RISING,    
                        "ebb_gpio_handler",    
                        NULL); 
   result2 = request_irq(irqNumberBttn2,             
                        (irq_handler_t) ebbgpio_irq_handler, 
                        IRQF_TRIGGER_RISING,    
                        "ebb_gpio_handler",    
                        NULL);
   result3 = request_irq(irqNumberBttn3,             
                        (irq_handler_t) ebbgpio_irq_handler, 
                        IRQF_TRIGGER_RISING,    
                        "ebb_gpio_handler",    
                        NULL); 
   result4 = request_irq(irqNumberBttn4,             
                        (irq_handler_t) ebbgpio_irq_handler, 
                        IRQF_TRIGGER_RISING,    
                        "ebb_gpio_handler",    
                        NULL);                                                                          

   printk(KERN_INFO "fase1: The interrupt request result1 is: %d\n", result1);
   return result1;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required. Used to release the
 *  GPIOs and display cleanup messages.
 */
static void __exit ebbgpio_exit(void){
   printk(KERN_INFO "fase1: The buttonA state is currently: %d\n", gpio_get_value(gpioButton1));
   printk(KERN_INFO "fase1: The buttonA was pressed %d times\n", numberPresses1);
   printk(KERN_INFO "fase1: The buttonB state is currently: %d\n", gpio_get_value(gpioButton2));
   printk(KERN_INFO "fase1: The buttonB was pressed %d times\n", numberPresses2);
   printk(KERN_INFO "fase1: The buttonC state is currently: %d\n", gpio_get_value(gpioButton3));
   printk(KERN_INFO "fase1: The buttonC was pressed %d times\n", numberPresses3);
   printk(KERN_INFO "fase1: The buttonD state is currently: %d\n", gpio_get_value(gpioButton4));
   printk(KERN_INFO "fase1: The buttonD was pressed %d times\n", numberPresses4);      
   gpio_set_value(gpioLEDR, 0);              // Turn the LEDs off, makes it clear the device was unloaded
   gpio_set_value(gpioLEDG, 0);
   gpio_unexport(gpioLEDR);                  // Unexport the LEDs GPIO
   gpio_unexport(gpioLEDG);
   free_irq(irqNumberBttn1, NULL);               // Free the IRQ numbers, no *dev_id required in this case
   free_irq(irqNumberBttn2, NULL);
   free_irq(irqNumberBttn3, NULL);
   free_irq(irqNumberBttn4, NULL);
   gpio_unexport(gpioButton1);               // Unexport the Buttons GPIO
   gpio_unexport(gpioButton2);
   gpio_unexport(gpioButton3);
   gpio_unexport(gpioButton4);
   gpio_free(gpioLEDR);                      // Free the LEDs GPIO
   gpio_free(gpioLEDG);
   gpio_free(gpioButton1);                   // Free the Buttons GPIO
   gpio_free(gpioButton2);
   gpio_free(gpioButton3);
   gpio_free(gpioButton4);
   printk(KERN_INFO "fase1: Goodbye from the LKM!\n");
}

/** @brief The GPIO IRQ Handler function
 *  This function is a custom interrupt handler that is attached to the GPIO above. The same interrupt
 *  handler cannot be invoked concurrently as the interrupt line is masked out until the function is complete.
 *  This function is static as it should not be invoked directly from outside of this file.
 *  @param irq    the IRQ number that is associated with the GPIO -- useful for logging.
 *  @param dev_id the *dev_id that is provided -- can be used to identify which device caused the interrupt
 *  Not used in this example as NULL is passed.
 *  @param regs   h/w specific register values -- only really ever used for debugging.
 *  return returns IRQ_HANDLED if successful -- should return IRQ_NONE otherwise.
 */
static irq_handler_t ebbgpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){       
   if(irq == irqNumberBttn1){
	   ledOnR = true;                          
	   gpio_set_value(gpioLEDR, ledOnR);          
	   printk(KERN_INFO "fase1: Interrupt! (buttonA state is %d)\n", gpio_get_value(gpioButton1));
       numberPresses1++; 
             
	   call_usermodehelper(argvA[0], argvA, envp, UMH_NO_WAIT);                       
       return (irq_handler_t) IRQ_HANDLED;      
   }
   if(irq == irqNumberBttn2){
	   ledOnR = false;                          
	   gpio_set_value(gpioLEDR, ledOnR);          
	   printk(KERN_INFO "fase1: Interrupt! (buttonB state is %d)\n", gpio_get_value(gpioButton1));
	   numberPresses2++;                         
	   	   
       call_usermodehelper(argvB[0], argvB, envp, UMH_NO_WAIT);                      
	   return (irq_handler_t) IRQ_HANDLED;      
   }
   if(irq == irqNumberBttn3){
	   ledOnG = true;                          
	   gpio_set_value(gpioLEDG, ledOnG);          
	   printk(KERN_INFO "fase1: Interrupt! (buttonC state is %d)\n", gpio_get_value(gpioButton1));
	   numberPresses3++;                         
       
	   call_usermodehelper(argvC[0], argvC, envp, UMH_NO_WAIT);                      
	   return (irq_handler_t) IRQ_HANDLED;      
   } else {
	   ledOnG = false;                          
	   gpio_set_value(gpioLEDG, ledOnG);          
	   printk(KERN_INFO "fase1: Interrupt! (buttonD state is %d)\n", gpio_get_value(gpioButton1));
	   numberPresses4++;                         
       
	   call_usermodehelper(argvD[0], argvD, envp, UMH_NO_WAIT);                      
	   return (irq_handler_t) IRQ_HANDLED;      
   }
   
}

/// This next calls are  mandatory -- they identify the initialization function
/// and the cleanup function (as above).
module_init(ebbgpio_init);
module_exit(ebbgpio_exit);
