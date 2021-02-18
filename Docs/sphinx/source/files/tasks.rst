*****
Tasks
*****

The system is controlled by a set of independent tasks. Each task contains
it's own context (registers, stack) and is assigned a priority at the start
of the system.

To learn more about tasks and how tasks are run, read the FreeRTOS
documentation.

https://www.freertos.org/taskandcr.html

https://www.freertos.org/implementation/a00002.html

.. doxygenfunction:: mainTaskFunction()
.. doxygenfunction:: fanTask()
.. doxygenfunction:: pcdcTask()
.. doxygenfunction:: sensorTask()
.. doxygenfunction:: controlTask(void *pvParameters)
.. doxygenfunction:: HVMeasureTask()
.. doxygenfunction:: batteryTask()
.. doxygenfunction:: canSendCellTask()
.. doxygenfunction:: faultMonitorTask()

