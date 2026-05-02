VOID CMD_SLEEP(U8 *line) {
    ARG_ARRAY args;
    RAW_LINE_TO_ARG_ARRAY(line, &args);
    if (args.argc < 2) {
        PUTS("Usage: sleep <seconds>\n");
        DELETE_ARG_ARRAY(&args);
        return;
    }


    U32 seconds = ATOI(args.argv[1]);
    if (seconds == 0 && STRCMP(args.argv[1], "0") != 0) {
        PUTS("Invalid number of seconds.\n");
        DELETE_ARG_ARRAY(&args);
        return;
    }
    CPU_SLEEP(seconds * 1000);
    DELETE_ARG_ARRAY(&args);
}