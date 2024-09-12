#include "client.h"
#include "connection.h"
#include "signal_handlers.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage : %s IP_ADDRESS\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    setup_signal_handlers();

    int ssock = create_client_socket(argv[1]);
    run_client(ssock);

    return 0;
}