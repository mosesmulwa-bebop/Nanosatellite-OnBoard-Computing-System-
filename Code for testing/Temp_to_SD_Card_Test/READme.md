## Notes

The code has two tasks and uses a queue to communicate between tasks.

One task reads the temperature and stores it in a queue.
The other reads from the queue and writes it to the SD Card

Code uses pin 34 instead of 36 for the analog temp input

The LM35 sensor and the Esp32 should have a common ground. Power as shown in the pinout.
