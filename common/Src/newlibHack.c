// Getting rid of the warning: ____ is not implemented and will always fail
// See https://stackoverflow.com/questions/73742774/gcc-arm-none-eabi-11-3-is-not-implemented-and-will-always-fail
void _close(void) {
}

void _lseek(void) {
}

void _read(void) {
}

void _write(void) {
}

void _kill(void) {
}

void _isatty(void) {
}

void _getpid(void) {
}

void _fstat(void) {
}