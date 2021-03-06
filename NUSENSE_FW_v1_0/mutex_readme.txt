Example Summary
------------------
This example shows the use of two tasks and one semaphore to perform
mutually exclusive data access.

Example Usage
--------------
Run the application, two tasks will run and print their running status to the
console.

Application Design Details
-------------------------
Two tasks are constructed within main() with the first task set to a higher
priority than the second.

Each task attempts to perform work on a locked resourses utilizing the
Semaphore module. As such, when a resource is in use, the task will be
placed in the pending state until the resource lock is freed.

Note: For IAR users using any SensorTag(STK) Board, the XDS110 debugger must be
selected with the 4-wire JTAG connection within your projects' debugger
configuration.

References
-----------
For more help, search either the SYS/BIOS User Guide or the TIRTOS
Getting Started Guide within your TIRTOS installation.