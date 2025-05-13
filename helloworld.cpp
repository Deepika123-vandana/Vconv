#include <systemc.h>

SC_MODULE(HelloSystemC) {
    SC_CTOR(HelloSystemC) {
        SC_THREAD(say_hello);
    }

    void say_hello() {
        cout << "Hello, SystemC!" << endl;
    }
};

int sc_main(int argc, char* argv[]) {
    HelloSystemC hello("HELLO");
    sc_start();
    return 0;
}

