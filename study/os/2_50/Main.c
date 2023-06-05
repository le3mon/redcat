int g_iB = 3;

void A(void) {
    g_iB = 4;
}

void _START(void) {
    A();

    return;
}