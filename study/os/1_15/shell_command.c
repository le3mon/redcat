typedef void (*CommandFunction)(const char *pcParameter);

typedef struct kShellCommandEntryStruct {
    char *pcCommand;
    char *pcHelp;
    CommandFunction pfFunction;   
} SHELLCOMMANDENTRY;