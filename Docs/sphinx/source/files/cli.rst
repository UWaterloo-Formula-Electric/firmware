######################
Command Line Interface
######################

The Command Line Interface (CLI) can be accessed over UART. Its baud rate is
230400, and otherwise uses standard UART parameters. To view the list of
commands, type ``help`` into the CLI and press return.

The following are constants in our code that define each command available to
run. The format is a little hard to read but the command in the CLI is
documented in the first set of quotes.

.. doxygenfile:: controlStateMachine_mock.c
  :sections: var

