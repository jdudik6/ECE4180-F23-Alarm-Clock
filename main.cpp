#include "mbed.h"
#include "rtos.h"
#include "Thread.h"
#include "Speaker.h"
#include "PinDetect.h"
// #include "XNucleo53L0A1.h"
#include <stdio.h>
#define VL53L0_I2C_SDA   p9
#define VL53L0_I2C_SCL   p10
#define EPOCH_SECONDS    (1701666000)   // Seconds since from 12/4/23 at 12:00:00 AM to EPOCH time

//Display time
//DONE Bluetooth buttons: set alarm, increase time, decrease time, change to minute, change to hour
//Bluetooth: change display to 12:00 before you set it
//DONE Speaker plays note when alarm on (time = alarm time)
//DONE TOF sensor: stops speaker when hand is close and speaker is playing


//TOF
//Bluetooth
//Speaker
//Class D amp for speaker
//Current time

// PwmOut speaker(p21);
Serial blue(p28,p27);       // Bluetooth Module
Speaker speaker(p21);       // Speaker
Serial pc(USBTX,USBRX);     // Serial to PC for debug
DigitalOut shdn(p26);       // TOF Sensor Reset

// Physical Pushbuttons     -- CHANGE PINS
PinDetect up(p20);
PinDetect down(p19);
PinDetect left(p18);
PinDetect right(p17);
PinDetect select(p16);


volatile uint32_t distance;

// Mode Variables
volatile bool time_set = 0;         // Has time been set by user
volatile bool hrs_mode = 1;       // Hours or minutes selected on display
volatile bool alarm_set_mode = 0;   // Time or Alarm displaying
volatile bool time_mode = 0;        // 12 or 24 Hour Time Mode


// Alarm Variables
volatile int alarm_min = 0;
volatile int alarm_hr = 0;
volatile int status;
volatile bool alarm_on;

// Time Tracking Variables
volatile int curr_hr;               // Current hour displayed on time set
volatile int curr_min;              // Current minute displayed on time set
int seconds_since;                  

// volatile time_t RTC_seconds;     // Seconds since 
// static XNucleo53L0A1 *board=NULL;

Mutex display_mutex;




// Thread 1
// Speaker plays oscillating tone if alarm goes off
void speakerThread(void const *args)
{
    // s = 1;

    // When the alarm is active, toggle the alarm sound until stopped
    while(true) {         
        if (alarm_on) {
            speaker.PlayNote(500, 0.25, 0.5);
            speaker.PlayNote(0, 0.25, 0.0);
        }
        // Thread::wait(500);    // wait 0.5s
    }
}

// Thread 2
// RGB LED
void bluetoothThread(void const *args)
{
    char bnum;
    char bhit;
    while(true) {
        // 
        // led_matrix.lock();
        while(!(blue.readable())) Thread::yield(); 
        if (blue.getc() == '!') {
            if (blue.getc()=='B') {
                bnum = blue.getc();
                bhit = blue.getc();
                    switch (bnum) {
                        case '1': //number 1 button
                            alarm_set_mode = !alarm_set_mode;
                            //switch display to between real time and alarm set time
                            break;
                        case '5': //up arrow - increase time
                            if (alarm_set_mode) {
                                if (hrs_mode) {
                                    if (alarm_hr == 23) alarm_hr = 0;
                                    else alarm_hr++;
                                } else {
                                    if (alarm_min == 59) alarm_min = 0;
                                    else alarm_min++;
                                }
                            }
                            break;

                        case '6': //down arrow - decrease time
                            if (alarm_set_mode) {
                                if (hrs_mode) {
                                    if (alarm_hr == 0) alarm_hr = 23;
                                    else alarm_hr--;
                                } else {
                                    if (alarm_min == 0) alarm_min = 59;
                                    else alarm_min--;
                                } 
                            }                       
                            break;

                        case '7': //left arrow - shift to hrs
                            if (alarm_set_mode) {
                                hrs_mode = 1;
                            }
                            break;
                        
                        case '8': //right arrow - shift to min
                            if (alarm_set_mode) {
                                hrs_mode = 0;
                            }
                            break;
                        
                        default:
                            break;
                    }
            }
        }
        display_mutex.unlock();
        Thread::wait(500);    // wait 1.5s
    }


}


// Thread3 led matrix thread
void displayThread(void const *arg)
{
    // Display Set Time Mode
    while(!time_set) {

    }

    // Normal Clock Functionality
    while(1) {
        // Display Normal Clock Mode
        if (!alarm_set_mode) {
            display_mutex.lock();

            display_mutex.unlock();
        }

        // Display Alarm Set Mode
        else {
            display_mutex.lock();

            display_mutex.unlock();
        }
    }
}

// Thread4 ToF sensor
void tofThread(void const *arg)
{
    // while (1) {
        // status = board->sensor_centre->get_distance(&distance);
        // if (status == VL53L0X_ERROR_NONE) {
        //     if (distance < 0.1) {
        //         speaker = 0;
        //     }
        // }
        // Thread::wait(500);
    // }    
}

// Thread5 compares alarm time to current time
void alarmThread(void const *arg)
{
    while(1) {

    }
}

void increase_time(void)
{
    if (time_set && !alarm_set_mode) return; // Invalid - Time has been set and the alarm mode not on
    
    if (hrs_mode) {
        if (!time_set) {
            if (curr_hr == 23) curr_hr = 0;
            else curr_hr++;
            return;
        }

        if (alarm_set_mode) {
            if (alarm_hr == 23) alarm_hr = 0;
            else alarm_hr++;
            return;
        }
    } else {
        if (!time_set) {
            if (curr_min == 59) curr_min = 0;
            else curr_min++;
            return;
        }

        if (alarm_set_mode) {
            if (alarm_min == 59) alarm_min = 0;
            else alarm_min++;
            return;
        }
    }
}

void decrease_time(void)
{
    if (time_set && !alarm_set_mode) return; // Invalid - Time has been set and the alarm mode not on

    if (hrs_mode) {
        if (!time_set) {
            if (curr_hr == 0) curr_hr = 23;
            else curr_hr--;
            return;
        }

        if (alarm_set_mode) {
            if (alarm_hr == 0) alarm_hr = 23;
            else alarm_hr--;
            return;
        }
    } else {
        if (!time_set) {
            if (curr_min == 0) curr_min = 59;
            else curr_min--;
            return;
        }

        if (alarm_set_mode) {
            if (alarm_min == 0) alarm_min = 59;
            else alarm_min--;
        }
    }
}

void pb_right(void)
{
    if (time_set && !alarm_set_mode) return; // Invalid - Time has been set and the alarm mode not on

    hrs_mode = 0;
}

void pb_left(void)
{
    if (time_set && !alarm_set_mode) return; // Invalid - Time has been set and the alarm mode not on

    hrs_mode = 1;
}

void pb_select(void)
{
    if (time_set && !alarm_set_mode) return; // Invalid - Time has been set and the alarm mode not on

    if (!time_set) {
        set_time(EPOCH_SECONDS + (3600 * curr_hr) + (60 * curr_min));
        // seconds_since = time(NULL) - EPOCH_SECONDS;
        time_set = 1;    // Time has been set
        return;
    }

    alarm_set_mode = !alarm_set_mode; // Toggle alarm mode.
    hrs_mode = 1;
}

int main()
{
    // DevI2C *device_i2c = new DevI2C(VL53L0_I2C_SDA, VL53L0_I2C_SCL);
    /* creates the 53L0A1 expansion board singleton obj */
    // board = XNucleo53L0A1::instance(device_i2c, A2, D8, D2);
    shdn = 0; //must reset sensor for an mbed reset to work
    wait(0.1);
    shdn = 1;
    wait(0.1);
    /* init the 53L0A1 board with default values */
    // status = board->init_board();
    // while (status) {
    //     pc.printf("Failed to init board! \r\n");
    //     status = board->init_board();
    // }


    // Get the current time as input from the user.
    time_set = false;
    up.mode(PullUp);
    down.mode(PullUp);
    left.mode(PullUp);
    right.mode(PullUp);
    select.mode(PullUp);

    up.attach_deasserted(&increase_time);
    down.attach_deasserted(&decrease_time);
    left.attach_deasserted(&pb_left);
    right.attach_deasserted(&pb_right);
    select.attach_deasserted(&pb_select);

    // Start display thread before others to handle setting time
    Thread t3(displayThread);

    // Start all other threads
    Thread t1(speakerThread);
    Thread t2(bluetoothThread);
    
    Thread t4(tofThread);
    Thread t5(alarmThread);

    // Continue with main thread - LCD display circle
    while (true) {
        Thread::wait(500);
    }
}