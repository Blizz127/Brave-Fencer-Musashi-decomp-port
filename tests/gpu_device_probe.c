#include "reset_query_probe.h"

int main(int argc, char **argv)
{
    return run_reset_query_probe(argc, argv, musashi_boot_call_8005c1c0, 1);
}
