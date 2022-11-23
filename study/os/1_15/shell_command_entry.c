typedef void (*CommandFunction)(const char *pcParameter);

typedef struct kShellCommandEntryStruct {
    char *pcCommand;
    char *pcHelp;
    CommandFunction pfFunction;   
} SHELLCOMMANDENTRY;

SHELLCOMMANDENTRY gs_vstCommandTable[] {
    {"help", "Show Help", kHelp},
    {"cls", "Clear Screen", kCls}
}

void kExecuteCommand(const char *pcCommandBuffer) {
    int i, iSpaceIndex;
    int iCommandBufferLength, iCommandLength;
    int iCount;

    iCommandBufferLength = kStrLen( pcCommandBuffer );
    for( iSpaceIndex = 0 ; iSpaceIndex < iCommandBufferLength ; iSpaceIndex++ ) {
        if( pcCommandBuffer[ iSpaceIndex ] == ' ' )
            break;
    }

    iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);
}