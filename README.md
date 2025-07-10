# Simple Demo App using the pico-rfid-2 library

Allows simple add/remove of approved cards, and changes the internal LED 
display to indicate which cards are on/off the approved list.

You'll need to pull down the library:
```
git submodule update --init
```

You need to plug things into the following PIN's or update the numbers as appropriate:
* EXIT_GPIO_PIN = 21
* LIST_ADD_PIN = 17
* LIST_DEL_PIN = 18
* I2C_GPIO_PIN_SDA = 26
* I2C_GPIO_PIN_SLC = 27
