#include <stdlib.h>
#include "server.h"

int main() {
        struct server_config *config = malloc(sizeof(struct server_config));
        load_config(config);
        display_config(config);
        return 0;
}
