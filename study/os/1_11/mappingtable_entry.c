#include "../MINT64/01.Kernel32/Source/Types.h"

typedef struct kKeyMappingEntryStruct {
    BYTE bNormalCode; // Shift 키나 Caps Lock 키와 조합되지 않은 아스키 코드

    BYTE bCombinedCode; // Shift 키나 Caps Lock 키와 조합된 아스키 코드
} KEYMAPPINGENTRY;