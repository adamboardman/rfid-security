#include <algorithm>
#include <stdio.h>
#include <vector>

#include "rfid_2.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/binary_info.h"

const uint LED_PIN = 25;
const uint MAX_PWM_LEVEL = 65535;
const uint PWM_SLICES = 30;
const uint EXIT_GPIO_PIN = 21;
const uint LIST_ADD_PIN = 17;
const uint LIST_DEL_PIN = 18;
const uint I2C_GPIO_PIN_SDA = 26;
const uint I2C_GPIO_PIN_SLC = 27;
const uint8_t UID_IS_RANDOM = 0x08;
bool keep_running = true;
bool add_last_list = false;
bool del_last_list = false;
int on_list = 0;
int last_see_random = 0;
std::vector<Uid> approved_list;

RFID_2 rfid_2(i2c1, 0x28);

void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == EXIT_GPIO_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        keep_running = false;
    }
    if (gpio == LIST_ADD_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        add_last_list = true;
    }
    if (gpio == LIST_DEL_PIN && (events & GPIO_IRQ_EDGE_FALL)) {
        del_last_list = true;
    }
}

int main() {
    bi_decl(bi_program_description("Simple program to make use of the RFID_2 library."));
    bi_decl(bi_1pin_with_name(LED_PIN, "On-board LED"));
    bi_decl(bi_1pin_with_name(EXIT_GPIO_PIN, "Switch - pull to ground to exit the loop and return to usb-disk mode"));
    bi_decl(bi_1pin_with_name(LIST_ADD_PIN, "Switch - pull to ground to add last device to the approved list"));
    bi_decl(bi_1pin_with_name(LIST_DEL_PIN, "Switch - pull to ground to remove the last device from the list"));
    bi_decl(bi_1pin_with_name(I2C_GPIO_PIN_SDA, "SDA attached to RFID_2 device"));
    bi_decl(bi_1pin_with_name(I2C_GPIO_PIN_SLC, "SLC attached to RFID_2 device"));

    stdio_init_all();

    i2c_init(i2c1, 400000);
    gpio_set_function(I2C_GPIO_PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_GPIO_PIN_SLC, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_GPIO_PIN_SDA);
    gpio_pull_up(I2C_GPIO_PIN_SLC);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);

    uint sliceNum = pwm_gpio_to_slice_num(LED_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_init(sliceNum, &config, true);

    gpio_init(EXIT_GPIO_PIN);
    gpio_set_dir(EXIT_GPIO_PIN, GPIO_IN);
    gpio_pull_up(EXIT_GPIO_PIN);

    gpio_init(LIST_ADD_PIN);
    gpio_set_dir(LIST_ADD_PIN, GPIO_IN);
    gpio_pull_up(LIST_ADD_PIN);

    gpio_init(LIST_DEL_PIN);
    gpio_set_dir(LIST_DEL_PIN, GPIO_IN);
    gpio_pull_up(LIST_DEL_PIN);

    gpio_set_irq_enabled_with_callback(EXIT_GPIO_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(LIST_ADD_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(LIST_DEL_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    rfid_2.PCD_Init();

    while (keep_running) {
        if (on_list == 0 || last_see_random > 0) {
            pwm_set_gpio_level(LED_PIN, 0);
        } else {
            pwm_set_gpio_level(LED_PIN, (MAX_PWM_LEVEL/PWM_SLICES)*on_list);
        }
        sleep_ms(200);
        if (last_see_random > 0) {
            last_see_random--;
        } else if (on_list == 0) {
            pwm_set_gpio_level(LED_PIN,MAX_PWM_LEVEL/2);
        } else {
            pwm_set_gpio_level(LED_PIN,(MAX_PWM_LEVEL/PWM_SLICES)*on_list);
            on_list--;
        }
        bool present = rfid_2.PICC_IsNewCardPresent();

        if (present) {
            printf("PICC_ReadCardSerial: %d\n",rfid_2.PICC_ReadCardSerial());
            auto it = std::find(approved_list.begin(), approved_list.end(), rfid_2.uid);
            if (it != approved_list.end()) {
                printf("On Approved List\n");
                on_list = PWM_SLICES;
                last_see_random = 0;
            } else if (rfid_2.uid.uidByte[0] == UID_IS_RANDOM) {
                printf("Random UID detected - cannot be used for access\n");
                last_see_random = PWM_SLICES;
            } else {
                on_list = 0;
                last_see_random = 0;
            }
            rfid_2.PICC_DumpToSerial(&rfid_2.uid);
        }
        if (add_last_list) {
            if (gpio_get(LIST_ADD_PIN) && rfid_2.uid.uidByte[0] != UID_IS_RANDOM) {
                printf("Added to Approved List\n");
                approved_list.push_back(rfid_2.uid);
            }
            add_last_list = false;
        }
        if (del_last_list) {
            if (gpio_get(LIST_DEL_PIN)) {
                printf("Removed from Approved List\n");
                auto it = std::find(approved_list.begin(), approved_list.end(), rfid_2.uid);
                if (it != approved_list.end()) {
                    approved_list.erase(it);
                }
            }
            del_last_list = false;
        }
    }
    rfid_2.PCD_AntennaOff();
}
