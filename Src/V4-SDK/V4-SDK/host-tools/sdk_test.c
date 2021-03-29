#include "cu_test.h"

int main(int argc, char *argv[])
{
    ADD_SUITE(AIOT_MQTT);
    ADD_SUITE(CORE_UTILS);
    ADD_SUITE(CORE_HTTP);
    ADD_SUITE(AIOT_HTTP);
    ADD_SUITE(COMPONENT_COTA);
    ADD_SUITE(COMPONENT_FOTA);
    ADD_SUITE(PORTFILES_AT);

    cut_main(argc, argv);

    return 0;
}
